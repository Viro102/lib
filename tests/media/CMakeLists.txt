create_test(media analysis utils)

target_include_directories(test-media-analysis PRIVATE "${PROJECT_SOURCE_DIR}/src/media")

target_link_libraries(test-media-analysis PRIVATE m3colours::m3colours)
