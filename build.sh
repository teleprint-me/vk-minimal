#!/usr/bin/env bash

set -euo pipefail

BUILD_PATH="build"
BUILD_TYPE="${1:-Debug}"
SHADER_DIR="shaders"
SHADER_OUT_DIR="${BUILD_PATH}/shaders"
SHADERS=("sum.comp")

# Clean previous build
echo "Cleaning previous build..."
rm -rf "${BUILD_PATH}"

# Configure project
echo "Configuring project (build type: ${BUILD_TYPE})..."
cmake -B "${BUILD_PATH}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

# Build project
echo "Building project..."
cmake --build "${BUILD_PATH}" --config "${BUILD_TYPE}" -j"$(nproc)"

# Compile shaders
echo "Compiling shaders..."
mkdir -p "${SHADER_OUT_DIR}"
for shader in "${SHADERS[@]}"; do
    input="${SHADER_DIR}/${shader}"
    output="${SHADER_OUT_DIR}/${shader%.comp}.spv"
    if glslangValidator -V "${input}" -o "${output}"; then
        echo "Compiled ${shader} -> ${output}"
    else
        echo "Failed to compile ${shader}" >&2
        exit 1
    fi
done

echo "Build and shader compilation completed successfully."
