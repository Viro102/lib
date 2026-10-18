cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED VERSION)
    execute_process(COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if("${VERSION}" STREQUAL "")
        message(FATAL_ERROR "VERSION is not set and failed to get from git")
    endif()
endif()

string(REGEX REPLACE "^v" "" VERSION "${VERSION}")

project(caelestia VERSION ${VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql)

add_library(caelestia-headers INTERFACE)
target_include_directories(caelestia-headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_subdirectory(src/config)
add_subdirectory(src/utils)
add_subdirectory(src/scheme)

add_library(caelestia SHARED)
set_target_properties(caelestia PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(caelestia PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(caelestia
    PUBLIC
        Qt::Core
    PRIVATE
        caelestia-config
        caelestia-utils
)

install(TARGETS caelestia EXPORT caelestiaTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT caelestiaTargets
    NAMESPACE caelestia::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/caelestia
)

configure_package_config_file(cmake/caelestiaConfig.cmake.in
    ${CMAKE_BINARY_DIR}/caelestiaConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/caelestia
)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/caelestiaConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_BINARY_DIR}/caelestiaConfig.cmake
    ${CMAKE_BINARY_DIR}/caelestiaConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/caelestia
)

option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
