add_library(caelestia-config OBJECT
    config.cpp
    paths.cpp
)
target_link_libraries(caelestia-config
    PUBLIC
        caelestia-headers
        Qt::Core
)
