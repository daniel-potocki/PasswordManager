cmake_minimum_required(VERSION 3.26)

project(PasswordManager)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
find_package(OpenSSL REQUIRED)

qt_standard_project_setup()

add_executable(PasswordManager
        main.cpp
)

target_link_libraries(PasswordManager PRIVATE
        Qt6::Widgets
        Qt6::Sql
        OpenSSL::Crypto
)