#ifndef I_VIDEO_FBDOOM_H
#define I_VIDEO_FBDOOM_H

int FBDoom_VideoInit(
    unsigned long address,
    unsigned int display_width,
    unsigned int display_height,
    unsigned int game_width,
    unsigned int game_height
);

void FBDoom_VideoShutdown(void);

void FBDoom_VideoPresent(void);

#endif
