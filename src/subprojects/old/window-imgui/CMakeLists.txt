cmake_minimum_required(VERSION 4.1.1)

set(TARGET_NAME window-imgui)

add_executable(${TARGET_NAME}
    main.cpp
)

# --- Shader compilation (GLSL -> SPIR-V via glslc)
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin bin)

set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(GLOB TRIANGLE_SHADER_SOURCES
    ${SHADER_SRC_DIR}/*.vert
    ${SHADER_SRC_DIR}/*.frag
)

set(COMPILED_SHADERS)
foreach(SHADER ${TRIANGLE_SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(OUTPUT_SPV ${SHADER_OUT_DIR}/${SHADER_NAME}.spv)
    list(APPEND COMPILED_SHADERS ${OUTPUT_SPV})
    add_custom_command(
        OUTPUT ${OUTPUT_SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
        COMMAND ${GLSLC_EXECUTABLE} -o ${OUTPUT_SPV} ${SHADER}
        DEPENDS ${SHADER}
        COMMENT "Compiling GLSL shader ${SHADER_NAME} -> ${OUTPUT_SPV}"
        VERBATIM
    )
endforeach()

if (TRIANGLE_SHADER_SOURCES AND GLSLC_EXECUTABLE)
    add_custom_target(${TARGET_NAME}_shaders DEPENDS ${COMPILED_SHADERS})
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_shaders)
    target_compile_definitions(${TARGET_NAME} PRIVATE SHADER_BIN_DIR="${SHADER_OUT_DIR}")
else()
    message(STATUS "Shaders or glslc not found; skipping shader compilation for ${TARGET_NAME}")
endif()

# Link against available libraries
# core is your local library; glfw, Vulkan::Vulkan, glm, imgui, and EnTT are provided/added at top-level

target_link_libraries(${TARGET_NAME}
    PRIVATE
        core
        # glfw
        # Vulkan::Vulkan
        # glm
        # imgui
        # EnTT::EnTT
)

# Include directories if needed
# target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
