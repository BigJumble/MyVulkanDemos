cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME rttest)

add_executable(${TARGET_NAME}
    main.cpp
)

# --- Shader compilation (GLSL -> SPIR-V via glslc)
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin bin)

set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(GLOB RT_SHADER_SOURCES
    ${SHADER_SRC_DIR}/*.rgen
    ${SHADER_SRC_DIR}/*.rchit
    ${SHADER_SRC_DIR}/*.rmiss
)

set(COMPILED_SHADERS)
foreach(SHADER ${RT_SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(OUTPUT_SPV ${SHADER_OUT_DIR}/${SHADER_NAME}.spv)
    list(APPEND COMPILED_SHADERS ${OUTPUT_SPV})
    add_custom_command(
        OUTPUT ${OUTPUT_SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
        COMMAND ${GLSLC_EXECUTABLE} --target-env=vulkan1.2 -o ${OUTPUT_SPV} ${SHADER}
        DEPENDS ${SHADER}
        COMMENT "Compiling RT shader ${SHADER_NAME} -> ${OUTPUT_SPV}"
        VERBATIM
    )
endforeach()

if (RT_SHADER_SOURCES AND GLSLC_EXECUTABLE)
    add_custom_target(${TARGET_NAME}_shaders DEPENDS ${COMPILED_SHADERS})
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_shaders)
    target_compile_definitions(${TARGET_NAME} PRIVATE SHADER_BIN_DIR="${SHADER_OUT_DIR}")
else()
    message(STATUS "Shaders or glslc not found; skipping shader compilation for ${TARGET_NAME}")
endif()

# Link against available libraries
target_link_libraries(${TARGET_NAME}
    PRIVATE
        core
)

