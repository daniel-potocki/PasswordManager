cmake_minimum_required(VERSION 3.26)

project(PasswordManager)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "/Users/daniel/Qt/6.10.2/macos")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(PasswordManager main.cpp)

target_link_libraries(PasswordManager PRIVATE Qt6::Widgets)