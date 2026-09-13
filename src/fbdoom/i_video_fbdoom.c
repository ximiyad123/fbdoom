#include "i_video_fbdoom.h"

#include <stdint.h>
#include <stdlib.h>

#include "fbprinter.h"

static FBPrinterConfig fbprinter;

static uint32_t *screen_buffer;
static unsigned int screen_width;
static unsigned int screen_height;

int FBDoom_VideoInit(
    unsigned long address,
    unsigned int display_width,
    unsigned int display_height,
    unsigned int game_width,
    unsigned int game_height
)
{
    screen_width = game_width;
    screen_height = game_height;

    fbprinter_config_init(&fbprinter);

    fbprinter.address = (uintptr_t)address;
    fbprinter.width = display_width;
    fbprinter.height = display_height;

    screen_buffer =
        malloc((size_t)screen_width *
               (size_t)screen_height *
               sizeof(uint32_t));

    if (screen_buffer == NULL)
        return -1;

    if (fbprinter_open(&fbprinter) != 0) {
        free(screen_buffer);
        screen_buffer = NULL;
        return -1;
    }

    return 0;
}

void FBDoom_VideoShutdown(void)
{
    fbprinter_close(&fbprinter);

    free(screen_buffer);
    screen_buffer = NULL;
}

void FBDoom_VideoPresent(void)
{
    if (screen_buffer == NULL)
        return;

    fbprinter_draw_buffer_positioned(
        &fbprinter,
        screen_buffer,
        screen_width,
        screen_height,
        FBPRINTER_POSITION_CENTER,
        0,
        0
    );
}
