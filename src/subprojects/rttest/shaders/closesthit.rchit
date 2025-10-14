#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitColor;
hitAttributeEXT vec2 attribs;

layout(set = 1, binding = 0) buffer Materials {
    vec4 data[];  // vec4(color.rgb, materialType)
} materials;

void main() {
    // Get material ID from instance custom index
    uint materialID = gl_InstanceCustomIndexEXT;
    vec4 materialData = materials.data[materialID];
    vec3 albedo = materialData.rgb;
    
    // Calculate barycentric coordinates for the hit triangle
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    
    // Calculate normal from barycentric coordinates and vertex normals
    // For now, use a simple face normal calculation
    vec3 worldNormal = normalize(gl_ObjectToWorldEXT * vec4(0.0, 0.0, 1.0, 0.0));
    
    // Simple directional light
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float ndotl = max(dot(worldNormal, lightDir), 0.0);
    
    // Ambient + diffuse
    vec3 ambient = vec3(0.1);
    vec3 diffuse = albedo * ndotl;
    
    hitColor = ambient + diffuse;
}

