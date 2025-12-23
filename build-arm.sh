#!/bin/bash
# Gemini Browser ARM webOS Cross-Compilation Build Script

set -e

echo "=== Gemini Browser ARM webOS Build ==="
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LINARO_GCC="${SCRIPT_DIR}/toolchains/gcc-linaro"

# Check prerequisites
echo "Checking prerequisites..."

if [ ! -d "${LINARO_GCC}" ]; then
    echo "ERROR: Linaro toolchain not found at ${LINARO_GCC}"
    echo ""
    echo "To set up the toolchain:"
    echo "  mkdir -p toolchains && cd toolchains"
    echo "  wget https://releases.linaro.org/components/toolchain/binaries/4.8-2015.06/arm-linux-gnueabi/gcc-linaro-4.8-2015.06-x86_64_arm-linux-gnueabi.tar.xz"
    echo "  tar xf gcc-linaro-4.8-2015.06-x86_64_arm-linux-gnueabi.tar.xz"
    echo "  mv gcc-linaro-4.8-2015.06-x86_64_arm-linux-gnueabi gcc-linaro"
    exit 1
fi

if [ ! -d "/opt/PalmPDK" ]; then
    echo "ERROR: PalmPDK not found at /opt/PalmPDK"
    exit 1
fi

echo "  Linaro GCC: ${LINARO_GCC}"
echo "  PalmPDK: /opt/PalmPDK"
echo ""

# Build
cd "${SCRIPT_DIR}"
make clean
make -j$(nproc)

# Verify the build
echo ""
echo "Build verification:"
file gemini
echo ""
echo "Library dependencies:"
readelf -d gemini | grep NEEDED | head -10
echo ""

echo "Build complete!"
echo "Binary: ${SCRIPT_DIR}/gemini"

# Package if requested
if [ "$1" == "--package" ]; then
    echo ""
    echo "=== Packaging for webOS ==="

    # Copy binary to package directory
    cp gemini "${SCRIPT_DIR}/package/"

    # Create IPK
    if command -v palm-package &> /dev/null; then
        cd "${SCRIPT_DIR}"
        palm-package package
        echo ""
        echo "Package created: $(ls -1 *.ipk 2>/dev/null | tail -1)"
    else
        echo "ERROR: palm-package not found"
        echo "Install the Palm SDK or use 'palm-package package' manually"
        exit 1
    fi
fi

# Install if requested
if [ "$1" == "--install" ] || [ "$2" == "--install" ]; then
    echo ""
    echo "=== Installing to device ==="

    IPK=$(ls -1 *.ipk 2>/dev/null | tail -1)
    if [ -z "$IPK" ]; then
        echo "No IPK found. Run with --package first."
        exit 1
    fi

    if command -v palm-install &> /dev/null; then
        palm-install "$IPK"
        echo "Installed: $IPK"
    else
        echo "ERROR: palm-install not found"
        exit 1
    fi
fi

echo ""
echo "Done!"
