enable_testing()

add_executable(hello hello.cpp)
target_link_libraries(hello gtest_main)

include(GoogleTest)
gtest_discover_tests(hello)
