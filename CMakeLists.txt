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

set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/icons/app.icns")
set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")

set_source_files_properties(${APP_ICON_MACOS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
)

qt_add_executable(PasswordManager
        MANUAL_FINALIZATION
        main.cpp
        resources.qrc
        ${APP_ICON_WINDOWS}
        ${APP_ICON_MACOS}
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

set_target_properties(PasswordManager PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE app.icns
)

qt_finalize_executable(PasswordManager)