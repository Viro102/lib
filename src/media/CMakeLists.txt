include(FetchContent)
FetchContent_Declare(m3colours
    GIT_REPOSITORY https://github.com/soramanew/m3colours.git
    GIT_TAG        66121b6cb43ab0f3f0d96e7b10c48f2fedb13cf1
)
FetchContent_MakeAvailable(m3colours)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libav REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswscale)

add_library(caelestia-media OBJECT
    analysis.cpp
    videopixels.cpp
)
target_link_libraries(caelestia-media
    PUBLIC
        caelestia-headers
        caelestia-utils
        Qt::Core
        Qt::Gui
    PRIVATE
        m3colours::m3colours
        PkgConfig::libav
)
