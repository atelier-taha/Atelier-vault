cmake_minimum_required(VERSION 3.16)
project(password_manager CXX)

# Force C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt's automatic MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# MSVC C++ Standard & Preprocessor Fix for Qt 6
# ==============================================================================
if(MSVC)
    add_compile_options(/Zc:__cplusplus /std:c++17 /W4 /permissive-)
    # Clean Release optimization flags (/O2: Maximize speed, /GL: Whole program optimization, /Gy: Function-level linking)
    add_compile_options("$<$<CONFIG:Release>:/O2;/GL;/Gy>")
    add_link_options("$<$<CONFIG:Release>:/LTCG>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ==============================================================================
# Qt 6 Path Discovery
# ==============================================================================
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
endif()

# Find Qt6 Widgets library
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# ==============================================================================
# Find Libsodium Package
# ==============================================================================
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SODIUM libsodium)
endif()

if(NOT SODIUM_FOUND)
    find_library(SODIUM_LIBRARIES 
        NAMES sodium libsodium libsodium-static
        HINTS "C:/libsodium/x64/Release/v143/dynamic" 
              "C:/libsodium/x64/Release/v142/dynamic"
              "C:/libsodium/lib" 
              "C:/libsodium/x64"
              "C:/libsodium"
              "/opt/homebrew/lib"
              "/usr/local/lib"
    )
    find_path(SODIUM_INCLUDE_DIRS 
        NAMES sodium.h 
        HINTS "C:/libsodium/include" 
              "C:/libsodium"
              "/opt/homebrew/include"
              "/usr/local/include"
    )
endif()

if(NOT SODIUM_LIBRARIES OR NOT SODIUM_INCLUDE_DIRS)
    message(FATAL_ERROR "Libsodium not found! Ensure libsodium is installed and contains include/sodium.h and the library file.")
endif()

# Set include and library directories
include_directories(include ${SODIUM_INCLUDE_DIRS})
if(SODIUM_LIBRARY_DIRS)
    link_directories(${SODIUM_LIBRARY_DIRS})
endif()

# List all project source files, headers, and Qt resource definitions
set(SOURCES
    src/main.cpp
    src/crypto.cpp
    src/vault.cpp
    src/dialogs.cpp
    src/loginview.cpp
    src/mainwindow.cpp
    include/crypto.h
    include/vault.h
    include/dialogs.h
    include/loginview.h
    include/mainwindow.h
    resources.qrc
)

# Embed Windows application icon resource into target sources on Windows
if(WIN32)
    list(APPEND SOURCES app_icon.rc)
endif()

# Build binary target (WIN32 suppresses console on Windows; MACOSX_BUNDLE creates .app on macOS)
if(WIN32)
    add_executable(password_manager WIN32 ${SOURCES})
elseif(APPLE)
    add_executable(password_manager MACOSX_BUNDLE ${SOURCES})
    set_target_properties(password_manager PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Atelier's Vault"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.atelier.ateliervault"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
    )
else()
    add_executable(password_manager ${SOURCES})
endif()

# Set output name to AtelierVault
set_target_properties(password_manager PROPERTIES OUTPUT_NAME "AtelierVault")

# Link libraries
target_link_libraries(password_manager PRIVATE Qt6::Widgets ${SODIUM_LIBRARIES})