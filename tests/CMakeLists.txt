# Test executable
if(NOT TARGET blinkbreak_tests)
    add_executable(blinkbreak_tests
        unit/test_timer.cpp
        unit/test_state_machine.cpp
        unit/test_config_manager.cpp
        unit/test_message_provider.cpp
        unit/test_break_scheduler.cpp
    )

    target_link_libraries(blinkbreak_tests
        PRIVATE
            blinkbreak_core
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
    )

    target_include_directories(blinkbreak_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )




    # Register tests with CTest
    gtest_discover_tests(blinkbreak_tests DISCOVERY_TIMEOUT 30 DISCOVERY_MODE PRE_TEST)
endif()

add_subdirectory(ui)
