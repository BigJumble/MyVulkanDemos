#version 450

layout(location = 0) out vec2 vUV;

void main() {
    // gl_VertexIndex for strip: 0:(-1,-1), 1:(1,-1), 2:(-1,1), 3:(1,1)
    vec2 pos = vec2((gl_VertexIndex & 1) * 2 - 1, ((gl_VertexIndex >> 1) & 1) * 2 - 1);
    vUV = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
