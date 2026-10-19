add_library(caelestia-scheme OBJECT
    generator.cpp
    scheme.cpp
)
target_link_libraries(caelestia-scheme
    PUBLIC
        caelestia-headers
        caelestia-config
        caelestia-media
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
