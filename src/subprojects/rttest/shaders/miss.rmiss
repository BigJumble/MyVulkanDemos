#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitColor;

void main() {
    // Simple gradient sky
    vec3 skyColor = mix(
        vec3(0.5, 0.7, 1.0),  // Sky blue
        vec3(1.0, 1.0, 1.0),  // White horizon
        gl_WorldRayDirectionEXT.y * 0.5 + 0.5
    );
    hitColor = skyColor;
}

