## Build & Run Setup
To clone this repository and all submodules recursively, use:
```git clone --recurse-submodules https://github.com/BigJumble/MyVulkanDemos.git```

### If you already cloned without submodules, run:
```git submodule update --init --recursive```

### Prerequisites

- **Vulkan SDK 1.4.328.1 or newer required**  
  (Older versions will not compile. [Download here](https://vulkan.lunarg.com/sdk/home))

- **Linux:**  
  - Install `gcc-14` and `g++-14`
- **Windows / macOS:**  
  - (TODO: Specify recommended compilers for these platforms)

### What worked for me on Ubuntu 24.04

`
sudo apt-get install build-essential gcc-14 c++-14
`

`
sudo apt-get install libxinerama-dev libxi-dev libxcursor-dev
`

From https://vulkan.lunarg.com/doc/view/1.4.328.1/linux/getting_started.html

`
sudo apt-get install libglm-dev cmake libxcb-dri3-0 libxcb-present0 libpciaccess0 \
libpng-dev libxcb-keysyms1-dev libxcb-dri3-dev libx11-dev g++ gcc \
libwayland-dev libxrandr-dev libxcb-randr0-dev libxcb-ewmh-dev \
git python-is-python3 bison libx11-xcb-dev liblz4-dev libzstd-dev \
ocaml-core ninja-build pkg-config libxml2-dev wayland-protocols python3-jsonschema \
clang-format qtbase5-dev qt6-base-dev
`

### VSCode/Cursor Extensions

- CMake Tools
- C/C++ 

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

### Bazzite, immutable linux

- create distrobox with fedora:41
- install cursor/vs code there
- distrobox-export --app "ur ide"
- in that ide since it runs in a podman container also add this (for some reason Vulkan didn't get full forwarding 💀):
- ```export VK_ICD_FILENAMES=/run/host/usr/share/vulkan/icd.d/nvidia_icd.x86_64.json```
- In /core cmake file select between X11 or Wayland
- Note: I've tried dev containers and died inside 💀💀💀

## TODO

- [x] Raster triangle example
- [x] Resisable window
- [x] Integrate ImGui
- [x] Refactor to use Vulkan 1.4 features for less bloat
- [x] Implement SPIRV-reflect for shader descriptor set layouts
- [x] shaderc - compile shaders realtime and hotreload them on modify
- [x] Implement Vulkan Memory Allocator (VMA)
- [x] Implement JSON parser for saved state
- [x] Implement Lua for realtime code
- [x] Make each subproject select its instance and device extentions
- [x] Port Shaderc shader compilation flags
- [ ] Build abstraction for resource creation with VMA and SPIR-V Reflect (maybe someday)
- [ ] Model view projection matrix, camera controller
- [ ] EPIC: create a shadertoy mini engine, that handles shader creation, resource assignment and light scripting with lua
- [ ] Use EnTT for model/entity management
- [ ] Implement compute-based Game of Life demo
- [ ] Implement ray tracing demo
- [ ] Create cloud renderer using ray tracing
- [ ] Implement Verlet integration sphere physics on compute
- [ ] Render sphere terrain using distance fields or marching cubes
- [ ] Add OpenXR demo


