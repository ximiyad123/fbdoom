#include "i_video_fbdoom.h"
#include "i_input_fbdoom.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"
#include "i_video.h"
#include "i_system.h"
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

static int initialized = 0;


/*
 * ============================================================
 * Low-level FBDOOM video backend
 * ============================================================
 *
 * Converts Doom's 320x200 indexed framebuffer into a scaled
 * 32-bit ARGB framebuffer and sends it to libfbprinter.
 */


/*
 * Initialize the low-level FBDOOM video backend.
 */
int FBDoom_VideoInit(
    uintptr_t address,
    unsigned int width,
    unsigned int height,
    unsigned int requested_scale
)
{
    size_t buffer_size;

    if (initialized)
        return 0;

    if (width == 0 || height == 0)
        return -1;

    if (requested_scale == 0)
        requested_scale = 1;

    display_width = width;
    display_height = height;
    scale = requested_scale;

    output_width =
        FBDOOM_GAME_WIDTH * scale;

    output_height =
        FBDOOM_GAME_HEIGHT * scale;

    buffer_size =
        (size_t)output_width *
        (size_t)output_height *
        sizeof(*output_buffer);

    output_buffer = malloc(buffer_size);

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

    initialized = 1;

    return 0;
}


/*
 * Control whether the existing physical framebuffer is preserved.
 */
void FBDoom_VideoSetKeepLeftover(int enabled)
{
    if (!initialized)
        return;

    fbprinter_set_keep_leftover(
        &fbprinter,
        enabled
    );

    if (!enabled)
        fbprinter_clear(&fbprinter);
}


/*
 * Shut down the low-level video backend.
 */
void FBDoom_VideoShutdown(void)
{
    if (!initialized)
        return;

    fbprinter_close(&fbprinter);

    free(output_buffer);
    output_buffer = NULL;

    initialized = 0;
}


/*
 * Render one Doom frame.
 *
 * source:
 *     320x200 8-bit palette indices.
 *
 * palette:
 *     256 entries in 0xAARRGGBB format.
 */
