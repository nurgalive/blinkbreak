# Core library
if(NOT TARGET blinkbreak_core)
    add_library(blinkbreak_core STATIC)
    target_sources(blinkbreak_core
        PRIVATE
            core/timer.cpp
            core/timer.hpp
            core/state_machine.cpp
            core/state_machine.hpp
            core/config_manager.cpp
            core/config_manager.hpp
            core/config_types.hpp
            core/message_provider.cpp
            core/message_provider.hpp
            core/break_scheduler.cpp
            core/break_scheduler.hpp
    )
    target_include_directories(blinkbreak_core
        PUBLIC
            ${PROJECT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(blinkbreak_core
        PUBLIC
            rapidjson::rapidjson
            spdlog::spdlog
    )
    target_compile_definitions(blinkbreak_core
        PRIVATE
            _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    )
endif()

# UI library
if(NOT TARGET blinkbreak_ui)
    add_library(blinkbreak_ui STATIC)
    target_sources(blinkbreak_ui
        PRIVATE
            ui/app_controller.cpp
            ui/app_controller.hpp
    )
    target_include_directories(blinkbreak_ui
        PUBLIC
            ${PROJECT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(blinkbreak_ui
        PUBLIC
            blinkbreak_core
            Slint::Slint
    )
    target_compile_definitions(blinkbreak_ui
        PRIVATE
            _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    )
    target_compile_options(blinkbreak_ui
        PRIVATE
            /wd4624
    )
    slint_target_sources(blinkbreak_ui
        ${PROJECT_SOURCE_DIR}/ui/main_window.slint
    )
endif()

# Main executable
if(NOT TARGET blinkbreak)
    add_executable(blinkbreak main.cpp)
    target_link_libraries(blinkbreak
        PRIVATE
            blinkbreak_ui
    )
    target_compile_definitions(blinkbreak
        PRIVATE
            _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    )
    if(WIN32)
        add_custom_command(TARGET blinkbreak POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:blinkbreak> $<TARGET_FILE_DIR:blinkbreak>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()
