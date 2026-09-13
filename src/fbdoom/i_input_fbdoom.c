#include "i_input_fbdoom.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <linux/input.h>

#undef KEY_ENTER
#undef KEY_END

#include "doomtype.h"
#include "d_event.h"
#include "doomkeys.h"
#include "m_config.h"

#define FBDOOM_VOL_UP_EVENT   "/dev/input/event0"
#define FBDOOM_POWER_EVENT    "/dev/input/event1"
#define FBDOOM_VOL_DOWN_EVENT "/dev/input/event2"

#define FBDOOM_POWER_USE_TIME_MS 1000

static int vol_up_fd = -1;
static int power_fd = -1;
static int vol_down_fd = -1;

static int vol_up_held = 0;
static int power_held = 0;
static int vol_down_held = 0;

static int current_action = 0;

static long long power_press_time = 0;

/*
 * Fire/use are released on the next input poll.
 *
 * This makes the key remain down for at least one Doom tic.
 */
static int pending_power_release = 0;
static int pending_power_key = 0;

/*
 * Chocolate Doom input variables.
 */
int novert = 0;
int vanilla_keyboard_mapping = 1;

float mouse_acceleration = 2.0f;
int mouse_threshold = 10;

static long long get_time_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;

    return
        (long long)ts.tv_sec * 1000LL +
        (long long)(ts.tv_nsec / 1000000LL);
}

static void post_key(int type, int key)
{
    event_t event;

    memset(&event, 0, sizeof(event));

    event.type = type;
    event.data1 = key;

    D_PostEvent(&event);
}

static void release_current_action(void)
{
    switch (current_action)
    {
        case 1:
            post_key(ev_keyup, KEY_UPARROW);
            break;

        case 2:
            post_key(ev_keyup, KEY_DOWNARROW);
            break;

        case 3:
            post_key(ev_keyup, KEY_LEFTARROW);
            break;

        case 4:
            post_key(ev_keyup, KEY_RIGHTARROW);
            break;

        default:
            break;
    }

    current_action = 0;
}

static void update_action(void)
{
    int new_action = 0;

    /*
     * Power + Vol+ = turn left
     * Power + Vol- = turn right
     *
     * Vol+ alone = forward
     * Vol- alone = backward
     */
    if (power_held && vol_up_held)
        new_action = 3;
    else if (power_held && vol_down_held)
        new_action = 4;
    else if (vol_up_held)
        new_action = 1;
    else if (vol_down_held)
        new_action = 2;

    if (new_action == current_action)
        return;

    release_current_action();

    switch (new_action)
    {
        case 1:
            post_key(ev_keydown, KEY_UPARROW);
            break;

        case 2:
            post_key(ev_keydown, KEY_DOWNARROW);
            break;

        case 3:
            post_key(ev_keydown, KEY_LEFTARROW);
            break;

        case 4:
            post_key(ev_keydown, KEY_RIGHTARROW);
            break;

        default:
            break;
    }

    current_action = new_action;
}

static void process_power_press(void)
{
    power_held = 1;

    power_press_time = get_time_ms();

    update_action();
}

static void process_power_release(void)
{
    long long now;
    long long held_time;

    now = get_time_ms();

    if (power_press_time == 0)
        held_time = 0;
    else
        held_time = now - power_press_time;

    power_held = 0;

    /*
     * If Power was being used together with a volume key,
     * this was a rotation action, not fire/use/Enter.
     */
    if (vol_up_held || vol_down_held)
    {
        power_press_time = 0;

        update_action();

        return;
    }

    /*
     * Long Power press = USE.
     */
    if (held_time >= FBDOOM_POWER_USE_TIME_MS)
    {
        post_key(
            ev_keydown,
            ' '
        );

        pending_power_key = ' ';
        pending_power_release = 1;

        power_press_time = 0;

        update_action();

        return;
    }

    /*
     * Short Power press = FIRE.
     *
     * Keep RCTRL held until the next input poll.
     */
    post_key(
        ev_keydown,
        KEY_RCTRL
    );

    pending_power_key = KEY_RCTRL;
    pending_power_release = 1;

    /*
     * Also send Enter.
     *
     * This allows Power to act as Enter in menus.
     */
    post_key(
        ev_keydown,
        KEY_ENTER
    );

    post_key(
        ev_keyup,
        KEY_ENTER
    );

    power_press_time = 0;

    update_action();
}

static void process_event(
    const struct input_event *event,
    int device
)
{
    int pressed;

    if (event->type != EV_KEY)
        return;

    /*
     * Ignore Linux key-repeat events.
     */
    if (event->value == 2)
        return;

    pressed = event->value != 0;

    if (device == 0)
    {
        vol_up_held = pressed;

        update_action();

        return;
    }

    if (device == 1)
    {
        if (pressed)
            process_power_press();
        else
            process_power_release();

        return;
    }

    if (device == 2)
    {
        vol_down_held = pressed;

        update_action();

        return;
    }
}

