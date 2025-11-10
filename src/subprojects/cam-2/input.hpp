#pragma once
#include "state.hpp"

#include <GLFW/glfw3.h>


namespace input
{
  // Store the previously installed GLFW cursor position callback (e.g., ImGui's)
  inline GLFWcursorposfun previousCursorPosCallback = nullptr;

  inline void framebufferResizeCallback( GLFWwindow * win, int, int )
  {
    state::framebufferResized = true;
  }

  inline void keyCallback( GLFWwindow * win, int key, int scancode, int action, int mods )
  {
    if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
    {
      if ( state::imguiMode )
      {
        state::imguiMode = false;
        // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // For FPS camera // doesn't work bruh, gotta make virtual cursor
      }
      else
      {
        state::imguiMode = true;
        // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // For FPS camera
      }
    }
    if ( key == GLFW_KEY_W && action == GLFW_PRESS )
      state::cameraPosition.z += 0.1f;
    if ( key == GLFW_KEY_S && action == GLFW_PRESS )
      state::cameraPosition.z -= 0.1f;
    if ( key == GLFW_KEY_A && action == GLFW_PRESS )
      state::cameraPosition.x -= 0.1f;
    if ( key == GLFW_KEY_D && action == GLFW_PRESS )
      state::cameraPosition.x += 0.1f;

    if ( key == GLFW_KEY_F11 && action == GLFW_PRESS )
    {
      static bool         isFullScreen = false;
      static int          windowedX, windowedY, windowedWidth, windowedHeight;
      GLFWmonitor *       monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode * mode    = glfwGetVideoMode( monitor );

      if ( !isFullScreen )
      {
        // Save window position and size
        glfwGetWindowPos( win, &windowedX, &windowedY );
        glfwGetWindowSize( win, &windowedWidth, &windowedHeight );
        // Go fullscreen
        glfwSetWindowMonitor( win, monitor, 0, 0, mode->width, mode->height, mode->refreshRate );
        isFullScreen = true;
      }
      else
      {
        // Restore windowed mode
        glfwSetWindowMonitor( win, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0 );
        isFullScreen = false;
      }
    }
  }

  inline void mouseButtonCallback( GLFWwindow * win, int button, int action, int mods )
  {
    if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS )
    {
      state::imguiMode = true;
      // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // For FPS camera
    }
  }

  inline double virtualxPos, virtualyPos;

  inline void cursorPositionCallback( GLFWwindow * win, double xpos, double ypos )
  {
    if ( state::imguiMode )
    {
      float x = virtualxPos - xpos + state::screenSize.width * 0.5f;
      float y = virtualyPos - ypos + state::screenSize.height * 0.5f;
    }
    else
    {
      virtualxPos = xpos;
      virtualyPos = ypos;
    }
    // Clamp xpos, ypos to screen size if available
    if ( state::screenSize.width > 0 && state::screenSize.height > 0 )
    {
      if ( xpos < 0 )
        xpos = 0;
      if ( ypos < 0 )
        ypos = 0;
      if ( xpos > state::screenSize.width - 1 )
        xpos = state::screenSize.width - 1;
      if ( ypos > state::screenSize.height - 1 )
        ypos = state::screenSize.height - 1;
    }

    state::cameraRotation.x += static_cast<float>( xpos - state::lastX ) / 1000.0f;
    state::cameraRotation.y += static_cast<float>( state::lastY - ypos ) / 1000.0f;
    state::lastX = static_cast<float>( xpos );
    state::lastY = static_cast<float>( ypos );

    // Forward to previously installed callback (keeps ImGui responsive)
    if ( previousCursorPosCallback )
    {
      previousCursorPosCallback( win, xpos, ypos );
    }
  }

  inline void scrollCallback( GLFWwindow * win, double xoffset, double yoffset )
  {
    state::cameraZoom -= static_cast<float>( yoffset ) * 0.1f;
    if ( state::cameraZoom < 0.1f )
      state::cameraZoom = 0.1f;
    if ( state::cameraZoom > 10.0f )
      state::cameraZoom = 10.0f;
  }

  inline void windowSizeCallback( GLFWwindow * win, int width, int height )
  {
    state::windowWidth  = width;
    state::windowHeight = height;
  }

  inline void cursorEnterCallback( GLFWwindow * win, int entered )
  {
    state::cursorInWindow = entered != 0;
  }
}  // namespace input
