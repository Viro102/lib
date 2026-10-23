create_test(scheme scheme media utils config)
create_test(scheme generator media utils config)

target_include_directories(test-scheme-scheme PRIVATE "${PROJECT_SOURCE_DIR}/src/scheme")

target_link_libraries(test-scheme-generator PRIVATE m3colours::m3colours)
