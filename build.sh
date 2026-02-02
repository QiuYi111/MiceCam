#!/bin/bash
# Quick build script for MiceCam development

set -e

echo "🔨 Building MiceCam..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
echo "Compiling..."
make -j$(sysctl -n hw.ncpu)

# Run tests
echo "Running tests..."
./micecam_tests --gtest_color=yes

echo
echo "✓ Build complete!"
echo "Binary: ./build/micecam"
