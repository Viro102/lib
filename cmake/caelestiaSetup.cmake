macro(caelestia_version)
    if(NOT DEFINED VERSION)
        execute_process(COMMAND git describe --tags --abbrev=0
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if("${VERSION}" STREQUAL "")
            message(FATAL_ERROR "VERSION is not set and failed to get from git")
        endif()
    endif()

    string(REGEX REPLACE "^v" "" VERSION "${VERSION}")
endmacro()

macro(caelestia_setup)
    include(GNUInstallDirs)
    caelestia_version()

    set(DISTRIBUTOR "Unset" CACHE STRING "Name of the party distributing this build")

    if(NOT DEFINED GIT_REVISION)
        execute_process(COMMAND git rev-parse HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_REVISION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if("${GIT_REVISION}" STREQUAL "")
            message(FATAL_ERROR "GIT_REVISION is not set and failed to get from git")
        endif()
    endif()

    set(caelestia_manifest "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.ini")
    file(WRITE "${caelestia_manifest}"
        "[Component]\n"
        "Version=${VERSION}\n"
        "Distributor=${DISTRIBUTOR}\n"
        "GitRevision=${GIT_REVISION}\n"
    )
    install(FILES "${caelestia_manifest}" DESTINATION "${CMAKE_INSTALL_DATADIR}/caelestia/components")
    unset(caelestia_manifest)
endmacro()
