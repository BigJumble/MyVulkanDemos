#pragma once
#include "state.hpp"
#include "structs.hpp"

#include <GLFW/glfw3.h>

// #include <print>

namespace input
{
  // Store the previously installed GLFW callbacks (e.g., ImGui's)
  inline GLFWwindowfocusfun previousWindowFocusCallback = nullptr;
  inline GLFWcursorenterfun previousCursorEnterCallback = nullptr;
  inline GLFWcursorposfun   previousCursorPosCallback   = nullptr;
  inline GLFWmousebuttonfun previousMouseButtonCallback = nullptr;
  inline GLFWscrollfun      previousScrollCallback      = nullptr;
  inline GLFWkeyfun         previousKeyCallback         = nullptr;
  inline GLFWcharfun        previousCharCallback        = nullptr;
  inline GLFWmonitorfun     previousMonitorCallback     = nullptr;

  inline void framebufferResizeCallback( GLFWwindow * win, int, int )
  {
    global::state::framebufferResized = true;
  }

  inline double virtualxPos = global::state::screenSize.width / 2.0, virtualyPos = global::state::screenSize.height / 2.0;
  inline double lastX = 0, lastY = 0;

  inline void keyCallback( GLFWwindow * win, int key, int scancode, int action, int mods )
  {
    if ( key == GLFW_KEY_F1 && action == GLFW_PRESS )
    {
      if ( global::state::imguiMode )
      {
        virtualxPos = lastX;
        virtualyPos = lastY;
      }
      if ( !global::state::imguiMode )
      {
        glfwSetCursorPos( win, virtualxPos, virtualyPos );
      }

      global::state::imguiMode = !global::state::imguiMode;
    }

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

    if ( global::state::imguiMode )
    {
      if ( previousKeyCallback )
      {
        previousKeyCallback( win, key, scancode, action, mods );
      }
      return;
    }

    core::Key coreKey = static_cast<core::Key>( key );

    switch ( coreKey )
    {
      case core::Key::W:
      case core::Key::A:
      case core::Key::S:
      case core::Key::D:
      case core::Key::LeftShift:
      case core::Key::RightShift:
      case core::Key::LeftControl:
      case core::Key::RightControl:
      case core::Key::Space:
      case core::Key::Escape:
      case core::Key::F11:
        if ( action == GLFW_PRESS )
        {
          global::state::keysPressed.insert( coreKey );
          global::state::keysDown.insert( coreKey );
        }
        if ( action == GLFW_RELEASE )
        {
          global::state::keysPressed.erase( coreKey );
          global::state::keysUp.insert( coreKey );
        }
        break;
      default: break;
    }
  }

  inline void mouseButtonCallback( GLFWwindow * win, int button, int action, int mods )
  {
    if ( global::state::imguiMode )
    {
      if ( previousMouseButtonCallback )
      {
        previousMouseButtonCallback( win, button, action, mods );
      }
      return;
    }

    core::Key coreKey = static_cast<core::Key>( button );

    switch ( coreKey )
    {
      case core::Key::MouseLeft:
      case core::Key::MouseMiddle:
      case core::Key::MouseRight:

        if ( action == GLFW_PRESS )
        {
          global::state::keysPressed.insert( coreKey );
          global::state::keysDown.insert( coreKey );
        }
        if ( action == GLFW_RELEASE )
        {
          global::state::keysPressed.erase( coreKey );
          global::state::keysUp.insert( coreKey );
        }
        break;
      default: break;
    }
  }

  inline void cursorPositionCallback( GLFWwindow * win, double xpos, double ypos )
  {
    global::state::cursorDelta.x = xpos - lastX;
    global::state::cursorDelta.y = ypos - lastY;

    lastX = static_cast<float>( xpos );
    lastY = static_cast<float>( ypos );

    if ( global::state::imguiMode )
    {
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

      lastX = static_cast<float>( xpos );
      lastY = static_cast<float>( ypos );

      // Forward to previously installed callback (keeps ImGui responsive)
      if ( previousCursorPosCallback )
      {
        glfwSetCursorPos( win, xpos, ypos );
        previousCursorPosCallback( win, xpos, ypos );
      }
      return;
    }

    global::state::cameraRotation.x -= static_cast<float>( global::state::cursorDelta.x ) / 1000.0f;
    global::state::cameraRotation.y -= static_cast<float>( global::state::cursorDelta.y ) / 1000.0f;
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

  inline void cursorEnterCallback( GLFWwindow * win, int entered )
  {
    if ( entered == 1 )
      glfwSetCursorPos( win, virtualxPos, virtualyPos );

    if ( previousCursorEnterCallback )
    {
      previousCursorEnterCallback( win, entered );
    }
  }
}  // namespace input
