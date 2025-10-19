#version 450

layout(location = 0) out vec3 vColor;

// Declare push constant structure and variable
layout(push_constant) uniform PushConstants {
    vec2 pos;
} pc;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

void main() {
    gl_Position = vec4(inPosition + pc.pos*0.3, 0.0, 1.0);
    vColor = inColor;
}
