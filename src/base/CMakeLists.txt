# Base library
file(GLOB BASE_SOURCES "*.cpp")
add_library(base STATIC
    ${BASE_SOURCES}
)
target_link_libraries(base PUBLIC glfw Vulkan::Vulkan glm EnTT::EnTT)

target_include_directories(base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
