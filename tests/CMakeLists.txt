find_package(Qt6 REQUIRED COMPONENTS Test)

set(CMAKE_AUTOMOC ON)

add_executable(test_cache cache.cpp)
target_link_libraries(test_cache PRIVATE libcaelestia Qt::Test)
add_test(NAME cache COMMAND test_cache)
