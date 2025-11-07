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
    // if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS )
    // {
    //   global::state::fpvMode = true;
    //   // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // For FPS camera
    // }
  }

  inline double virtualxPos = 0, virtualyPos = 0;
  inline double lastX = 0, lastY = 0;

  inline void cursorPositionCallback( GLFWwindow * win, double xpos, double ypos )
  {
    float deltaX = xpos - lastX;
    float deltaY = ypos - lastY;

    lastX = static_cast<float>( xpos );
    lastY = static_cast<float>( ypos );

    if ( global::state::fpvMode )
    {
      global::state::cameraRotation.x -= static_cast<float>( deltaX ) / 1000.0f;
      global::state::cameraRotation.y -= static_cast<float>( deltaY ) / 1000.0f;
    }
    else
    {
      virtualxPos += deltaX;
      virtualyPos += deltaY;

      // Clamp xpos, ypos to screen size if available
      if ( global::state::screenSize.width > 0 && global::state::screenSize.height > 0 )
      {
        if ( virtualxPos < 0 )
          virtualxPos = 0;
        if ( virtualyPos < 0 )
          virtualyPos = 0;
        if ( virtualxPos > global::state::screenSize.width - 1 )
          virtualxPos = global::state::screenSize.width - 1;
        if ( virtualyPos > global::state::screenSize.height - 1 )
          virtualyPos = global::state::screenSize.height - 1;
      }
    }

    // Forward to previously installed callback (keeps ImGui responsive)
    if ( previousCursorPosCallback )
    {
      previousCursorPosCallback( win, virtualxPos, virtualyPos );
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

  // inline void windowSizeCallback( GLFWwindow * win, int width, int height )
  // {
  //   global::state::windowWidth  = width;
  //   global::state::windowHeight = height;
  // }

  // inline void cursorEnterCallback( GLFWwindow * win, int entered )
  // {
  //   global::state::cursorInWindow = entered != 0;
  // }
}  // namespace input
