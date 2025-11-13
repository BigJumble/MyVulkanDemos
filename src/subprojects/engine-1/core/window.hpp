#pragma once
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <string>
#include <print>

namespace
{
  void errorCallback(int error, const char* msg)
  {
    static std::string formatted = std::to_string(error) + (msg ? msg : "Unknown error");
    std::println("GLFW Error: {}", formatted);
    throw std::runtime_error(formatted);
  }
}

namespace eeng
{
  namespace raii
  {
    struct Window
    {
      GLFWwindow * window = nullptr;
      vk::SurfaceKHR surface = nullptr;

      // Default constructor
      Window() = default;

      // Construct from vk::raii::Instance (main use)
      Window(vk::raii::Instance const & instance)
      {
        glfwSetErrorCallback(errorCallback);

        if (!glfwInit())
        {
          throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Replace this with your actual global/application parameters
        int width = 1280, height = 720;
        const char* title = "App";
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window)
        {
          glfwTerminate();
          throw std::runtime_error("Failed to create GLFW window!");
        }

        VkSurfaceKHR raw_surface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &raw_surface) != VK_SUCCESS)
        {
          glfwDestroyWindow(window);
          glfwTerminate();
          throw std::runtime_error("Failed to create window surface!");
        }
        surface = vk::SurfaceKHR(raw_surface);
      }

      // Construct from raw GLFWwindow*
      explicit Window(GLFWwindow* w) : window(w) {}

      // Move constructor
      Window(Window&& other) noexcept
        : window(other.window), surface(other.surface)
      {
        other.window = nullptr;
        other.surface = nullptr;
      }

      // Move assignment operator
      Window& operator=(Window&& other) noexcept
      {
        if (this != &other)
        {
          reset();
          window = other.window;
          surface = other.surface;
          other.window = nullptr;
          other.surface = nullptr;
        }
        return *this;
      }

      // Delete copy constructor and copy assignment
      Window(const Window&) = delete;
      Window& operator=(const Window&) = delete;

      // Destroy window and surface, if owned
      void reset()
      {
        // NOTE: vk::SurfaceKHR is a handle wrapper (non-raii), 
        // destroying must be handled externally by the user.
        surface = nullptr;
        if (window)
        {
          glfwDestroyWindow(window);
          glfwTerminate();
          window = nullptr;
        }
      }

      // Get underlying pointer
      GLFWwindow* get() const
      {
        return window;
      }

      // Get underlying Vulkan surface handle
      vk::SurfaceKHR getSurface() const
      {
        return surface;
      }

      // Convenience conversion
      operator GLFWwindow* () const
      {
        return window;
      }

      ~Window()
      {
        reset();
      }
    };
  }  // namespace raii
}  // namespace eeng
