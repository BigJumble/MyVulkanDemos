#pragma once
#include "state.hpp"

#include <GLFW/glfw3.h>


namespace input
{
  // Store the previously installed GLFW cursor position callback (e.g., ImGui's)
  inline GLFWcursorposfun previousCursorPosCallback = nullptr;

  inline void framebufferResizeCallback( GLFWwindow * win, int, int )
  {
    global::state::framebufferResized = true;
  }

  inline void keyCallback( GLFWwindow * win, int key, int scancode, int action, int mods )
  {
    if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
    {
      if ( global::state::fpvMode )
      {
        global::state::fpvMode = false;
        // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // For FPS camera // doesn't work bruh, gotta make virtual cursor
      }
      else
      {
        global::state::fpvMode = true;
        // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // For FPS camera
      }
    }
    if ( key == GLFW_KEY_W && action == GLFW_PRESS )
      global::state::cameraPosition.z += 0.1f;
    if ( key == GLFW_KEY_S && action == GLFW_PRESS )
      global::state::cameraPosition.z -= 0.1f;
    if ( key == GLFW_KEY_A && action == GLFW_PRESS )
      global::state::cameraPosition.x -= 0.1f;
    if ( key == GLFW_KEY_D && action == GLFW_PRESS )
      global::state::cameraPosition.x += 0.1f;

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
      global::state::fpvMode = true;
      // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // For FPS camera
    }
  }

  inline double virtualxPos, virtualyPos;

  inline void cursorPositionCallback( GLFWwindow * win, double xpos, double ypos )
  {
    if ( global::state::fpvMode )
    {
      float x = virtualxPos - xpos + global::state::screenSize.width * 0.5f;
      float y = virtualyPos - ypos + global::state::screenSize.height * 0.5f;
    }
    else
    {
      virtualxPos = xpos;
      virtualyPos = ypos;
    }
    // Clamp xpos, ypos to screen size if available
    if ( global::state::screenSize.width > 0 && global::state::screenSize.height > 0 )
    {
      if ( xpos < 0 )
        xpos = 0;
      if ( ypos < 0 )
        ypos = 0;
      if ( xpos > global::state::screenSize.width - 1 )
        xpos = global::state::screenSize.width - 1;
      if ( ypos > global::state::screenSize.height - 1 )
        ypos = global::state::screenSize.height - 1;
    }

    global::state::cameraRotation.x += static_cast<float>( xpos - global::state::lastX ) / 1000.0f;
    global::state::cameraRotation.y += static_cast<float>( global::state::lastY - ypos ) / 1000.0f;
    global::state::lastX = static_cast<float>( xpos );
    global::state::lastY = static_cast<float>( ypos );

    // Forward to previously installed callback (keeps ImGui responsive)
    if ( previousCursorPosCallback )
    {
      previousCursorPosCallback( win, xpos, ypos );
    }
  }

  inline void scrollCallback( GLFWwindow * win, double xoffset, double yoffset )
  {
    global::state::cameraZoom -= static_cast<float>( yoffset ) * 0.1f;
    if ( global::state::cameraZoom < 0.1f )
      global::state::cameraZoom = 0.1f;
    if ( global::state::cameraZoom > 10.0f )
      global::state::cameraZoom = 10.0f;
  }

  inline void windowSizeCallback( GLFWwindow * win, int width, int height )
  {
    global::state::windowWidth  = width;
    global::state::windowHeight = height;
  }

  inline void cursorEnterCallback( GLFWwindow * win, int entered )
  {
    global::state::cursorInWindow = entered != 0;
  }
}  // namespace input
