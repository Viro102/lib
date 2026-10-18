cmake_minimum_required(VERSION 3.25)
project(libcaelestia LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql)

add_library(caelestia SHARED
    src/utils/cache.cpp
    src/utils/cacheworker.cpp
)
target_link_libraries(caelestia
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Sql
)
target_include_directories(caelestia
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
