cmake_minimum_required(VERSION 3.16)
project(CommunityConnect LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

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

target_compile_options(cc-server PRIVATE
    -Wall -Wextra -Wpedantic
)

if(NOT APPLE)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(cc-server PRIVATE OpenSSL::Crypto)
endif()
