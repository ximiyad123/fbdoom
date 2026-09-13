#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i_video_fbdoom.h"

#define DEFAULT_SCALE 2

#define GAME_WIDTH  320
#define GAME_HEIGHT 200


typedef struct FBDoomOptions {

    uintptr_t address;

    unsigned int display_width;
    unsigned int display_height;

    unsigned int scale;

    int keep_leftover;

    const char *vol_up_event;
    const char *power_event;
    const char *vol_down_event;

    const char *wad;

} FBDoomOptions;


static void print_usage(const char *program)
{
    fprintf(
        stderr,

        "Usage:\n"
        "  %s ADDRESS,WIDTH,HEIGHT [options]\n"
        "\n"

        "Display:\n"
        "  --scale=N\n"
        "  --keep-leftover\n"
        "\n"

        "Input:\n"
        "  --vol-up-event=PATH\n"
        "  --power-event=PATH\n"
        "  --vol-down-event=PATH\n"
        "\n"

        "Game:\n"
        "  --wad=PATH\n"
        "\n"

        "Example:\n"
        "  %s 0xb8000000,1220,2712 \\\n"
        "      --scale=2 \\\n"
        "      --keep-leftover \\\n"
        "      --vol-up-event=/dev/input/event0 \\\n"
        "      --power-event=/dev/input/event1 \\\n"
        "      --vol-down-event=/dev/input/event2 \\\n"
        "      --wad=/usr/share/fbdoom/DOOM.wad\n",

        program,
        program
    );
}


static int parse_display(
    const char *value,
    uintptr_t *address,
    unsigned int *width,
    unsigned int *height
)
{
    unsigned long long parsed_address;
    unsigned long parsed_width;
    unsigned long parsed_height;

    if (sscanf(
            value,
            "%llx,%lu,%lu",
            &parsed_address,
            &parsed_width,
            &parsed_height
        ) != 3) {

        return -1;
    }

    if (parsed_width == 0 ||
        parsed_height == 0) {

        return -1;
    }

    *address = (uintptr_t)parsed_address;

    *width = (unsigned int)parsed_width;
    *height = (unsigned int)parsed_height;

    return 0;
}


static int parse_scale(
    const char *value,
    unsigned int *scale
)
{
    unsigned long parsed;
    char *end;

    if (strncmp(value, "--scale=", 8) != 0)
        return -1;

    errno = 0;

    end = NULL;

    parsed = strtoul(
        value + 8,
        &end,
        10
    );

    if (errno != 0 ||
        end == value + 8 ||
        *end != '\0' ||
        parsed == 0) {

        return -1;
    }

    *scale = (unsigned int)parsed;

    return 0;
}


static int parse_options(
    int argc,
    char **argv,
    FBDoomOptions *options
)
{
    int i;

    for (i = 2; i < argc; i++) {

        const char *arg = argv[i];

        if (strcmp(
                arg,
                "--keep-leftover"
            ) == 0) {

            options->keep_leftover = 1;

            continue;
        }


        if (strncmp(
                arg,
                "--scale=",
                8
            ) == 0) {

            if (parse_scale(
                    arg,
                    &options->scale
                ) != 0) {

                fprintf(
                    stderr,
                    "Invalid scale: %s\n",
                    arg
                );

                return -1;
            }

            continue;
        }


        if (strncmp(
                arg,
                "--vol-up-event=",
                15
            ) == 0) {

            options->vol_up_event =
                arg + 15;

            continue;
        }


        if (strncmp(
                arg,
                "--power-event=",
                14
            ) == 0) {

            options->power_event =
                arg + 14;

            continue;
        }


        if (strncmp(
                arg,
                "--vol-down-event=",
                18
            ) == 0) {

            options->vol_down_event =
                arg + 18;

            continue;
        }


        if (strncmp(
                arg,
                "--wad=",
                6
            ) == 0) {

            options->wad =
                arg + 6;

            continue;
        }


        fprintf(
            stderr,
            "Unknown option: %s\n",
            arg
        );

        return -1;
    }

    return 0;
}


