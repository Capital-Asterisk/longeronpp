project(longeron-circuitsim)
add_executable(longeron-circuitsim "main.cpp" "circuit_builder.cpp")
target_link_libraries(longeron-circuitsim longeron)
