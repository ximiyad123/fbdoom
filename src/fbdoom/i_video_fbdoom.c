#include "i_video_fbdoom.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fbprinter.h"

#define FBDOOM_GAME_WIDTH  320
#define FBDOOM_GAME_HEIGHT 200

static FBPrinterConfig fbprinter;

static uint32_t *output_buffer = NULL;

static unsigned int display_width;
static unsigned int display_height;
static unsigned int scale;

static unsigned int output_width;
static unsigned int output_height;


/*
 * Initialize the FBDOOM video backend.
 */
int FBDoom_VideoInit(
    uintptr_t address,
    unsigned int width,
    unsigned int height,
    unsigned int requested_scale
)
{
    if (width == 0 || height == 0)
        return -1;

    if (requested_scale == 0)
        requested_scale = 1;

    display_width = width;
    display_height = height;
    scale = requested_scale;

    output_width = FBDOOM_GAME_WIDTH * scale;
    output_height = FBDOOM_GAME_HEIGHT * scale;

    output_buffer = malloc(
        (size_t)output_width *
        (size_t)output_height *
        sizeof(*output_buffer)
    );

    if (output_buffer == NULL)
        return -1;

    fbprinter_config_init(&fbprinter);

    fbprinter.address = address;
    fbprinter.width = display_width;
    fbprinter.height = display_height;

    if (fbprinter_open(&fbprinter) != 0) {
        free(output_buffer);
        output_buffer = NULL;
        return -1;
    }

    return 0;
}


/*
 * Match the behavior of the fbprinter CLI:
 *
 *   default:
 *       clear the framebuffer before drawing
 *
 *   --keep-leftover:
 *       preserve the existing framebuffer contents
 */
void FBDoom_VideoSetKeepLeftover(int enabled)
{
    fbprinter_set_keep_leftover(
        &fbprinter,
        enabled
    );

    if (!enabled)
        fbprinter_clear(&fbprinter);
}


/*
 * Shut down the video backend.
 */
void FBDoom_VideoShutdown(void)
{
    fbprinter_close(&fbprinter);

    free(output_buffer);
    output_buffer = NULL;
}


/*
 * Render a 320×200 indexed Doom frame.
 *
 * source:
 *     320×200 palette indices
 *
 * palette:
 *     256 entries of 0xAARRGGBB
 */
void FBDoom_VideoPresent(
    const uint8_t *source,
    const uint32_t *palette
)
{
    unsigned int y;
    unsigned int x;

    if (output_buffer == NULL ||
        source == NULL ||
        palette == NULL)
        return;

    /*
     * Convert the Doom indexed framebuffer into an
     * ARGB framebuffer while applying the requested scale.
     */
    for (y = 0; y < FBDOOM_GAME_HEIGHT; y++) {
        for (x = 0; x < FBDOOM_GAME_WIDTH; x++) {
            uint8_t index;
            uint32_t color;
            unsigned int sy;
            unsigned int sx;

            index = source[
                y * FBDOOM_GAME_WIDTH + x
            ];

            color = palette[index];

            for (sy = 0; sy < scale; sy++) {
                uint32_t *dst =
                    output_buffer +
                    (size_t)(y * scale + sy) *
                    output_width +
                    x * scale;

                for (sx = 0; sx < scale; sx++)
                    dst[sx] = color;
            }
        }
    }

    /*
     * Center the Doom image on the physical display.
     */
    fbprinter_draw_buffer_positioned(
        &fbprinter,
        output_buffer,
        output_width,
        output_height,
        FBPRINTER_POSITION_CENTER,
        0,
        0
    );
}
