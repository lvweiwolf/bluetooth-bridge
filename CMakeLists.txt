
# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.10)
cmake_policy(SET CMP0048 NEW)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)
set(VERSION_PLAIN ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

project(bluetooth-bridge VERSION ${VERSION_PLAIN})


if(NOT DEFINED PROJECT_SOURCE_DIR)
	set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()


# 引用编译器配置
include(${PROJECT_SOURCE_DIR}/cmake/config.cmake)

# 头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/src/lib)
include_directories(${PROJECT_BINARY_DIR}/src/lib)

set(DEPENDENCY_LIBS "")

# mqtt 库
find_package(unofficial-mosquitto CONFIG REQUIRED)
list(APPEND DEPENDENCY_LIBS unofficial::mosquitto::mosquittopp)

# systemd 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(Systemd REQUIRED IMPORTED_TARGET libsystemd)
list(APPEND DEPENDENCY_LIBS PkgConfig::Systemd)

# sdbus-cpp 库
find_package(sdbus-c++ REQUIRED)
list(APPEND DEPENDENCY_LIBS SDBusCpp::sdbus-c++)

# spdlog 库
find_package(spdlog CONFIG REQUIRED)
list(APPEND DEPENDENCY_LIBS spdlog::spdlog)

# json 库
find_package(jsoncpp CONFIG REQUIRED)
list(APPEND DEPENDENCY_LIBS JsonCpp::JsonCpp)

# dbus 库
# find_package(DBus1 CONFIG REQUIRED)
# list(APPEND DEPENDENCY_LIBS dbus-1)

# BlueZ 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLUETOOTH REQUIRED bluez)
list(APPEND DEPENDENCY_LIBS ${BLUETOOTH_LIBRARIES})

add_subdirectory(src/lib)
add_subdirectory(src/bin)

# 二进制输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
