#!/bin/sh

set -eu

IMAGE="fbprinter-builder"

echo "========================================"
echo " fbprinter setup"
echo "========================================"

# Prefer Podman, fall back to Docker
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

echo "Container runtime: $CONTAINER_RUNTIME"
echo

echo "Updating Git submodules..."
echo

git submodule update --init --recursive

echo
echo "Submodules ready."
echo

echo "Fixing Chocolate Doom startup..."
echo

MAIN_C="src/chocolate-doom/src/i_main.c"

if [ ! -f "$MAIN_C" ]; then
    echo "Error: $MAIN_C was not found."
    exit 1
fi

if grep -q 'M_SetExeDir();' "$MAIN_C"; then
    echo "M_SetExeDir() fix already present."
else
    if ! grep -q 'free(doom_argv);' "$MAIN_C"; then
        echo "Error: could not find free(doom_argv); in $MAIN_C"
        echo
        echo "Refusing to modify the file."
        exit 1
    fi

    sed -i '/^[[:space:]]*free(doom_argv);[[:space:]]*$/a\
\
    M_SetExeDir();
' "$MAIN_C"

    echo "M_SetExeDir() fix applied."
fi

echo
echo "Building existing Dockerfile..."
echo

# IMPORTANT:
# This script NEVER creates or modifies Dockerfile.
"$CONTAINER_RUNTIME" build \
    -t "$IMAGE" \
    -f Dockerfile \
    .

echo
echo "========================================"
echo " Setup complete"
echo "========================================"
echo "Image: $IMAGE"
