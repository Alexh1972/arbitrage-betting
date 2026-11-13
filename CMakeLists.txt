cmake_minimum_required(VERSION 3.24)
project(arbitrage_betting LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(Threads REQUIRED)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

find_package(CURL QUIET)
if(NOT CURL_FOUND)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
    set(BUILD_MISC_DOCS OFF CACHE BOOL "" FORCE)
    set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
    set(CURL_ZLIB OFF CACHE STRING "" FORCE)
    set(CURL_BROTLI OFF CACHE STRING "" FORCE)
    set(CURL_ZSTD OFF CACHE STRING "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
    if(WIN32)
        set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    else()
        set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_Declare(
        curl
        URL https://github.com/curl/curl/releases/download/curl-8_11_1/curl-8.11.1.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(curl)
endif()

FetchContent_Declare(
    odpic
    URL https://github.com/oracle/odpi/archive/refs/tags/v5.4.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR _none
)
FetchContent_MakeAvailable(odpic)

add_library(odpic STATIC ${odpic_SOURCE_DIR}/embed/dpi.c)
target_include_directories(odpic PUBLIC ${odpic_SOURCE_DIR}/include)
if(UNIX)
    target_link_libraries(odpic PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)
endif()

set(ORT_VERSION 1.20.1)
if(WIN32)
    set(ORT_ARCHIVE onnxruntime-win-x64-${ORT_VERSION}.zip)
elseif(APPLE)
    set(ORT_ARCHIVE onnxruntime-osx-universal2-${ORT_VERSION}.tgz)
else()
    set(ORT_ARCHIVE onnxruntime-linux-x64-${ORT_VERSION}.tgz)
endif()

FetchContent_Declare(
    onnxruntime
    URL https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/${ORT_ARCHIVE}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR _none
)
FetchContent_MakeAvailable(onnxruntime)

add_library(ort::onnxruntime SHARED IMPORTED)
set_target_properties(ort::onnxruntime PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${onnxruntime_SOURCE_DIR}/include)
if(WIN32)
    set_target_properties(ort::onnxruntime PROPERTIES
        IMPORTED_LOCATION ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll
        IMPORTED_IMPLIB ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib)
    file(GLOB ORT_RUNTIME_LIBS ${onnxruntime_SOURCE_DIR}/lib/*.dll)
elseif(APPLE)
    set_target_properties(ort::onnxruntime PROPERTIES IMPORTED_LOCATION ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.dylib)
    file(GLOB ORT_RUNTIME_LIBS ${onnxruntime_SOURCE_DIR}/lib/*.dylib)
else()
    set_target_properties(ort::onnxruntime PROPERTIES IMPORTED_LOCATION ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so)
    file(GLOB ORT_RUNTIME_LIBS ${onnxruntime_SOURCE_DIR}/lib/*.so*)
endif()

set(MODEL_DIR ${CMAKE_BINARY_DIR}/models)
set(MODEL_VERSION 0.33.0)
set(MODEL_ARTIFACT langchain4j-embeddings-bge-small-en-v15-q)
if(NOT EXISTS ${MODEL_DIR}/bge-small-en-v1.5-q.onnx OR NOT EXISTS ${MODEL_DIR}/bge-small-en-v1.5-tokenizer.json)
    set(MODEL_JAR ${CMAKE_BINARY_DIR}/${MODEL_ARTIFACT}-${MODEL_VERSION}.jar)
    file(DOWNLOAD
        https://repo1.maven.org/maven2/dev/langchain4j/${MODEL_ARTIFACT}/${MODEL_VERSION}/${MODEL_ARTIFACT}-${MODEL_VERSION}.jar
        ${MODEL_JAR}
        STATUS MODEL_DOWNLOAD_STATUS)
    list(GET MODEL_DOWNLOAD_STATUS 0 MODEL_DOWNLOAD_CODE)
    if(NOT MODEL_DOWNLOAD_CODE EQUAL 0)
        message(FATAL_ERROR "Could not download embedding model: ${MODEL_DOWNLOAD_STATUS}")
    endif()
    file(ARCHIVE_EXTRACT INPUT ${MODEL_JAR} DESTINATION ${MODEL_DIR} PATTERNS "*.onnx" "*.json")
    file(REMOVE ${MODEL_JAR})
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(arbitrage_betting ${SOURCES})
target_include_directories(arbitrage_betting PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(arbitrage_betting PRIVATE
    nlohmann_json::nlohmann_json
    CURL::libcurl
    odpic
    ort::onnxruntime
    Threads::Threads)

if(MSVC)
    target_compile_options(arbitrage_betting PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(arbitrage_betting PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(arbitrage_betting PRIVATE -Wall -Wextra)
    set_target_properties(arbitrage_betting PROPERTIES BUILD_RPATH "$ORIGIN" INSTALL_RPATH "$ORIGIN")
endif()

foreach(LIB ${ORT_RUNTIME_LIBS})
    add_custom_command(TARGET arbitrage_betting POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB} $<TARGET_FILE_DIR:arbitrage_betting>)
endforeach()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/config/application.properties)
    configure_file(${CMAKE_SOURCE_DIR}/config/application.properties.example ${CMAKE_SOURCE_DIR}/config/application.properties COPYONLY)
endif()

add_custom_command(TARGET arbitrage_betting POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/config/application.properties $<TARGET_FILE_DIR:arbitrage_betting>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${MODEL_DIR} $<TARGET_FILE_DIR:arbitrage_betting>/models)
