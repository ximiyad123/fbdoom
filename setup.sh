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
