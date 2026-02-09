# Platform-specific implementations

add_library(blinkbreak_platform STATIC)

target_sources(blinkbreak_platform
    PRIVATE
        platform_interface.hpp
)

# Add Windows-specific sources if on Windows
if(WIN32)
    target_sources(blinkbreak_platform
        PRIVATE
            windows/tray_icon_win.hpp
            windows/tray_icon_win.cpp
    )
endif()

target_include_directories(blinkbreak_platform
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(blinkbreak_platform
    PUBLIC
        spdlog::spdlog
)

# Link Windows-specific libraries
if(WIN32)
    target_link_libraries(blinkbreak_platform
        PRIVATE
            shell32
    )
endif()

target_compile_features(blinkbreak_platform PUBLIC cxx_std_23)
