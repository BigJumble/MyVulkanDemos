#pragma once
#include "state.hpp"
#include "structs.hpp"

#include <GLFW/glfw3.h>

// #include <print>
namespace eeng
{
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
      eeng::state::framebufferResized = true;
    }

    inline double virtualxPos = eeng::state::screenSize.width / 2.0, virtualyPos = eeng::state::screenSize.height / 2.0;
    inline double lastX = 0, lastY = 0;

    inline void keyCallback( GLFWwindow * win, int key, int scancode, int action, int mods )
    {
      if ( key == GLFW_KEY_F1 && action == GLFW_PRESS )
      {
        if ( eeng::state::imguiMode )
        {
          virtualxPos = lastX;
          virtualyPos = lastY;
        }
        if ( !eeng::state::imguiMode )
        {
          glfwSetCursorPos( win, virtualxPos, virtualyPos );
        }

        eeng::state::imguiMode = !eeng::state::imguiMode;
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

      if ( eeng::state::imguiMode )
      {
        if ( previousKeyCallback )
        {
          previousKeyCallback( win, key, scancode, action, mods );
        }
        return;
      }

      Key coreKey = static_cast<Key>( key );

      switch ( coreKey )
      {
        case Key::W:
        case Key::A:
        case Key::S:
        case Key::D:
        case Key::LeftShift:
        case Key::RightShift:
        case Key::LeftControl:
        case Key::RightControl:
        case Key::Space:
        case Key::Escape:
        case Key::F11:
          if ( action == GLFW_PRESS )
          {
            eeng::state::keysPressed.insert( coreKey );
            eeng::state::keysDown.insert( coreKey );
          }
          if ( action == GLFW_RELEASE )
          {
            eeng::state::keysPressed.erase( coreKey );
            eeng::state::keysUp.insert( coreKey );
          }
          break;
        default: break;
      }
    }

    inline void mouseButtonCallback( GLFWwindow * win, int button, int action, int mods )
    {
      if ( eeng::state::imguiMode )
      {
        if ( previousMouseButtonCallback )
        {
          previousMouseButtonCallback( win, button, action, mods );
        }
        return;
      }

      Key coreKey = static_cast<Key>( button );

      switch ( coreKey )
      {
        case Key::MouseLeft:
        case Key::MouseMiddle:
        case Key::MouseRight:

          if ( action == GLFW_PRESS )
          {
            eeng::state::keysPressed.insert( coreKey );
            eeng::state::keysDown.insert( coreKey );
          }
          if ( action == GLFW_RELEASE )
          {
            eeng::state::keysPressed.erase( coreKey );
            eeng::state::keysUp.insert( coreKey );
          }
          break;
        default: break;
      }
    }

    inline void cursorPositionCallback( GLFWwindow * win, double xpos, double ypos )
    {
      eeng::state::cursorDelta.x = xpos - lastX;
      eeng::state::cursorDelta.y = ypos - lastY;

      lastX = static_cast<float>( xpos );
      lastY = static_cast<float>( ypos );

      if ( eeng::state::imguiMode )
      {
        // Clamp xpos, ypos to screen size if available
        if ( eeng::state::screenSize.width > 0 && eeng::state::screenSize.height > 0 )
        {
          if ( xpos < 0 )
            xpos = 0;
          if ( ypos < 0 )
            ypos = 0;
          if ( xpos > eeng::state::screenSize.width - 1 )
            xpos = eeng::state::screenSize.width - 1;
          if ( ypos > eeng::state::screenSize.height - 1 )
            ypos = eeng::state::screenSize.height - 1;
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

      eeng::state::cameraRotation.x -= static_cast<float>( eeng::state::cursorDelta.x ) / 1000.0f;
      eeng::state::cameraRotation.y -= static_cast<float>( eeng::state::cursorDelta.y ) / 1000.0f;
    }

    inline void scrollCallback( GLFWwindow * win, double xoffset, double yoffset )
    {
      eeng::state::cameraZoom -= static_cast<float>( yoffset ) * 0.1f;
      if ( eeng::state::cameraZoom < 0.1f )
        eeng::state::cameraZoom = 0.1f;
      if ( eeng::state::cameraZoom > 10.0f )
        eeng::state::cameraZoom = 10.0f;
    }

    // inline void windowSizeCallback( GLFWwindow * win, int width, int height )
    // {
    //   eeng::state::windowWidth  = width;
    //   eeng::state::windowHeight = height;
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
}  // namespace eeng
