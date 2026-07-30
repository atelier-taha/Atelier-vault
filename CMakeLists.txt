cmake_minimum_required(VERSION 3.16)
project(password_manager CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings for cleaner, safer code
add_compile_options(-Wall -Wextra -Wpedantic)

# Find Libsodium library (Tries pkg-config first, falls back to direct library search)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SODIUM libsodium)
endif()

# Fallback search if pkg-config is not present or didn't find libsodium
if(NOT SODIUM_FOUND)
    find_library(SODIUM_LIBRARIES NAMES sodium libsodium)
    find_path(SODIUM_INCLUDE_DIRS NAMES sodium.h)
endif()

# Ensure Libsodium was successfully located
if(NOT SODIUM_LIBRARIES)
    message(FATAL_ERROR "Libsodium not found! On Fedora, run: sudo dnf install libsodium-devel")
endif()

# Set include directories (your headers + libsodium headers)
include_directories(include ${SODIUM_INCLUDE_DIRS})

# Define source files for current build phase
set(SOURCES
    src/main.cpp
    src/crypto.cpp
    src/vault.cpp
    src/ui.cpp
)

# Define executable binary name
add_executable(password_manager ${SOURCES})

# Link Libsodium to the executable
target_link_libraries(password_manager PRIVATE ${SODIUM_LIBRARIES})
install(TARGETS password_manager DESTINATION /usr/local/bin)