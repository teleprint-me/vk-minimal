# VkMinimal

A minimal compute pipeline for Vulkan in C.

## Dependencies

This implementation is designed to be minimal, headless, and Unix-focused, using GPU compute where
available:

- **Linux** — Primary development and testing environment.
- **POSIX** — Ensures compatibility across Unix-like systems.
- **libc** — Standard C runtime for basic I/O and memory management.
- **Vulkan**
- **SPIR-V**

The entire GPU pipeline runs via compute shaders, with no windowing or graphics
context—just raw computation.

## Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug -j$(nproc)
./build/main
```

## Resources

### Low-Level GPU Programming

- [DRM Overview](https://dri.freedesktop.org/wiki/DRM/)
- [Render Nodes](https://dri.freedesktop.org/docs/drm/gpu/drm-uapi.html#render-nodes)
- [Mesa GBM](https://en.wikipedia.org/wiki/Mesa_%28computer_graphics%29)

  - [Mesa GBM Source](https://gitlab.freedesktop.org/mesa/mesa/-/tree/main/src/gbm)

### Kernel and Driver Internals

- [Linux GPU Subsystem Documentation](https://www.kernel.org/doc/html/latest/gpu/index.html)
    - [Devices](https://www.kernel.org/doc/Documentation/admin-guide/devices.txt)

### Khronos Specifications

- [SPIR-V Specification](https://registry.khronos.org/SPIR-V/#spec)
- [Vulkan Specification](https://registry.khronos.org/vulkan/#apispecs)

## License

Licensed under the **GNU Affero General Public License (AGPL)** to ensure full freedom of use,
modification, and redistribution. Built for educational exploration, transparency, and long-term
adaptability.
