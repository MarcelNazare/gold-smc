add_executable(gold_smc_tests
    test_main.cpp
    test_indicators.cpp
    test_strategy.cpp
)
target_link_libraries(gold_smc_tests PRIVATE goldsmc_core)
target_include_directories(gold_smc_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME gold_smc_tests COMMAND gold_smc_tests)
