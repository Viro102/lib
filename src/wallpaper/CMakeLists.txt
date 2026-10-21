add_library(caelestia-wallpaper OBJECT
    wallpaper.cpp
)
target_link_libraries(caelestia-wallpaper
    PUBLIC
        caelestia-headers
        caelestia-config
        caelestia-media
        caelestia-scheme
        caelestia-theme
        Qt::Core
)

set(default_wall "${CMAKE_INSTALL_FULL_DATADIR}/caelestia/wallpaper.webp")
target_compile_definitions(caelestia-wallpaper PRIVATE
    CAELESTIA_DEFAULT_WALLPAPER="${default_wall}"
)
install(FILES data/wallpaper.webp DESTINATION "${CMAKE_INSTALL_DATADIR}/caelestia")
