include(FetchContent)
FetchContent_Declare(m3colours
    GIT_REPOSITORY https://github.com/soramanew/m3colours.git
    GIT_TAG        f38b9185357d22bd1b63e7dd14c0c044c268c4cd
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
