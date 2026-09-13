#ifndef I_VIDEO_FBDOOM_H
#define I_VIDEO_FBDOOM_H

#include <stdint.h>

int FBDoom_VideoInit(
    uintptr_t address,
    unsigned int display_width,
    unsigned int display_height,
    unsigned int scale
);

void FBDoom_VideoSetKeepLeftover(int enabled);

void FBDoom_VideoShutdown(void);

void FBDoom_VideoPresent(
    const uint8_t *source,
    const uint32_t *palette
);

#endif
