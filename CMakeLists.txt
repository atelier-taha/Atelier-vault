cmake_minimum_required(VERSION 3.16)
project(password_manager CXX)

# Force C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt's automatic Meta-Object Compiler (MOC), User Interface Compiler (UIC), and Resource Compiler (RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Export compile_commands.json automatically to fix VS Code IntelliSense header errors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict compiler warnings for clean C++ code
add_compile_options(-Wall -Wextra -Wpedantic)

# Find Qt6 Widgets library
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Find Libsodium package
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SODIUM libsodium)
endif()

# Fallback check for Libsodium if pkg-config isn't present
if(NOT SODIUM_FOUND)
    find_library(SODIUM_LIBRARIES NAMES sodium libsodium)
    find_path(SODIUM_INCLUDE_DIRS NAMES sodium.h)
endif()

if(NOT SODIUM_LIBRARIES)
    message(FATAL_ERROR "Libsodium not found! On Fedora, run: sudo dnf install libsodium-devel")
endif()

# Set include directories
include_directories(include ${SODIUM_INCLUDE_DIRS})

# List all project source files and headers so CMake re-compiles cleanly on changes
set(SOURCES
    src/main.cpp
    src/loginview.cpp
    src/crypto.cpp
    src/vault.cpp
    src/mainwindow.cpp
    include/loginview.h
    include/crypto.h
    include/vault.h
    include/mainwindow.h
)

# Build the binary executable
add_executable(password_manager ${SOURCES})

# Link Qt6 Widgets and Libsodium
target_link_libraries(password_manager PRIVATE Qt6::Widgets ${SODIUM_LIBRARIES})

# Optional system-wide installation rule
install(TARGETS password_manager DESTINATION /usr/local/bin)