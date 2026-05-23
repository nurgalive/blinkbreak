/// @file main.cpp
/// @brief Entry point for BlinkBreak application.

#include <cstdlib>
#include <iostream>
#include <string>

#include <blinkbreak/version.hpp>
#include <spdlog/spdlog.h>

#include "ui/app_controller.hpp"

/// @brief Main entry point for the application.
/// @param argc Argument count.
/// @param argv Argument values.
/// @return Exit code (0 for success).
int main(int argc, char* argv[])
{
  // Force Slint to use the software renderer to avoid the DXGI pre-rotation
  // bug on portrait monitors. GPU renderers (FemtoVG/wgpu) create a swapchain
  // sized for logical (rotated) dimensions but don't apply the matching
  // pre-rotation matrix, causing stretched/misrotated framebuffers on portrait
  // displays. The software renderer bypasses GPU compositing entirely.
  // Only set if not already overridden by the user.

  // For windows keep winit-software
#ifdef _WIN32
  if (std::getenv("SLINT_BACKEND") == nullptr)
  {
    _putenv_s("SLINT_BACKEND", "winit-femtovg-wgpu");
    // _putenv_s("SLINT_BACKEND", "winit-software");  // this backend produces the rendering
    // issues
  }
#else
  ::setenv("SLINT_BACKEND", "winit-software", 0);
#endif
  // Configure logging
  spdlog::set_level(spdlog::level::debug);

  spdlog::info("{} v{}", blinkbreak::kAppName, blinkbreak::kVersionString);

  // Check for version flag
  for (int i = 1; i < argc; ++i)
  {
    std::string arg(argv[i]);
    if (arg == "--version" || arg == "-v")
    {
      std::cout << blinkbreak::kAppName << " version " << blinkbreak::kVersionString << std::endl;
      return 0;
    }
  }

  // Create and run application
  blinkbreak::AppController app;

  if (!app.Initialize())
  {
    spdlog::error("Failed to initialize application");
    return 1;
  }

  return app.Run();
}
