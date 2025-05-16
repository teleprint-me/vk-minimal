# VkMinimal

A minimal headless compute pipeline using Vulkan in C.

## Dependencies

This project is designed for raw, low-level compute on Unix-like systems, emphasizing simplicity and
transparency:

- **POSIX** — For broad compatibility across Unix systems.
- **Linux** — Target environment; tested on Arch Linux.
- **CMake** — For project configuration and build management.
- **PThreads** — Enables multi-threaded execution.
- **libc** — Standard C library for system-level utilities.
- **GLSL** — High-level shading language used to write shaders.
- **SPIR-V** — Portable shader intermediate representation.
- **Vulkan** — Cross-platform GPU API for compute workloads.

All GPU work is done via compute shaders—no windowing, no rendering, no BS.

## Platform Support

Other Linux distributions may work but are untested.

## Build

### 1. `doxy.sh`

Automates compilation of HTML documentation.

- Applies custom configurations via `doxy.conf`.
- Creates a `docs/html` directory.
- Recursively generates documentation from headers in `include/`.

```sh
chmod +x doxy.sh && ./doxy.sh
```

### 2. `build.sh`

Automates compilation of binaries and SPIR-V shaders.

- Creates a `build/` directory with subdirectories:

  - `shaders/` for SPIR-V binaries.
  - `examples/` for test drivers.

- Builds:

  - CPU and Vulkan examples.
  - GLSL shaders (compiled to SPIR-V).

```sh
chmod +x build.sh && ./build.sh
```

### 3. `vk.sh`

Runs the Vulkan example binary with AddressSanitizer (ASAN) enabled.

- Applies custom suppressions via `asan.supp`.
- Configures LSAN and ASAN environment variables.
- Launches the Vulkan example with precompiled shaders.

```sh
chmod +x vk.sh && ./vk.sh
```

## Usage

After building, binaries can be found in `build/examples/`.

Run the Vulkan example directly or use the provided `vk.sh` script for ASAN-instrumented runs.

## Resources

### GPU and Driver Internals

- [DRM Overview](https://dri.freedesktop.org/wiki/DRM/)
- [Render Nodes (DRM)](https://dri.freedesktop.org/docs/drm/gpu/drm-uapi.html#render-nodes)
- [Mesa GBM Overview](https://en.wikipedia.org/wiki/Mesa_%28computer_graphics%29)

  - [Mesa GBM Source](https://gitlab.freedesktop.org/mesa/mesa/-/tree/main/src/gbm)

- [Linux GPU Subsystem Docs](https://www.kernel.org/doc/html/latest/gpu/index.html)

  - [Device Management](https://www.kernel.org/doc/Documentation/admin-guide/devices.txt)

### Vulkan + SPIR-V Specs

- [GLSL Spec](https://registry.khronos.org/OpenGL/#spec)
- [SPIR-V Spec](https://registry.khronos.org/SPIR-V/#spec)
- [Vulkan Spec](https://registry.khronos.org/vulkan/#apispecs)

## License

This project is released under the **GNU Affero General Public License (AGPL)** to guarantee
end-user freedom, transparency, and educational value.
