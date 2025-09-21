#include "base/helper.hpp"
#include "base/test.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <print>
#include <string>
#include <vulkan/vulkan.hpp>

static std::string AppName    = "01_InitInstance";
static std::string EngineName = "Vulkan.hpp";

int main()
{
  // Initialize GLFW
  if ( !glfwInit() )
  {
    std::println( "Failed to initialize GLFW" );
    return -1;
  }

  // Tell GLFW not to create an OpenGL context
  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

  // Create a windowed mode window and its OpenGL context
  GLFWwindow * window = glfwCreateWindow( 800, 600, AppName.c_str(), nullptr, nullptr );
  if ( !window )
  {
    std::println( "Failed to create GLFW window" );
    glfwTerminate();
    return -1;
  }

  std::println( "GLFW window created successfully!" );

  // Helper function example
  int resultadd = add( 5, 3 );
  std::println( "Helper add function result: 5 + 3 = {}", resultadd );

  // Helper function example
  int resultmin = minus( 5, 3 );
  std::println( "Test minus function result: 5 - 3 = {}", resultmin );

  // GLM Example: Demonstrate vector and matrix operations
  std::println( "\n=== GLM Math Library Example ===" );

  // Vector operations
  glm::vec3 position( 1.0f, 2.0f, 3.0f );
  glm::vec3 direction( 0.0f, 1.0f, 0.0f );
  glm::vec3 up( 0.0f, 0.0f, 1.0f );

  std::println( "Position vector: ({}, {}, {})", position.x, position.y, position.z );
  std::println( "Direction vector: ({}, {}, {})", direction.x, direction.y, direction.z );

  // Vector math
  glm::vec3 result = position + direction * 2.0f;
  std::println( "Position + Direction * 2: ({}, {}, {})", result.x, result.y, result.z );

  float length = glm::length( position );
  std::println( "Length of position vector: {}", length );

  // Matrix operations
  glm::mat4 model      = glm::mat4( 1.0f );  // Identity matrix
  glm::mat4 view       = glm::lookAt( position, position + direction, up );
  glm::mat4 projection = glm::perspective( glm::radians( 45.0f ), 800.0f / 600.0f, 0.1f, 100.0f );

  std::println( "\nModel matrix (identity):" );
  for ( int i = 0; i < 4; ++i )
  {
    std::println( "  [{}, {}, {}, {}]", model[i][0], model[i][1], model[i][2], model[i][3] );
  }

  // Matrix multiplication
  glm::mat4 mvp = projection * view * model;
  std::println( "\nMVP matrix (Projection * View * Model):" );
  for ( int i = 0; i < 4; ++i )
  {
    std::println( "  [{}, {}, {}, {}]", mvp[i][0], mvp[i][1], mvp[i][2], mvp[i][3] );
  }

  // Transform a point
  glm::vec4 point( 1.0f, 0.0f, 0.0f, 1.0f );  // Homogeneous coordinates
  glm::vec4 transformed = mvp * point;
  std::println( "\nTransformed point (1,0,0,1): ({}, {}, {}, {})", transformed.x, transformed.y, transformed.z, transformed.w );

  std::println( "=== End GLM Example ===\n" );

  try
  {
    // initialize the vk::ApplicationInfo structure
    vk::ApplicationInfo applicationInfo( AppName.c_str(), 1, EngineName.c_str(), 1, VK_API_VERSION_1_1 );

    // initialize the vk::InstanceCreateInfo
    vk::InstanceCreateInfo instanceCreateInfo( {}, &applicationInfo );

    // create an Instance
    vk::Instance instance = vk::createInstance( instanceCreateInfo );

    // Main loop
    while ( !glfwWindowShouldClose( window ) )
    {
      glfwPollEvents();
      // Rendering and Vulkan code would go here
    }

    // destroy it again
    instance.destroy();
  }
  catch ( vk::SystemError & err )
  {
    std::println( "vk::SystemError: {}", err.what() );
    glfwDestroyWindow( window );
    glfwTerminate();
    exit( -1 );
  }
  catch ( std::exception & err )
  {
    std::println( "std::exception: {}", err.what() );
    glfwDestroyWindow( window );
    glfwTerminate();
    exit( -1 );
  }
  catch ( ... )
  {
    std::println( "unknown error" );
    glfwDestroyWindow( window );
    glfwTerminate();
    exit( -1 );
  }

  // Clean up GLFW resources
  glfwDestroyWindow( window );
  glfwTerminate();

  return 0;
}
