#include "db/OracleDatabase.h"

#include <stdexcept>
#include <utility>

namespace arb {

OracleDatabase::OracleDatabase(const DataSourceConfig& config) {
    dpiErrorInfo errorInfo;
    if (dpiContext_createWithParams(DPI_MAJOR_VERSION, DPI_MINOR_VERSION, nullptr, &context_, &errorInfo) != DPI_SUCCESS) {
        throw std::runtime_error("Cannot create Oracle context: " + std::string(errorInfo.message, errorInfo.messageLength));
    }

    dpiCommonCreateParams commonParams;
    check(dpiContext_initCommonCreateParams(context_, &commonParams));
    commonParams.createMode = DPI_MODE_CREATE_THREADED;
    commonParams.encoding = "UTF-8";
    commonParams.nencoding = "UTF-8";

    dpiPoolCreateParams poolParams;
    check(dpiContext_initPoolCreateParams(context_, &poolParams));
    poolParams.minSessions = 1;
    poolParams.maxSessions = config.poolSize;
    poolParams.sessionIncrement = 1;
    poolParams.getMode = DPI_MODE_POOL_GET_WAIT;

    const std::string connectString = "(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)(HOST=" + config.host
        + ")(PORT=" + std::to_string(config.port) + "))(CONNECT_DATA=(SID=" + config.sid + ")))";

    check(dpiPool_create(context_,
        config.username.c_str(), static_cast<std::uint32_t>(config.username.size()),
        config.password.c_str(), static_cast<std::uint32_t>(config.password.size()),
        connectString.c_str(), static_cast<std::uint32_t>(connectString.size()),
        &commonParams, &poolParams, &pool_));
}

OracleDatabase::~OracleDatabase() {
    if (pool_) {
        dpiPool_release(pool_);
    }
    if (context_) {
        dpiContext_destroy(context_);
    }
}

Connection OracleDatabase::acquire() {
    dpiConn* conn = nullptr;
    check(dpiPool_acquireConnection(pool_, nullptr, 0, nullptr, 0, nullptr, &conn));
    return Connection(*this, conn);
}

void OracleDatabase::check(int status) const {
    if (status != DPI_SUCCESS) {
        raise();
    }
}

void OracleDatabase::raise() const {
    dpiErrorInfo errorInfo;
    dpiContext_getError(context_, &errorInfo);
    throw std::runtime_error(std::string(errorInfo.message, errorInfo.messageLength));
}

Connection::Connection(OracleDatabase& database, dpiConn* conn) : database_(&database), conn_(conn) {}

Connection::Connection(Connection&& other) noexcept
    : database_(other.database_), conn_(std::exchange(other.conn_, nullptr)) {}

Connection::~Connection() {
    if (conn_) {
        dpiConn_release(conn_);
    }
}

void Connection::commit() {
    database_->check(dpiConn_commit(conn_));
}

void Connection::rollback() {
    database_->check(dpiConn_rollback(conn_));
}

void Connection::execute(const std::string& sql) {
    Statement statement(*this, sql);
    statement.execute();
}

Statement::Statement(Connection& connection, const std::string& sql) : connection_(connection) {
    connection_.database().check(dpiConn_prepareStmt(connection_.handle(), 0,
        sql.c_str(), static_cast<std::uint32_t>(sql.size()), nullptr, 0, &stmt_));
}

Statement::~Statement() {
    if (stmt_) {
        dpiStmt_release(stmt_);
    }
    for (dpiVar* var : vars_) {
        dpiVar_release(var);
    }
}

dpiVar* Statement::newVar(dpiOracleTypeNum oracleType, dpiNativeTypeNum nativeType, std::uint32_t size, dpiData** data) {
    dpiVar* var = nullptr;
    connection_.database().check(dpiConn_newVar(connection_.handle(), oracleType, nativeType, 1, size, 1, 0, nullptr, &var, data));
    vars_.push_back(var);
    return var;
}

void Statement::bindVar(const std::string& name, dpiVar* var) {
    connection_.database().check(dpiStmt_bindByName(stmt_, name.c_str(), static_cast<std::uint32_t>(name.size()), var));
}

void Statement::bindString(const std::string& name, const std::string& value) {
    const auto size = static_cast<std::uint32_t>(value.size());
    const auto oracleType = size > 4000 ? DPI_ORACLE_TYPE_LONG_VARCHAR : DPI_ORACLE_TYPE_VARCHAR;
    dpiData* data = nullptr;
    dpiVar* var = newVar(oracleType, DPI_NATIVE_TYPE_BYTES, size == 0 ? 1 : size, &data);
    if (value.empty()) {
        data->isNull = 1;
    } else {
        connection_.database().check(dpiVar_setFromBytes(var, 0, value.data(), size));
    }
    bindVar(name, var);
}

void Statement::bindInt(const std::string& name, long long value) {
    dpiData* data = nullptr;
    dpiVar* var = newVar(DPI_ORACLE_TYPE_NUMBER, DPI_NATIVE_TYPE_INT64, 0, &data);
    data->isNull = 0;
    data->value.asInt64 = value;
    bindVar(name, var);
}

void Statement::bindDouble(const std::string& name, double value) {
    dpiData* data = nullptr;
    dpiVar* var = newVar(DPI_ORACLE_TYPE_NUMBER, DPI_NATIVE_TYPE_DOUBLE, 0, &data);
    data->isNull = 0;
    data->value.asDouble = value;
    bindVar(name, var);
}

void Statement::bindBytes(const std::string& name, const std::vector<std::uint8_t>& value) {
    const auto size = static_cast<std::uint32_t>(value.size());
    const auto oracleType = size > 2000 ? DPI_ORACLE_TYPE_LONG_RAW : DPI_ORACLE_TYPE_RAW;
    dpiData* data = nullptr;
    dpiVar* var = newVar(oracleType, DPI_NATIVE_TYPE_BYTES, size == 0 ? 1 : size, &data);
    if (value.empty()) {
        data->isNull = 1;
    } else {
        connection_.database().check(dpiVar_setFromBytes(var, 0, reinterpret_cast<const char*>(value.data()), size));
    }
    bindVar(name, var);
}

void Statement::execute() {
    std::uint32_t numColumns = 0;
    connection_.database().check(dpiStmt_execute(stmt_, DPI_MODE_EXEC_DEFAULT, &numColumns));
}

bool Statement::fetch() {
    int found = 0;
    connection_.database().check(dpiStmt_fetch(stmt_, &found, &bufferRowIndex_));
    return found != 0;
}

std::uint64_t Statement::rowCount() {
    std::uint64_t count = 0;
    connection_.database().check(dpiStmt_getRowCount(stmt_, &count));
    return count;
}

void Statement::defineLongString(std::uint32_t position) {
    connection_.database().check(dpiStmt_defineValue(stmt_, position, DPI_ORACLE_TYPE_LONG_VARCHAR, DPI_NATIVE_TYPE_BYTES, 0, 0, nullptr));
}

void Statement::defineLongBytes(std::uint32_t position) {
    connection_.database().check(dpiStmt_defineValue(stmt_, position, DPI_ORACLE_TYPE_LONG_RAW, DPI_NATIVE_TYPE_BYTES, 0, 0, nullptr));
}

void Statement::defineInt(std::uint32_t position) {
    connection_.database().check(dpiStmt_defineValue(stmt_, position, DPI_ORACLE_TYPE_NUMBER, DPI_NATIVE_TYPE_INT64, 0, 0, nullptr));
}

void Statement::defineDouble(std::uint32_t position) {
    connection_.database().check(dpiStmt_defineValue(stmt_, position, DPI_ORACLE_TYPE_NUMBER, DPI_NATIVE_TYPE_DOUBLE, 0, 0, nullptr));
}

dpiData* Statement::getValue(std::uint32_t position) {
    dpiNativeTypeNum nativeType;
    dpiData* data = nullptr;
    connection_.database().check(dpiStmt_getQueryValue(stmt_, position, &nativeType, &data));
    return data;
}

std::string Statement::getString(std::uint32_t position) {
    const dpiData* data = getValue(position);
    if (data->isNull) {
        return "";
    }
    return std::string(data->value.asBytes.ptr, data->value.asBytes.length);
}

std::vector<std::uint8_t> Statement::getBytes(std::uint32_t position) {
    const dpiData* data = getValue(position);
    if (data->isNull) {
        return {};
    }
    const auto* begin = reinterpret_cast<const std::uint8_t*>(data->value.asBytes.ptr);
    return std::vector<std::uint8_t>(begin, begin + data->value.asBytes.length);
}

long long Statement::getInt(std::uint32_t position) {
    const dpiData* data = getValue(position);
    return data->isNull ? 0 : data->value.asInt64;
}

double Statement::getDouble(std::uint32_t position) {
    const dpiData* data = getValue(position);
    return data->isNull ? 0.0 : data->value.asDouble;
}

}
