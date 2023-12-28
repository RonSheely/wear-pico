#include "sw_apps/apps.h"
#include "sw_common/util.h"
#include "sw_res/resources.h"

/* Initializes the clock User Interface */
extern enum app_status_t apps_load_clock(void);
/* Shuts the screen off until its cancellation */
extern enum app_status_t apps_lock_screen(void);
/* Initializes the menu User Interface */
extern enum app_status_t apps_load_menu(void);
/* Initializes the Alarm User Interface */
extern enum app_status_t apps_load_alarm(void);
/* Initializes the Stopwatch User Interface */
extern enum app_status_t apps_load_chrono(void);
/* Initializes the Calendar User Interface */
extern enum app_status_t apps_load_event(void);
/* Initializes the Media User Interface */
extern enum app_status_t apps_load_media(void);
/* Initializes the Pedometer User Interface */
extern enum app_status_t apps_load_step(void);
/* Initializes the logger User Interface */
extern enum app_status_t apps_load_log(void);

const char* _menu_s(enum menu_t m)
{
    switch (m) {
    case MENU_ALARM: return "MENU_ALARM";
    case MENU_CHRONO: return "MENU_CHRONO";
    case MENU_EVENT: return "MENU_EVENT";
    case MENU_MEDIA: return "MENU_MEDIA";
    case MENU_STEP: return "MENU_STEP";
    case MENU_LOG: return "MENU_LOG";
    default: return "NONE";
    }
}
#define MENU_S(CURRENT)                                                        \
    CURRENT == MENU_ALARM    ? "MENU_ALARM"                                    \
    : CURRENT == MENU_CHRONO ? "MENU_CHRONO"                                   \
    : CURRENT == MENU_EVENT  ? "MENU_EVENT"                                    \
    : CURRENT == MENU_MEDIA  ? "MENU_MEDIA"                                    \
    : CURRENT == MENU_STEP   ? "MENU_STEP"                                     \
    : CURRENT == MENU_LOG    ? "MENU_LOG"                                      \
                             : "NONE"

enum app_status_t apps_load(enum screen_t s)
{
    int current_mode = XY.mode;
    enum app_status_t r;
    switch (s) {
    case SCREEN_LOCK: r = apps_lock_screen(); break;
    case SCREEN_CLOCK: r = apps_load_clock(); break;
    case SCREEN_MENU: r = apps_load_menu(); break;
    case SCREEN_ALARM: r = apps_load_alarm(); break;
    case SCREEN_CHRONO: r = apps_load_chrono(); break;
    case SCREEN_MEDIA: r = apps_load_media(); break;
    case SCREEN_LOG: r = apps_load_log(); break;
    case SCREEN_STEP:
        r = apps_load_step();
        break;
        /*    case SCREEN_EVENT: r = apps_load_event();break; */
    default: break;
    }

    /* Revert the module configurations */
    XY.mode = current_mode;
    if (Touch_1IN28_init(XY.mode) != 1) WARN(SCR_WARN_TOUCH_FAILED);
    return r;
}

/* Displays the current menu screen */
static void _menu_display(enum menu_t current)
{
    if (screen.redraw == DISP_REDRAW) {
        apps_reset();
        apps_draw(res_get_direction(GEST_DIR_L), 0, 90);
        apps_draw(res_get_direction(GEST_DIR_R), 200, 90);
        apps_draw(res_get_direction(GEST_DIR_D), 96, 202);
    }
    screen.sstate = SCREEN_MENU;
    apps_draw(res_get_menu_screen(current), 40, 40);
    apps_post_process(false);
    LCD_1IN28_Display(screen.buffer);

    PRINT("SELECT %s", , _menu_s(current));
    screen.redraw = DISP_SYNC;
    XY.Gesture = None;
}

enum app_status_t apps_load_menu()
{
    SET_MODULE(SCREEN_MENU, TOUCH_GESTURE);

    enum menu_t current = MENU_ALARM;
    while (true) {
        if (!apps_poll_popup()) screen.redraw = DISP_REDRAW;
        switch (XY.Gesture) {
        case UP: return APP_OK; /*Calling DOWN returns to clock*/
        case LEFT:
            current = current == MENU_T_SIZE - 1 ? MENU_ALARM : current + 1;
            screen.redraw = DISP_REDRAW;
            break;
        case RIGHT:
            current = current == 0 ? MENU_T_SIZE - 1 : current - 1;
            screen.redraw = DISP_REDRAW;
            break;
        case CLICK: apps_load(current + 3); break;
        case DOUBLE_CLICK: apps_load(SCREEN_LOCK); break;
        case LONG_PRESS: break;
        }

        if (screen.sstate != SCREEN_MENU) {
            screen.sstate = SCREEN_MENU;
            screen.redraw = DISP_REDRAW;
        }
        if (screen.redraw) _menu_display(current);
    }
}
