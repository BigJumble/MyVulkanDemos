#pragma once
#include <GLFW/glfw3.h>
#include "state.hpp"

namespace input
{
    inline void framebufferResizeCallback( GLFWwindow * win, int, int )
    {
      state::framebufferResized = true;
    }

    inline void keyCallback( GLFWwindow * win, int key, int scancode, int action, int mods )
    {
      if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
      {
        state::fpsMode = false;
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // For FPS camera
      }
      if ( key == GLFW_KEY_W && action == GLFW_PRESS )
        state::cameraPosition.z += 0.1f;
      if ( key == GLFW_KEY_S && action == GLFW_PRESS )
        state::cameraPosition.z -= 0.1f;
      if ( key == GLFW_KEY_A && action == GLFW_PRESS )
        state::cameraPosition.x -= 0.1f;
      if ( key == GLFW_KEY_D && action == GLFW_PRESS )
        state::cameraPosition.x += 0.1f;
    }

    inline void mouseButtonCallback( GLFWwindow * win, int button, int action, int mods )
    {
      if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS )
      {
        state::fpsMode = true;
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // For FPS camera
      }
    }
    
    inline void cursorPositionCallback( GLFWwindow * win, double xpos, double ypos )
    {
      state::cameraRotation.x += (float)(xpos - state::lastX) / 1000.0f;
      state::cameraRotation.y += (float)(state::lastY - ypos) / 1000.0f;
      state::lastX = xpos;
      state::lastY = ypos;
    }
    
    inline void scrollCallback(GLFWwindow* win, double xoffset, double yoffset)
    {
      state::cameraZoom -= static_cast<float>(yoffset) * 0.1f;
      if (state::cameraZoom < 0.1f) state::cameraZoom = 0.1f;
      if (state::cameraZoom > 10.0f) state::cameraZoom = 10.0f;
    }

    inline void windowSizeCallback(GLFWwindow* win, int width, int height)
    {
      state::windowWidth = width;
      state::windowHeight = height;
    }

    inline void cursorEnterCallback(GLFWwindow* win, int entered)
    {
      state::cursorInWindow = entered != 0;
    }
}  // namespace input