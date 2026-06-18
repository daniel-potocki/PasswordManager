cmake_minimum_required(VERSION 3.26)

project(PasswordManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
find_package(OpenSSL REQUIRED)

find_path(ARGON2_INCLUDE_DIR argon2.h
        PATHS /opt/homebrew/include /usr/local/include
)

find_library(ARGON2_LIBRARY argon2
        PATHS /opt/homebrew/lib /usr/local/lib
)

if(NOT ARGON2_INCLUDE_DIR OR NOT ARGON2_LIBRARY)
    message(FATAL_ERROR "Argon2 not found. Install it with: brew install argon2")
endif()

qt_standard_project_setup()

add_executable(PasswordManager
        main.cpp
)

target_include_directories(PasswordManager PRIVATE
        ${ARGON2_INCLUDE_DIR}
)

target_link_libraries(PasswordManager PRIVATE
        Qt6::Widgets
        Qt6::Sql
        OpenSSL::Crypto
        ${ARGON2_LIBRARY}
)