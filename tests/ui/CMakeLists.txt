# UI test executable(s)
if(NOT TARGET blinkbreak_ui_tests)
    add_executable(blinkbreak_ui_tests
        test_main_window.cpp
        test_settings_dialog.cpp
        test_timer_display.cpp
        test_overlay.cpp
    )

    target_link_libraries(blinkbreak_ui_tests
        PRIVATE
            Slint::Slint
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
    )

    target_include_directories(blinkbreak_ui_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )

    target_compile_definitions(blinkbreak_ui_tests
        PRIVATE
            _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    )
    target_compile_options(blinkbreak_ui_tests
        PRIVATE
            /wd4624
    )

    if(WIN32)
        add_custom_command(TARGET blinkbreak_ui_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:blinkbreak_ui_tests> $<TARGET_FILE_DIR:blinkbreak_ui_tests>
            COMMAND_EXPAND_LISTS
        )
    endif()

    slint_target_sources(blinkbreak_ui_tests
        ${PROJECT_SOURCE_DIR}/ui/main_window.slint
        ${PROJECT_SOURCE_DIR}/ui/overlay.slint
        ${PROJECT_SOURCE_DIR}/ui/components/timer_display.slint
    )

    gtest_discover_tests(blinkbreak_ui_tests
        DISCOVERY_TIMEOUT 30
        DISCOVERY_MODE PRE_TEST
        PROPERTIES LABELS ui
        TEST_PREFIX "ui_"
    )
endif()
