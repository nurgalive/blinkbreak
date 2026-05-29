# Test executable - Unit tests
add_executable(blinkbreak_tests
    unit/test_timer.cpp
    unit/test_state_machine.cpp
    unit/test_config_manager.cpp
    unit/test_message_provider.cpp
    unit/test_break_scheduler.cpp
    unit/test_monitor_manager.cpp
    unit/test_idle_detector.cpp
    unit/test_notification.cpp
    unit/test_dnd_detector.cpp
)

target_link_libraries(blinkbreak_tests
    PRIVATE
        blinkbreak_core
        blinkbreak_platform
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(blinkbreak_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(blinkbreak_tests
    PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=1
)

# Register tests with CTest
gtest_discover_tests(blinkbreak_tests DISCOVERY_TIMEOUT 30 DISCOVERY_MODE PRE_TEST)

# Integration tests
add_executable(blinkbreak_integration_tests
    integration/test_harness.cpp
    integration/test_full_workflow.cpp
    integration/test_idle_integration.cpp
    integration/test_dnd_integration.cpp
    integration/test_config_changes.cpp
    integration/test_extended_simulation.cpp
)

target_link_libraries(blinkbreak_integration_tests
    PRIVATE
        blinkbreak_core
        blinkbreak_platform
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(blinkbreak_integration_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/integration
)

target_compile_definitions(blinkbreak_integration_tests
    PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=1
)

# Register integration tests with CTest
gtest_discover_tests(blinkbreak_integration_tests
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
    TEST_PREFIX "integration_"
)

# Performance tests
add_executable(blinkbreak_performance_tests
    performance/test_memory.cpp
    performance/test_cpu.cpp
    performance/test_startup.cpp
)

target_link_libraries(blinkbreak_performance_tests
    PRIVATE
        blinkbreak_core
        blinkbreak_platform
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(blinkbreak_performance_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(blinkbreak_performance_tests
    PRIVATE
        GTEST_LINKED_AS_SHARED_LIBRARY=1
)

# Link Windows-specific libraries for memory profiling
if(WIN32)
    target_link_libraries(blinkbreak_performance_tests PRIVATE psapi)
endif()

# Register performance tests with CTest
gtest_discover_tests(blinkbreak_performance_tests
    DISCOVERY_TIMEOUT 120
    DISCOVERY_MODE PRE_TEST
    TEST_PREFIX "perf_"
)

# UI tests subdirectory
add_subdirectory(ui)
