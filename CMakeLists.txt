cmake_minimum_required(VERSION 3.25)
project(libcaelestia LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql)

add_library(caelestia-headers INTERFACE)
target_include_directories(caelestia-headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_subdirectory(src/config)
add_subdirectory(src/utils)

add_library(caelestia SHARED)
add_library(caelestia::caelestia ALIAS caelestia)
target_link_libraries(caelestia
    PUBLIC
        caelestia-headers
        caelestia-config
        caelestia-utils
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
