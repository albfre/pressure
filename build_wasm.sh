#!/bin/bash

# Exit on error
set -e

source ~/emsdk/emsdk_env.sh

# Check if Emscripten is in the PATH
if !command -v emcc &> /dev/null
then
    echo "Emscripten not found."
    exit 1
fi

mkdir -p build_wasm
cd build_wasm

emcmake cmake ..
emmake make

# Copy the output files to the docs directory
cp pressure_optimization_wasm.js ../docs/
cp pressure_optimization_wasm.wasm ../docs/

echo "Build complete. Files copied to docs directory."