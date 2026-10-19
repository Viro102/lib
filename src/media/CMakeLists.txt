include(FetchContent)
FetchContent_Declare(m3colours
    GIT_REPOSITORY https://github.com/soramanew/m3colours.git
    GIT_TAG        fa15d0ddcc6011344cb150d8677fc2f4fe14d5da
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
        m3colours::m3colours
        Qt::Core
        Qt::Gui
    PRIVATE
        PkgConfig::libav
)
