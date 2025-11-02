#version 450

layout(location = 0) out vec3 vColor;

// Declare push constant structure with camera view and projection matrices
layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
} pc;

// Per-vertex attributes
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

// Per-instance attribute (3D position for this instance)
layout(location = 2) in vec3 instancePosition;

void main() {
    // Construct 3D position from 2D vertex position and instance position
    vec3 worldPos = vec3(inPosition.x, inPosition.y, 0.0) + instancePosition;
    
    // Apply view and projection transforms
    gl_Position = pc.proj * pc.view * vec4(worldPos, 1.0);
    vColor = inColor;
}
