# VkC

A headless compute pipeline using Vulkan in C.

## Overview

**VkC** is designed for raw, low-level GPU compute on Unix-like systems. It emphasizes
transparency, portability, and simplicity—**no rendering**, **no windowing**, **no BS**. All GPU
work is performed via compute shaders.

## Dependencies

VkC relies on the following components:

- **POSIX** — For broad Unix compatibility.
- **Linux** — Target platform; tested on Arch Linux.
- **CMake** — Build configuration and project generation.
- **PThreads** — Enables multi-threaded CPU execution.
- **libc** — Standard C runtime utilities.
- **GLSL** — High-level language for writing compute shaders.
- **SPIR-V** — Intermediate binary format for GPU execution.
- **Vulkan** — Cross-platform GPU API focused on explicit compute.

## Installation

### Debian/Ubuntu

```sh
apt update && apt install -y \
  gcc \
  build-essential \
  cmake \
  git \
  libvulkan-dev \
  glslang-tools \
  doxygen \
  libpcre2-dev \
  libpthread-stubs0-dev
```

### Arch Linux

```sh
pacman -Syu
pacman -S --noconfirm \
  vulkan-headers \
  vulkan-validation-layers \
  vulkan-extra-layers \
  vulkan-tools \
  vulkan-extra-tools \
  vulkan-icd-loader \
  vulkan-utility-libraries \
  renderdoc \
  spirv-headers \
  spirv-tools \
  glslang
```

_**NOTE:** See [Vulkan](https://wiki.archlinux.org/title/Vulkan) for more information._

## Cloning

This project includes `dsa` as a submodule for data structures and algorithms.

### 1. Clone the repository

```sh
git clone https://github.com/teleprint-me/vk.c vkc
cd vkc
```

### 2. Initialize submodules

```sh
git submodule update --init --recursive --remote
```

## Building

### 1. Documentation (`doxy.sh`)

Generates HTML documentation using Doxygen.

```sh
chmod +x doxy.sh && ./doxy.sh
```

- Uses `doxy.conf` for configuration.
- Outputs to `docs/html/`.
- Scans headers in `include/`.

### 2. Binaries & Shaders (`build.sh`)

Compiles all binaries and SPIR-V shaders.

```sh
chmod +x build.sh && ./build.sh
```

- Creates `build/` with:

  - `shaders/` – SPIR-V output.
  - `examples/` – Test drivers.

- Builds:

  - Vulkan and CPU-based examples.
  - GLSL compute shaders.

### 3. Debug Execution (`vk.sh`)

Runs the Vulkan binary with AddressSanitizer (ASAN) instrumentation.

```sh
chmod +x vk.sh
./vk.sh build/examples/vk
```

- Applies suppression rules from `asan.supp`.
- Configures LSAN and ASAN environment variables.
- Launches the Vulkan binary using precompiled shaders.

### 4. Unit Testing

Run tests via CTest:

```sh
ctest --rerun-failed --output-on-failure --test-dir build
```

- Test sources are in `tests/`.
- Outputs go to `build/tests/`.
- Individual binaries can be run directly for targeted debugging.

## Usage

Built binaries are located in `build/examples/`.

To run the Vulkan example:

- Use `./build/examples/vk` directly.
- Or run with `./vk.sh` to enable ASAN and other debug tools.

## Resources

### GPU & Driver Internals

- [DRM Overview](https://dri.freedesktop.org/wiki/DRM/)
- [Render Nodes](https://dri.freedesktop.org/docs/drm/gpu/drm-uapi.html#render-nodes)
- [Mesa GBM Overview](https://en.wikipedia.org/wiki/Mesa_%28computer_graphics%29)
  - [Mesa GBM Source](https://gitlab.freedesktop.org/mesa/mesa/-/tree/main/src/gbm)
- [Linux GPU Subsystem Docs](https://www.kernel.org/doc/html/latest/gpu/index.html)
  - [Device Management](https://www.kernel.org/doc/Documentation/admin-guide/devices.txt)

### Vulkan & SPIR-V

- [GLSL Specification](https://registry.khronos.org/OpenGL/#spec)
- [SPIR-V Specification](https://registry.khronos.org/SPIR-V/#spec)
- [Vulkan Specification](https://registry.khronos.org/vulkan/#apispecs)

## License

This project is licensed under the **GNU Affero General Public License (AGPL)** to promote
transparency, user freedom, and educational value.
