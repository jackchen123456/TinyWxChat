cmake_minimum_required(VERSION 3.20)
project(tinywechat-server VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Dependencies ──────────────────────────────────────────
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# ── Source files ──────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/session.cpp
    src/protocol.cpp
    src/database.cpp
)

set(HEADERS
    src/server.h
    src/session.h
    src/protocol.h
    src/database.h
)

# ── Executable ────────────────────────────────────────────
add_executable(tinywechat-server ${SOURCES} ${HEADERS})

target_include_directories(tinywechat-server PRIVATE
    src/
    ${CMAKE_CURRENT_SOURCE_DIR}/..   # 共享协议头：#include "protocol/common.h"
)

target_link_libraries(tinywechat-server PRIVATE
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
    Threads::Threads
)

# ── Platform-specific crypto ──────────────────────────────
if(APPLE)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(tinywechat-server PRIVATE OpenSSL::Crypto)
    target_include_directories(tinywechat-server PRIVATE ${OPENSSL_INCLUDE_DIR})
else()
    target_link_libraries(tinywechat-server PRIVATE crypt)
endif()
