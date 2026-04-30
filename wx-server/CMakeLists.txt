cmake_minimum_required(VERSION 3.20)
project(tinywechat-server VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Dependencies ──────────────────────────────────────────
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# asio is header-only, typically installed at /usr/include
find_path(ASIO_INCLUDE_DIR asio.hpp PATHS /usr/include)
if(ASIO_INCLUDE_DIR)
    message(STATUS "Found asio at ${ASIO_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "asio not found — install libasio-dev")
endif()

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
    ${ASIO_INCLUDE_DIR}
    src/
)

target_link_libraries(tinywechat-server PRIVATE
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
    Threads::Threads
    crypt      # for bcrypt via crypt_r()
)
