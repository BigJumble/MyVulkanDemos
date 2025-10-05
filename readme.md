## Build & Run Setup

### Prerequisites

- **Linux:**  
  - Install `gcc-14` and `g++-14`
- **Windows / macOS:**  
  - (TODO: Specify recommended compilers for these platforms)

### VSCode Extensions

- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
- [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)

### Configure CMake Tools

1. Set CMake Tools to use `gcc-14`:
   - Press <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>P</kbd>
   - Select `CMake: Select a Kit`
   - Choose the kit corresponding to `gcc-14`
2. Build: Press <kbd>F7</kbd>
3. Run: Press <kbd>F5</kbd>

#### Debugging Setup

- To enable <kbd>F5</kbd> (Run/Debug):
  1. Go to the **Run and Debug** tab in VSCode.
  2. Create a `launch.json` when prompted.
  3. Edit the `"program"` line to:
     ```json
     "program": "${workspaceFolder}/build/src/subprojects/{subproject-name}/{subproject-name}",
     "cwd": "${workspaceFolder}/build/src/subprojects/{subproject-name}",
     ```

---

## TODO

- [x] Raster triangle example
- [x] Resisable window
- [x] Integrate ImGui
- [x] Refactor to use Vulkan 1.4 features for less bloat
- [x] Implement SPIRV-reflect for shader descriptor set layouts
- [ ] shaderc - compile shaders realtime and hotreload them on modify
- [ ] Implement Vulkan Memory Allocator (VMA)
- [ ] Use EnTT for model/entity management
- [ ] Implement JSON parser for saved state
- [ ] Implement Lua for realtime code
- [ ] EPIC: create a shadertoy mini engine, that handles shader creation, resource assignment and light scripting with lua
- [x] Implement compute-based Game of Life demo
- [ ] Implement ray tracing demo
- [ ] Create cloud renderer using ray tracing
- [ ] Implement Verlet integration sphere physics on compute
- [ ] Render sphere terrain using distance fields or marching cubes