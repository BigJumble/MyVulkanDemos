// #include <iostream>
// #include <format>
#include <print>
#include <string>

int main()
{
  int         myVar    = 10;
  std::string myString = "Hello, World!";
  // Using C++20 std::format (more widely supported)
  // std::cout << std::format("Hello, World!\n");

  // Alternative C++23 features that might work:
  std::println( "Hello, World!" );  // C++23 print library
  std::println( "myVar: {}", std::to_string( myVar ) );
  std::println( "myString: {}", myString );
  std::println( "myString1: {}", myString );
  std::println( "myString2: {}", myString );
  // std::println( "myString2: {}", myString );
  if ( true )
  {
    std::println( "True" );
  }
  else
  {
    std::println( "False" );
  }

  return 0;
}
