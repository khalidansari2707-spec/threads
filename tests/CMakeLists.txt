add_executable(multicorex_tests test_all.cpp)
target_link_libraries(multicorex_tests PRIVATE multicorex_core)

add_test(NAME AllUnitTests COMMAND multicorex_tests)
