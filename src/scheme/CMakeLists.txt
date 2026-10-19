include(FetchContent)
FetchContent_Declare(m3colours
    GIT_REPOSITORY https://github.com/soramanew/m3colours.git
    GIT_TAG        f9157fdf3ce0d6aa9d8e4c25f05fd945a1c231f0
)
FetchContent_MakeAvailable(m3colours)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libav REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswscale)

add_library(caelestia-scheme OBJECT
    generator.cpp
    scheme.cpp
    score.cpp
    videopixels.cpp
)
target_link_libraries(caelestia-scheme
    PUBLIC
        caelestia-headers
        caelestia-config
        caelestia-utils
        m3colours::m3colours
        Qt::Core
        Qt::Gui
    PRIVATE
        PkgConfig::libav
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
