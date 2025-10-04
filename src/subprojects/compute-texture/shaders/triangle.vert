#version 450

layout(location = 0) out vec2 vUV;

// Fullscreen triangle strip quad (2 triangles, 4 vertices)
vec2 positions[4] = vec2[](
    vec2(-1.0, -1.0), // bottom left
    vec2( 1.0, -1.0), // bottom right
    vec2(-1.0,  1.0), // top left
    vec2( 1.0,  1.0)  // top right
);

// UV coordinates for texture sampling
vec2 uvs[4] = vec2[](
    vec2(0.0, 1.0), // bottom left
    vec2(1.0, 1.0), // bottom right
    vec2(0.0, 0.0), // top left
    vec2(1.0, 0.0)  // top right
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vUV = uvs[gl_VertexIndex];
}
