create_test(scheme scheme utils)
create_test(scheme score utils)
create_test(scheme generator utils)

target_include_directories(test-scheme-scheme PRIVATE "${PROJECT_SOURCE_DIR}/src/scheme")
target_include_directories(test-scheme-generator PRIVATE "${PROJECT_SOURCE_DIR}/src/scheme")
