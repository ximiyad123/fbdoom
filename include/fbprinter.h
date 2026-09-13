#ifndef FBPRINTER_H
#define FBPRINTER_H

#include <stdint.h>
#include <stddef.h>


/*
 * Generic object positioning modes.
 */
typedef enum {
    FBPRINTER_POSITION_EXACT = 0,
    FBPRINTER_POSITION_TOP_LEFT,
    FBPRINTER_POSITION_TOP_RIGHT,
    FBPRINTER_POSITION_BOTTOM_LEFT,
    FBPRINTER_POSITION_BOTTOM_RIGHT,
    FBPRINTER_POSITION_CENTER
} FBPrinterPosition;


/*
 * Main fbprinter configuration.
 */
typedef struct FBPrinterConfig {

    /*
     * Physical framebuffer.
     */
    uintptr_t address;

    uint32_t width;
    uint32_t height;


    /*
     * Text rendering.
     */
    int text_size;

    int text_x;
    int text_y;

    FBPrinterPosition text_position;


    /*
     * Image rendering.
     */
    int img_size;

    int image_x;
    int image_y;

    FBPrinterPosition image_position;


    /*
     * Preserve framebuffer contents that are not
     * overwritten by the renderer.
     */
    int keep_leftover;


    /*
     * Input file used by the high-level renderer.
     */
    char *input_file;


    /*
     * Internal framebuffer state.
     *
     * Applications should not modify these directly.
     */
    int mem_fd;

    void *mapped_base;

    uint32_t *fb_ptr;

    size_t map_size;

} FBPrinterConfig;


/*
 * =========================================================
 * Configuration
 * =========================================================
 */

void fbprinter_config_init(
    FBPrinterConfig *config
);


/*
 * =========================================================
 * Framebuffer
 * =========================================================
 */

int fbprinter_open(
    FBPrinterConfig *config
);

void fbprinter_close(
    FBPrinterConfig *config
);

void fbprinter_clear(
    FBPrinterConfig *config
);


/*
 * =========================================================
 * Pixel operations
 * =========================================================
 */

void fbprinter_put_pixel(
    FBPrinterConfig *config,
    int x,
    int y,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a
);


/*
 * =========================================================
 * Generic buffer drawing
 * =========================================================
 *
 * pixels:
 *
 *     0xAARRGGBB
 *
 * width/height:
 *
 *     Dimensions of the source buffer.
 *
 * x/y:
 *
 *     Destination top-left coordinate.
 */
int fbprinter_draw_buffer(
    FBPrinterConfig *config,
    const uint32_t *pixels,
    uint32_t width,
    uint32_t height,
    int x,
    int y
);


/*
 * Draw a buffer using one of the generic positioning modes.
 *
 * For FBPRINTER_POSITION_EXACT:
 *
 *     x/y are used directly.
 *
 * For the other modes:
 *
 *     x/y are ignored.
 */
int fbprinter_draw_buffer_positioned(
    FBPrinterConfig *config,
    const uint32_t *pixels,
    uint32_t width,
    uint32_t height,
    FBPrinterPosition position,
    int x,
    int y
);


/*
 * =========================================================
 * Position calculation
 * =========================================================
 *
 * Calculates where an object should start inside the
 * framebuffer.
 *
 * object_width/object_height:
 *
 *     Size of the object being positioned.
 *
 * position:
 *
 *     Position mode.
 *
 * exact_x/exact_y:
 *
 *     Used only for FBPRINTER_POSITION_EXACT.
 */
void fbprinter_calculate_position(
    const FBPrinterConfig *config,
    uint32_t object_width,
    uint32_t object_height,
    FBPrinterPosition position,
    int exact_x,
    int exact_y,
    int *out_x,
    int *out_y
);


/*
 * =========================================================
 * Text positioning
 * =========================================================
 */

void fbprinter_set_text_position(
    FBPrinterConfig *config,
    int x,
    int y
);

void fbprinter_set_text_position_mode(
    FBPrinterConfig *config,
    FBPrinterPosition position
);

void fbprinter_get_text_position(
    const FBPrinterConfig *config,
    uint32_t object_width,
    uint32_t object_height,
    int *x,
    int *y
);


/*
 * =========================================================
 * Image positioning
 * =========================================================
 */

void fbprinter_set_image_position(
    FBPrinterConfig *config,
    int x,
    int y
);

void fbprinter_set_image_position_mode(
    FBPrinterConfig *config,
    FBPrinterPosition position
);

void fbprinter_get_image_position(
    const FBPrinterConfig *config,
    uint32_t object_width,
    uint32_t object_height,
    int *x,
    int *y
);


/*
 * =========================================================
 * High-level rendering
 * =========================================================
 */

int fbprinter_render(
    FBPrinterConfig *config,
    const char *filename
);


/*
 * =========================================================
 * Scaling
 * =========================================================
 */

void fbprinter_set_text_size(
    FBPrinterConfig *config,
    int size
);

void fbprinter_set_image_size(
    FBPrinterConfig *config,
    int size
);


/*
 * =========================================================
 * Framebuffer preservation
 * =========================================================
 */

void fbprinter_set_keep_leftover(
    FBPrinterConfig *config,
    int enabled
);


/*
 * =========================================================
 * Position string conversion
 * =========================================================
 *
 * Useful for generic configuration files and CLIs.
 */

const char *fbprinter_position_to_string(
    FBPrinterPosition position
);

FBPrinterPosition fbprinter_position_from_string(
    const char *value
);

#endif
