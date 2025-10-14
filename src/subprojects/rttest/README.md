# RTTest - Minimal Ray Tracing Example

A minimal Vulkan ray tracing example demonstrating the core concepts of RT rendering.

## What It Does

Renders a single red triangle using ray tracing (VK_KHR_ray_tracing_pipeline). Shows a simple gradient sky when rays miss geometry.

## Key Components

### Acceleration Structures
- **BLAS (Bottom-Level)**: Contains the triangle geometry
- **TLAS (Top-Level)**: Scene structure referencing the BLAS

### Ray Tracing Pipeline
- **Ray Generation Shader** (`raygen.rgen`): Launches one ray per pixel from camera
- **Closest Hit Shader** (`closesthit.rchit`): Calculates color when ray hits triangle
- **Miss Shader** (`miss.rmiss`): Sky gradient when ray misses all geometry

### Shader Binding Table (SBT)
Maps ray types to shaders. In this example:
- Group 0: Ray generation
- Group 1: Hit group (closest hit shader)
- Group 2: Miss shader

## Differences from Rasterization

| Rasterization (synctest) | Ray Tracing (rttest) |
|-------------------------|----------------------|
| Vertex/Index buffers | Acceleration structures (BLAS/TLAS) |
| `cmd.draw()` | `cmd.traceRaysKHR()` |
| Vertex + Fragment shaders | RayGen + ClosestHit + Miss shaders |
| Framebuffer attachments | Storage image |
| Many draw calls | One trace call for entire scene |

## Architecture Highlights

### No Draw Calls!
```cpp
// Rasterization: One draw call per object
for (auto& obj : objects) {
    cmd.bindVertexBuffers(...);
    cmd.draw(...);
}

// Ray Tracing: One call for the entire scene!
cmd.traceRaysKHR(raygenRegion, missRegion, hitRegion, callableRegion, 
                 width, height, 1);
```

### Automatic Hidden Surface Removal
No depth buffer needed - BVH traversal automatically finds closest intersection.

### Natural Support for Reflections/Shadows
Just trace more rays from hit points (not shown in this minimal example).

## Building

```bash
cd build
cmake ..
make rttest
```

## Running

```bash
./src/subprojects/rttest/rttest
```

## Requirements

- Vulkan 1.2+
- GPU with ray tracing support (NVIDIA RTX, AMD RDNA 2+, or Intel Arc)
- VK_KHR_ray_tracing_pipeline extension
- VK_KHR_acceleration_structure extension

## Next Steps

To extend this example:
1. Add more geometry (multiple triangles, meshes)
2. Add recursive ray tracing for reflections
3. Implement shadow rays
4. Add textures and more complex materials
5. Implement denoising for path tracing

## Notes

This is a **minimal educational example**. Production ray tracers would:
- Use a proper scene graph
- Implement spatial data structures
- Add denoising
- Optimize BVH construction
- Use hybrid rendering (raster + RT)

