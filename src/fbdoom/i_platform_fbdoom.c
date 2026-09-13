#include <stdio.h>

#include "i_system.h"


/*
 * FBDOOM has no XScreenSaver integration.
 */

void I_CheckIsScreensaver(void)
{
}


/*
 * FBDOOM does not currently provide the
 * Textscreen-based network launch UI.
 */

void NET_WaitForLaunch(void)
{
    fprintf(
        stderr,
        "FBDOOM: network launch UI is unavailable.\n"
    );
}


/*
 * Doom can request the ENDOOM screen when exiting.
 *
 * FBDOOM has no Textscreen backend, so simply skip it.
 */

void I_Endoom(void)
{
}
