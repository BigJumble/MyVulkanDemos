#include "setup.hpp"

#include "GLFW/glfw3.h"
#include "state.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace core
{
  core::raii::Window::Window(vk::raii::Instance const & instance)
  {
    static std::string lastGlfwError;
    glfwSetErrorCallback(
      [](int error, const char* msg)
      {
        lastGlfwError = std::string("GLFW Error (") + std::to_string(error) + "): " + (msg ? msg : "Unknown error");
        std::cerr << "[GLFW] " << lastGlfwError << std::endl;
      });

    isDebug(
      std::println(
        "[DisplayBundle] Initializing GLFW for window: '{}' ({}x{})",
        global::state::AppName,
        global::state::screenSize.width,
        global::state::screenSize.height));

    if (!glfwInit())
    {
      std::string errorMsg = "Failed to initialize GLFW";
      if (!lastGlfwError.empty())
        errorMsg += ": " + lastGlfwError;
      else
        errorMsg += " (no error details available)";
      std::println("[DisplayBundle] ERROR: {}", errorMsg);
      throw std::runtime_error(errorMsg);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(
      global::state::screenSize.width,
      global::state::screenSize.height,
      global::state::AppName.data(),
      nullptr,
      nullptr
    );
    if (!window)
    {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window!");
    }
  }

  vk::raii::SurfaceKHR createWindowSurface( const vk::raii::Instance & instance, GLFWwindow * window )
  {
    VkSurfaceKHR _surface;
    if ( glfwCreateWindowSurface( *instance, window, nullptr, &_surface ) != VK_SUCCESS )
    {
      glfwDestroyWindow( window );
      glfwTerminate();
      throw std::runtime_error( "Failed to create window surface!" );
    }
    return vk::raii::SurfaceKHR( instance, _surface );
  }

}  // namespace core
