#!/bin/bash
# TrussC Web Build Script (macOS)
# Uses CMake presets for consistent build configuration

cd "$(dirname "$0")"

# Setup Emscripten environment
if [ -z "$EMSDK" ]; then
    # Only warn if using auto-detected toolchain
    # echo "Warning: EMSDK environment variable is not set."
    :
else
    source "$EMSDK/emsdk_env.sh"
fi

# Configure and build using CMake presets
cmake --preset web || exit 1
cmake --build --preset web --parallel || exit 1

echo ""
echo "Build complete! Output files are in bin/"
echo "To test locally:"
echo "  cd bin && python3 -m http.server 8080"
echo "  Open http://localhost:8080/trussBuster.html"
