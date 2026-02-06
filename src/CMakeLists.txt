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
endif()

# Main executable
if(NOT TARGET blinkbreak)
    add_executable(blinkbreak main.cpp)
    target_link_libraries(blinkbreak
        PRIVATE
            blinkbreak_core
    )
endif()
