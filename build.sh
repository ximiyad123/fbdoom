#!/bin/sh

set -eu

IMAGE="fbprinter-builder"

# ------------------------------------------------------------
# Find container runtime
# ------------------------------------------------------------

if command -v podman >/dev/null 2>&1; then
    CONTAINER_RUNTIME="podman"
elif command -v docker >/dev/null 2>&1; then
    CONTAINER_RUNTIME="docker"
else
    echo "Error: neither Podman nor Docker was found."
    echo
    echo "Please install Podman or Docker first."
    exit 1
fi

# ------------------------------------------------------------
# Check ARCH
# ------------------------------------------------------------

if [ -z "${ARCH:-}" ]; then
    echo "Error: ARCH is not set."
    echo
    echo "Supported architectures:"
    echo "  arm64"
    echo "  arm"
    echo
    echo "Example:"
    echo "  export ARCH=arm64"
    exit 1
fi

case "$ARCH" in
    arm64|arm)
        ;;
    *)
        echo "Error: unsupported ARCH='$ARCH'"
        echo
        echo "Supported architectures:"
        echo "  arm64"
        echo "  arm"
        exit 1
        ;;
esac

# ------------------------------------------------------------
# Select target
# ------------------------------------------------------------

case "${1:-dynamic}" in
    dynamic)
        TARGET="dynamic"
        ;;

    static)
        TARGET="static"
        ;;

    clean)
        echo "========================================"
        echo " Cleaning FBDOOM"
        echo "========================================"
        echo

        make clean

        echo
        echo "Cleaning fbprinter..."
        ./src/fbprinter/build.sh clean

        echo
        echo "Cleaning Chocolate Doom..."
        make -C src/chocolate-doom clean

        echo
        echo "Clean complete."
        exit 0
        ;;

    *)
        echo "Usage: $0 [dynamic|static|clean]"
        exit 1
        ;;
esac

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

echo "========================================"
echo " FBDOOM build"
echo "========================================"
echo "Container    : $CONTAINER_RUNTIME"
echo "Image        : $IMAGE"
echo "Architecture : $ARCH"
echo "Target       : $TARGET"
echo "========================================"
echo

"$CONTAINER_RUNTIME" run --rm \
    -v "$PWD:/src:Z" \
    -w /src \
    "$IMAGE" \
    make ARCH="$ARCH" "$TARGET"

echo
echo "========================================"
echo " Build complete"
echo "========================================"
echo