void FBDoom_VideoPresent(
    const uint8_t *source,
    const uint32_t *palette
)
{
    unsigned int x;
    unsigned int y;

    if (!initialized)
        return;

    if (output_buffer == NULL)
        return;

    if (source == NULL)
        return;

    if (palette == NULL)
        return;


    /*
     * Convert the indexed Doom framebuffer into
     * a scaled 32-bit framebuffer.
     */
    for (y = 0; y < FBDOOM_GAME_HEIGHT; y++) {

        for (x = 0; x < FBDOOM_GAME_WIDTH; x++) {

            uint8_t index;
            uint32_t color;

            unsigned int sx;
            unsigned int sy;

            index =
                source[
                    y * FBDOOM_GAME_WIDTH + x
                ];

            color =
                palette[index];


            /*
             * Expand one Doom pixel into
             * scale x scale physical pixels.
             */
            for (sy = 0; sy < scale; sy++) {

                uint32_t *dst =
                    output_buffer +
                    (size_t)(y * scale + sy) *
                    output_width +
                    (x * scale);

                for (sx = 0; sx < scale; sx++)
                    dst[sx] = color;
            }
        }
    }


    /*
     * Center the scaled Doom framebuffer
     * on the physical display.
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


/*
 * ============================================================
 * Chocolate Doom video interface
 * ============================================================
 *
 * Adapts the low-level FBDOOM renderer to Chocolate Doom's
 * I_VideoBuffer API.
 */


/*
 * Chocolate Doom's actual game framebuffer.
 *
 * 320x200, one byte per pixel.
 */
pixel_t *I_VideoBuffer = NULL;


/*
 * Chocolate Doom video globals.
 *
 * Keep these definitions here.
 *
 * vanilla_keyboard_mapping is NOT defined here because it is
 * provided by i_input_fbdoom.c.
 */
char *video_driver = NULL;

boolean screenvisible = true;

boolean screensaver_mode = false;

int usegamma = 0;

int screen_width = FBDOOM_GAME_WIDTH;
int screen_height = FBDOOM_GAME_HEIGHT;

int fullscreen = true;

int aspect_ratio_correct = false;

int integer_scaling = true;

int smooth_pixel_scaling = false;

int vga_porch_flash = false;

int force_software_renderer = true;

int png_screenshots = false;

char *window_position = NULL;

unsigned int joywait = 0;

int usemouse = 0;


/*
 * Chocolate Doom's palette converted to ARGB.
 */
static uint32_t palette32[256];


/*
 * FBDOOM configuration supplied by main.c.
 */
extern uintptr_t fbdoom_address;

extern unsigned int fbdoom_display_width;

extern unsigned int fbdoom_display_height;

extern unsigned int fbdoom_scale;

extern int fbdoom_keep_leftover;


/*
 * ============================================================
 * Graphics initialization
 * ============================================================
 */

void I_InitGraphics(void)
{
    size_t buffer_size;

    buffer_size =
        (size_t)FBDOOM_GAME_WIDTH *
        (size_t)FBDOOM_GAME_HEIGHT;

    I_VideoBuffer = malloc(buffer_size);

    if (I_VideoBuffer == NULL) {

        I_Error(
            "FBDOOM: unable to allocate "
            "320x200 video buffer"
        );

        return;
    }


    if (FBDoom_VideoInit(
            fbdoom_address,
            fbdoom_display_width,
            fbdoom_display_height,
            fbdoom_scale
        ) != 0) {

        free(I_VideoBuffer);

        I_VideoBuffer = NULL;

        I_Error(
            "FBDOOM: unable to initialize "
            "framebuffer renderer"
        );

        return;
    }


    FBDoom_VideoSetKeepLeftover(
        fbdoom_keep_leftover
    );


    /*
     * Initialize the physical input devices.
     */
    if (FBDoom_InputInit() != 0) {

        FBDoom_VideoShutdown();

        free(I_VideoBuffer);

        I_VideoBuffer = NULL;

        I_Error(
            "FBDOOM: unable to initialize input"
        );

        return;
    }


    /*
     * Start with a black Doom framebuffer.
     */
    memset(
        I_VideoBuffer,
        0,
        buffer_size
    );
}


/*
 * FBDOOM handles its own video command-line options.
 */
void I_GraphicsCheckCommandLine(void)
{
}


/*
 * ============================================================
 * Shutdown
 * ============================================================
 */

void I_ShutdownGraphics(void)
{
    FBDoom_InputShutdown();

    FBDoom_VideoShutdown();

    free(I_VideoBuffer);

    I_VideoBuffer = NULL;
}


/*
 * ============================================================
 * Palette
 * ============================================================
 *
 * Chocolate Doom supplies 256 RGB triplets:
 *
 *     R G B
 *
 * FBDOOM uses:
 *
 *     0xAARRGGBB
 *
 * with full opacity.
 */

void I_SetPalette(byte *palette)
{
    unsigned int i;

    if (palette == NULL)
        return;


    for (i = 0; i < 256; i++) {

        uint32_t r;
        uint32_t g;
        uint32_t b;

        r = palette[i * 3 + 0];
        g = palette[i * 3 + 1];
        b = palette[i * 3 + 2];

        palette32[i] =
            0xff000000u |
            (r << 16) |
            (g << 8) |
            b;
    }
}


/*
 * Return the closest palette entry to RGB.
 */
int I_GetPaletteIndex(
    int r,
    int g,
    int b
)
{
    unsigned int i;

    unsigned int best_index = 0;
    uint32_t best_distance = UINT32_MAX;


    for (i = 0; i < 256; i++) {

        int pr;
        int pg;
        int pb;

        int dr;
        int dg;
        int db;

        uint32_t distance;


        pr =
            (int)(
                (palette32[i] >> 16) &
                0xff
            );

        pg =
            (int)(
                (palette32[i] >> 8) &
                0xff
            );

        pb =
            (int)(
                palette32[i] &
                0xff
            );


        dr = r - pr;
        dg = g - pg;
        db = b - pb;


        distance =
            (uint32_t)(
                dr * dr +
                dg * dg +
                db * db
            );


        if (distance < best_distance) {

            best_distance = distance;
            best_index = i;
        }
    }


    return (int)best_index;
}


/*
 * ============================================================
 * Frame presentation
 * ============================================================
 */

void I_UpdateNoBlit(void)
{
}


void I_FinishUpdate(void)
{
    if (I_VideoBuffer == NULL)
        return;

    FBDoom_VideoPresent(
        I_VideoBuffer,
        palette32
    );
}


/*
 * ============================================================
 * Screen readback
 * ============================================================
 */

void I_ReadScreen(pixel_t *scr)
{
    if (scr == NULL)
        return;

    if (I_VideoBuffer == NULL)
        return;

    memcpy(
        scr,
        I_VideoBuffer,
        (size_t)FBDOOM_GAME_WIDTH *
        (size_t)FBDOOM_GAME_HEIGHT
    );
}


/*
 * ============================================================
 * Miscellaneous video functions
 * ============================================================
 */

void I_BeginRead(void)
{
}


void I_SetWindowTitle(
    const char *title
)
{
    (void)title;
}


void I_SetGrabMouseCallback(
    grabmouse_callback_t func
)
{
    (void)func;
}


void I_DisplayFPSDots(
    boolean dots_on
)
{
    (void)dots_on;
}


void I_BindVideoVariables(void)
{
}


void I_InitWindowTitle(void)
{
}


void I_RegisterWindowIcon(
    const unsigned int *icon,
    int width,
    int height
)
{
    (void)icon;
    (void)width;
    (void)height;
}


void I_InitWindowIcon(void)
{
}


void I_StartFrame(void)
{
}


void I_StartTic(void)
{
    FBDoom_InputPoll();
}


void I_EnableLoadingDisk(
    int xoffs,
    int yoffs
)
{
    (void)xoffs;
    (void)yoffs;
}


void I_GetWindowPosition(
    int *x,
    int *y,
    int w,
    int h
)
{
    (void)w;
    (void)h;

    if (x != NULL)
        *x = 0;

    if (y != NULL)
        *y = 0;
}
