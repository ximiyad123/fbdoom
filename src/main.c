#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "d_main.h"
#include "m_misc.h"
#include "m_argv.h"

#include "i_video_fbdoom.h"


/*
 * Chocolate Doom 3.1.1 defines D_DoomMain() in d_main.c
 * but does not declare it in d_main.h.
 */

void D_DoomMain(void);


#define DEFAULT_SCALE 2


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


/*
 * These are consumed by the FBDOOM platform backends.
 */

uintptr_t fbdoom_address;
unsigned int fbdoom_display_width;
unsigned int fbdoom_display_height;
unsigned int fbdoom_scale;
int fbdoom_keep_leftover;

const char *fbdoom_vol_up_event;
const char *fbdoom_power_event;
const char *fbdoom_vol_down_event;


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

        if (strcmp(arg, "--keep-leftover") == 0) {

            options->keep_leftover = 1;
            continue;
        }

        if (strncmp(arg, "--scale=", 8) == 0) {

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

        if (strncmp(arg, "--vol-up-event=", 15) == 0) {

            options->vol_up_event = arg + 15;
            continue;
        }

        if (strncmp(arg, "--power-event=", 14) == 0) {

            options->power_event = arg + 14;
            continue;
        }

        if (strncmp(arg, "--vol-down-event=", 18) == 0) {

            options->vol_down_event = arg + 18;
            continue;
        }

        if (strncmp(arg, "--wad=", 6) == 0) {

            options->wad = arg + 6;
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


int main(
    int argc,
    char **argv
)
{
    FBDoomOptions options;

    char **doom_argv;
    int doom_argc;
    int i;
    int extra_args = 0;


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


    /*
     * Export FBDOOM platform configuration.
     */

    fbdoom_address = options.address;
    fbdoom_display_width = options.display_width;
    fbdoom_display_height = options.display_height;
    fbdoom_scale = options.scale;
    fbdoom_keep_leftover = options.keep_leftover;

    fbdoom_vol_up_event = options.vol_up_event;
    fbdoom_power_event = options.power_event;
    fbdoom_vol_down_event = options.vol_down_event;


    /*
     * Build the argument list passed to Chocolate Doom.
     */

    if (options.wad != NULL)
        extra_args = 2;


    doom_argv = malloc(
        (size_t)(argc + extra_args + 1) *
        sizeof(*doom_argv)
    );

    if (doom_argv == NULL) {

        fprintf(
            stderr,
            "Failed to allocate argument list.\n"
        );

        return 1;
    }


    doom_argc = 0;

    doom_argv[doom_argc++] = argv[0];


    for (i = 2; i < argc; i++) {

        const char *arg = argv[i];


        /*
         * Remove FBDOOM-specific options.
         */

        if (strncmp(arg, "--scale=", 8) == 0)
            continue;

        if (strcmp(arg, "--keep-leftover") == 0)
            continue;

        if (strncmp(arg, "--vol-up-event=", 15) == 0)
            continue;

        if (strncmp(arg, "--power-event=", 14) == 0)
            continue;

        if (strncmp(arg, "--vol-down-event=", 18) == 0)
            continue;

        if (strncmp(arg, "--wad=", 6) == 0)
            continue;


        doom_argv[doom_argc++] = argv[i];
    }


    /*
     * Convert --wad=PATH into normal Chocolate Doom arguments.
     */

    if (options.wad != NULL) {

        doom_argv[doom_argc++] = "-iwad";
        doom_argv[doom_argc++] = (char *)options.wad;
    }


    doom_argv[doom_argc] = NULL;


    /*
     * Chocolate Doom's m_argv.c owns myargc/myargv.
     */

    myargc = doom_argc;

    myargv = malloc(
        (size_t)doom_argc *
        sizeof(*myargv)
    );

    if (myargv == NULL) {

        fprintf(
            stderr,
            "Failed to allocate Chocolate Doom arguments.\n"
        );

        free(doom_argv);


        return 1;
    }


    for (i = 0; i < doom_argc; i++)
        myargv[i] = M_StringDuplicate(doom_argv[i]);


    free(doom_argv);



    M_SetExeDir();

    printf(
        "FBDOOM\n"
        "------\n"
        "Display : 0x%lx %ux%u\n"
        "Scale   : %u\n"
        "WAD     : %s\n"
        "\n",
        (unsigned long)fbdoom_address,
        fbdoom_display_width,
        fbdoom_display_height,
        fbdoom_scale,
        options.wad ? options.wad : "(auto)"
    );


    /*
     * Start the real Chocolate Doom engine.
     */

    D_DoomMain();


    /*
     * Normally D_DoomMain() never returns, but clean up
     * the manually-created argument vector if it does.
     */

    if (myargv != NULL) {

        for (i = 0; i < myargc; i++)
            free(myargv[i]);

        free(myargv);
        myargv = NULL;
    }


    return 0;
}
