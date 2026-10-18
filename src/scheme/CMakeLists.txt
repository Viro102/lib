include(FetchContent)
FetchContent_Declare(m3colours
    GIT_REPOSITORY https://github.com/soramanew/m3colours.git
    GIT_TAG        cb09a60779ad9dcee068d7bfa3e0df54a8316a30
)
FetchContent_MakeAvailable(m3colours)

add_library(caelestia-scheme OBJECT
    generator.cpp
    scheme.cpp
    score.cpp
)
target_link_libraries(caelestia-scheme
    PUBLIC
        caelestia-headers
        caelestia-utils
        m3colours::m3colours
        Qt::Core
        Qt::Gui
)

file(GLOB_RECURSE scheme_files
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    data/*
)
qt_add_resources(caelestia-scheme "schemes"
    PREFIX "/schemes"
    BASE data
    FILES ${scheme_files}
)
