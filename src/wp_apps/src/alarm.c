#include "wp_apps/apps.h"
#include "wp_bt/bt.h"
#include "wp_dev/state.h"
#include "wp_res/resources.h"
#define ALARM_X 40
#define ALARM_Y 66
#define ALARM_BUTTON_SIZE 164, 36

/* Draws the idx'th alarm to the screen. */
static void _alarm_draw(int idx, Alarm* alarm)
{
    int y = ALARM_Y + idx * 36;
    apps_draw(res_get_app_alarm_button(alarm->is_active), ALARM_X, y);
    char str[6];
    snprintf(str, 6, "%02d:%02d", alarm->at.hour, alarm->at.minute);
    int fg = alarm->is_active ? COLOR_FG : GRAY;
    Paint_DrawString_EN(ALARM_X + 12, y + 10, str, &Font16, COLOR_BG, fg);
}

/* Check whether given toggle is updated or not. */
static void _check_buttons(int idx)
{
    if (apps_is_clicked(ALARM_X, ALARM_Y + idx * 36, ALARM_BUTTON_SIZE)) {
        state.alarms.list[idx].is_active = !state.alarms.list[idx].is_active;
        PRINT("ALARM_%d = %s", , idx,
              state.alarms.list[idx].is_active ? "TRUE" : "FALSE");
        _alarm_draw(idx, &state.alarms.list[idx]);
        screen.redraw = DISP_PARTIAL;
    }
}

enum app_status_t apps_load_alarm(void)
{
    bool clicked;
    int x = 0, y = 0;
    while (true) {
        if (x != XY.x_point || y != XY.y_point) {
            x = XY.x_point;
            y = XY.y_point;
            clicked = true;
        }
        if (!apps_poll_popup()) { screen.redraw = DISP_REDRAW; }
        if (apps_is_exited()) { return APP_OK; }

        if (clicked) {
            for (int i = 0; i < state.alarms.len; i++) {
                _check_buttons(i);
            }
        }
        if (!state.alarms.is_fetched) {
            PRINT(BT_FETCH_ALARMS);
            state.alarms.is_fetched = true;
            screen.redraw = DISP_REDRAW;
        }
        if (apps_set_titlebar(SCREEN_ALARM, POPUP_NONE)) {
            XY.x_point = 0;
            XY.y_point = 0;
            for (int i = 0; i < state.alarms.len; i++)
                _alarm_draw(i, &state.alarms.list[i]);
            apps_post_process(false);
        }

        if (screen.redraw) {
            LCD_1IN28_Display(screen.buffer);
            screen.redraw = DISP_SYNC;
        }
        clicked = false;
    }
}