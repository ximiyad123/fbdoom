# FBDOOM

Chocolate Doom based Doom port using fbprinter for direct display output.

## Display backend

FBDOOM does not use:

- DRM
- /dev/fb0
- X11
- Wayland

Rendering path:

    Doom renderer
        ↓
    640x480 software framebuffer
        ↓
    libfbprinter.so
        ↓
    /dev/mem
        ↓
    display memory

## Example

    fbdoom 0xb8000000,1220,2712 \
        --position=center \
        --game-resolution=640x480 \
        --wad=/usr/share/fbdoom/DOOM.wad
