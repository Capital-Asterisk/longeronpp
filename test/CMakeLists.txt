enable_testing()
include(GoogleTest)

function(lgrn_add_test name sources libs)
    add_executable("test_${name}" ${sources})
    target_link_libraries("test_${name}" gtest_main ${libs})
    gtest_discover_tests("test_${name}")
endfunction()

lgrn_add_test(hierarchical_bitset hierarchical_bitset.cpp longeron)
lgrn_add_test(intarray_multimap intarray_multimap.cpp longeron)
lgrn_add_test(id_registry id_management/registry.cpp longeron)