static void make_test_frame(
    uint8_t *framebuffer,
    uint32_t *palette
)
{
    unsigned int x;
    unsigned int y;

    /*
     * Temporary grayscale palette.
     *
     * Chocolate Doom will eventually provide the
     * real palette through I_SetPalette().
     */

    for (unsigned int i = 0; i < 256; i++) {

        uint32_t c = i;

        palette[i] =
            0xff000000u |
            (c << 16) |
            (c << 8) |
            c;
    }


    /*
     * Gradient.
     */

    for (y = 0; y < GAME_HEIGHT; y++) {

        for (x = 0; x < GAME_WIDTH; x++) {

            uint8_t value;

            value =
                (uint8_t)(
                    (x * 255) /
                    (GAME_WIDTH - 1)
                );

            framebuffer[
                y * GAME_WIDTH + x
            ] = value;
        }
    }


    /*
     * White vertical line.
     */

    for (y = 0; y < GAME_HEIGHT; y++) {

        framebuffer[
            y * GAME_WIDTH +
            GAME_WIDTH / 2
        ] = 255;
    }


    /*
     * White horizontal line.
     */

    for (x = 0; x < GAME_WIDTH; x++) {

        framebuffer[
            (GAME_HEIGHT / 2) *
            GAME_WIDTH +
            x
        ] = 255;
    }
}


int main(
    int argc,
    char **argv
)
{
    FBDoomOptions options;

    uint8_t *framebuffer;

    uint32_t palette[256];


    if (argc < 2) {

        print_usage(argv[0]);

        return 1;
    }


    memset(
        &options,
        0,
        sizeof(options)
    );


    options.scale = DEFAULT_SCALE;


    if (parse_display(
            argv[1],
            &options.address,
            &options.display_width,
            &options.display_height
        ) != 0) {

        fprintf(
            stderr,
            "Invalid display specification: %s\n",
            argv[1]
        );

        print_usage(argv[0]);

        return 1;
    }


    if (parse_options(
            argc,
            argv,
            &options
        ) != 0) {

        print_usage(argv[0]);

        return 1;
    }


    printf(
        "FBDOOM video test\n"
        "-----------------\n"
    );


    printf(
        "Display address : 0x%lx\n",
        (unsigned long)options.address
    );


    printf(
        "Display size    : %ux%u\n",
        options.display_width,
        options.display_height
    );


    printf(
        "Game size       : %ux%u\n",
        GAME_WIDTH,
        GAME_HEIGHT
    );


    printf(
        "Scale           : %u\n",
        options.scale
    );


    printf(
        "Keep leftover   : %s\n",
        options.keep_leftover ?
            "yes" :
            "no"
    );


    printf(
        "Volume Up       : %s\n",
        options.vol_up_event ?
            options.vol_up_event :
            "(not set)"
    );


    printf(
        "Power           : %s\n",
        options.power_event ?
            options.power_event :
            "(not set)"
    );


    printf(
        "Volume Down     : %s\n",
        options.vol_down_event ?
            options.vol_down_event :
            "(not set)"
    );


    printf(
        "WAD             : %s\n",
        options.wad ?
            options.wad :
            "(not set)"
    );


    if (FBDoom_VideoInit(
            options.address,
            options.display_width,
            options.display_height,
            options.scale
        ) != 0) {

        fprintf(
            stderr,
            "Failed to initialize FBDOOM video backend.\n"
        );

        return 1;
    }


    FBDoom_VideoSetKeepLeftover(
        options.keep_leftover
    );


    framebuffer = malloc(
        (size_t)GAME_WIDTH *
        (size_t)GAME_HEIGHT
    );


    if (framebuffer == NULL) {

        fprintf(
            stderr,
            "Failed to allocate framebuffer.\n"
        );

        FBDoom_VideoShutdown();

        return 1;
    }


    make_test_frame(
        framebuffer,
        palette
    );


    FBDoom_VideoPresent(
        framebuffer,
        palette
    );


    printf(
        "Frame presented.\n"
    );


    free(framebuffer);

    FBDoom_VideoShutdown();

    return 0;
}