static void poll_device(
    int fd,
    int device
)
{
    struct input_event event;
    ssize_t result;

    if (fd < 0)
        return;

    for (;;)
    {
        result = read(
            fd,
            &event,
            sizeof(event)
        );

        if (result == (ssize_t)sizeof(event))
        {
            process_event(
                &event,
                device
            );

            continue;
        }

        if (result < 0 &&
            (errno == EAGAIN ||
             errno == EWOULDBLOCK))
        {
            break;
        }

        if (result < 0 &&
            errno == EINTR)
        {
            continue;
        }

        break;
    }
}

void I_StartTextInput(
    int x1,
    int y1,
    int x2,
    int y2
)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
}

void I_StopTextInput(void)
{
}

void I_BindInputVariables(void)
{
    M_BindFloatVariable(
        "mouse_acceleration",
        &mouse_acceleration
    );

    M_BindIntVariable(
        "mouse_threshold",
        &mouse_threshold
    );

    M_BindIntVariable(
        "vanilla_keyboard_mapping",
        &vanilla_keyboard_mapping
    );

    M_BindIntVariable(
        "novert",
        &novert
    );
}

int FBDoom_InputInit(void)
{
    vol_up_fd = open(
        FBDOOM_VOL_UP_EVENT,
        O_RDONLY | O_NONBLOCK
    );

    if (vol_up_fd < 0)
    {
        fprintf(
            stderr,
            "FBDOOM: failed to open %s: %s\n",
            FBDOOM_VOL_UP_EVENT,
            strerror(errno)
        );

        return -1;
    }

    power_fd = open(
        FBDOOM_POWER_EVENT,
        O_RDONLY | O_NONBLOCK
    );

    if (power_fd < 0)
    {
        fprintf(
            stderr,
            "FBDOOM: failed to open %s: %s\n",
            FBDOOM_POWER_EVENT,
            strerror(errno)
        );

        close(vol_up_fd);

        vol_up_fd = -1;

        return -1;
    }

    vol_down_fd = open(
        FBDOOM_VOL_DOWN_EVENT,
        O_RDONLY | O_NONBLOCK
    );

    if (vol_down_fd < 0)
    {
        fprintf(
            stderr,
            "FBDOOM: failed to open %s: %s\n",
            FBDOOM_VOL_DOWN_EVENT,
            strerror(errno)
        );

        close(vol_up_fd);
        close(power_fd);

        vol_up_fd = -1;
        power_fd = -1;

        return -1;
    }

    vol_up_held = 0;
    power_held = 0;
    vol_down_held = 0;

    current_action = 0;

    power_press_time = 0;

    pending_power_release = 0;
    pending_power_key = 0;

    fprintf(
        stderr,
        "FBDOOM: input initialized\n"
        "  Vol+  = %s\n"
        "  Power = %s\n"
        "  Vol-  = %s\n",
        FBDOOM_VOL_UP_EVENT,
        FBDOOM_POWER_EVENT,
        FBDOOM_VOL_DOWN_EVENT
    );

    return 0;
}

void FBDoom_InputShutdown(void)
{
    release_current_action();

    /*
     * Release a pending fire/use key before shutting down.
     */
    if (pending_power_release)
    {
        post_key(
            ev_keyup,
            pending_power_key
        );

        pending_power_release = 0;
        pending_power_key = 0;
    }

    if (vol_up_fd >= 0)
    {
        close(vol_up_fd);
        vol_up_fd = -1;
    }

    if (power_fd >= 0)
    {
        close(power_fd);
        power_fd = -1;
    }

    if (vol_down_fd >= 0)
    {
        close(vol_down_fd);
        vol_down_fd = -1;
    }

    vol_up_held = 0;
    power_held = 0;
    vol_down_held = 0;

    current_action = 0;

    power_press_time = 0;

    pending_power_release = 0;
    pending_power_key = 0;
}

void FBDoom_InputPoll(void)
{
    /*
     * Release the previous Power fire/use action.
     *
     * The keydown was posted during the previous poll,
     * so Doom gets one complete tic with the key held.
     */
    if (pending_power_release)
    {
        post_key(
            ev_keyup,
            pending_power_key
        );

        pending_power_release = 0;
        pending_power_key = 0;
    }

    poll_device(
        vol_up_fd,
        0
    );

    poll_device(
        power_fd,
        1
    );

    poll_device(
        vol_down_fd,
        2
    );
}
