#pragma once

#include <dpi.h>

#include <cstdint>
#include <string>
#include <vector>

namespace arb {

struct DataSourceConfig {
    std::string host;
    int port = 1521;
    std::string sid;
    std::string username;
    std::string password;
    unsigned int poolSize = 50;
};

class OracleDatabase;

class Connection {
public:
    Connection(OracleDatabase& database, dpiConn* conn);
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&& other) noexcept;
    ~Connection();

    void commit();
    void rollback();
    void execute(const std::string& sql);

    dpiConn* handle() const { return conn_; }
    OracleDatabase& database() const { return *database_; }

private:
    OracleDatabase* database_;
    dpiConn* conn_;
};

class Statement {
public:
    Statement(Connection& connection, const std::string& sql);
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    ~Statement();

    void bindString(const std::string& name, const std::string& value);
    void bindInt(const std::string& name, long long value);
    void bindDouble(const std::string& name, double value);
    void bindBytes(const std::string& name, const std::vector<std::uint8_t>& value);

    void execute();
    bool fetch();
    std::uint64_t rowCount();

    void defineLongString(std::uint32_t position);
    void defineLongBytes(std::uint32_t position);
    void defineInt(std::uint32_t position);
    void defineDouble(std::uint32_t position);

    std::string getString(std::uint32_t position);
    std::vector<std::uint8_t> getBytes(std::uint32_t position);
    long long getInt(std::uint32_t position);
    double getDouble(std::uint32_t position);

private:
    dpiData* getValue(std::uint32_t position);
    dpiVar* newVar(dpiOracleTypeNum oracleType, dpiNativeTypeNum nativeType, std::uint32_t size, dpiData** data);
    void bindVar(const std::string& name, dpiVar* var);

    Connection& connection_;
    dpiStmt* stmt_ = nullptr;
    std::vector<dpiVar*> vars_;
    std::uint32_t bufferRowIndex_ = 0;
};

class OracleDatabase {
public:
    explicit OracleDatabase(const DataSourceConfig& config);
    OracleDatabase(const OracleDatabase&) = delete;
    OracleDatabase& operator=(const OracleDatabase&) = delete;
    ~OracleDatabase();

    Connection acquire();

    void check(int status) const;
    [[noreturn]] void raise() const;

private:
    dpiContext* context_ = nullptr;
    dpiPool* pool_ = nullptr;
};

}
