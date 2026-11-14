# CMakeLists.txt - optional CMake build for the Community Connect backend
# -----------------------------------------------------------------------------
# The Makefile is the primary, simplest way to build:
#
#     cd backend && make
#
# This file exists for people who prefer CMake or want to open the project in
# VS Code with the CMake Tools extension. Both produce the same binary.
#
#     cmake -S . -B build-cmake
#     cmake --build build-cmake
#     ./build-cmake/cc-server --selftest
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)
project(CommunityConnect LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# FileManager.h, SessionManager.h and PriorityQueue.h are header-only
# (templates / inline), so they are not listed here. Only translation units
# belong in add_executable.
add_executable(cc-server
    src/main.cpp
    src/PasswordHasher.cpp
    src/UserService.cpp
    src/ResourceService.cpp
    src/NotificationService.cpp
    src/RequestService.cpp
    src/ConsoleDemo.cpp
    src/SelfTest.cpp
    src/SearchService.cpp
    src/HttpServer.cpp
)

target_include_directories(cc-server PRIVATE include)

# Warnings help catch real mistakes; they are not errors so a fresh clone still
# builds on a slightly different toolchain.
target_compile_options(cc-server PRIVATE
    -Wall -Wextra -Wpedantic
)

# On macOS password hashing uses CommonCrypto, which is part of the system.
# On other platforms OpenSSL is required.
if(NOT APPLE)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(cc-server PRIVATE OpenSSL::Crypto)
endif()
