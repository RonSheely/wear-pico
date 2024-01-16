#include "sw_bt/bt.h"
#include "sw_common/util.h"
#include "sw_os/state.h"

/**
 * Parses and splits the incoming request into tokens up to the size of the
 * given array's size. Updates the size to match the actual number of tokens.
 * @str - incoming data
 * @str_s - size of the data
 * @arr - array to insert into
 * @array_s - size of the array
 * @returns - error code
 */
static enum bt_fmt_t _req_decode(char* str, size_t str_s, char** arr,
                                 int* arr_s);

/**
 * Parses and validates the incoming request. Sets the request type,
 * @arr - array of tokens
 * @array_s - size of the array
 * @req - request to set
 * @returns - error code
 */
enum bt_fmt_t _req_parse(char** arr, int arr_s, enum bt_req_t* req);

/**
 * Executes the function corresponding to the request type
 * @arr - array of tokens
 * @arr_s - size of the array
 * @req - request type to select
 * @returns - error code
 */
static enum bt_fmt_t _req_route(char** arr, int arr_s,
                                enum bt_req_t request_type);

enum bt_fmt_t bt_handle_req(char* str, size_t str_s)
{
    char* str_arr[5] = {0};
    enum bt_fmt_t err;
    int size = 5;
    enum bt_req_t request_t;

    err = _req_decode(str, str_s, str_arr, &size);
    if (err) { return err; }

    err = _req_parse(str_arr, size, &request_t);
    if (err) { return err; }

    return _req_route(str_arr, size, request_t);
}

static enum bt_fmt_t _req_decode(char* str, size_t str_s, char** arr,
                                 int* arr_s)
{
    int str_arr_i = 0;
    size_t i = 0;
    size_t len = strnlen(str, str_s);
    char* old_str = str;
    while (i < len && str_arr_i < *arr_s) {
        if (str[i] == '|') {
            str[i] = '\0';
            arr[str_arr_i] = old_str;
            old_str = &str[i + 1];
            str_arr_i++;
        }
        i++;
    }
    if (str_arr_i == 0) { return ERROR(BT_FMT_ERROR_INVALID_INPUT); }
    *arr_s = str_arr_i;
    return BT_FMT_OK;
}

enum bt_fmt_t _req_parse(char** arr, int arr_s, enum bt_req_t* req)
{
    if (arr_s > 5) { return ERROR(BT_FMT_ERROR_INVALID_INPUT); }
    char* endptr;
    long request_type = strtol(arr[0], &endptr, 10);
    if (*endptr != '\0') { return ERROR(BT_FMT_ERROR_REQ_TYPE); }

    *req = request_type;
    return BT_FMT_OK;
}

static enum bt_fmt_t _handle_fetch_date(char** str, int str_s)
{
    if (str_s < 2) { return ERROR(BT_FMT_ERROR_PAYLOAD); }
    bool result = date_decode(str[1], &state.dt);
    state.is_connected = true;
    PRINT("fetch_date(%s) = %s", , str[1], result ? "true" : "false");
    return result ? BT_FMT_OK : ERROR(BT_FMT_ERROR_DATE_PARSE);
}

static enum bt_fmt_t _handle_call_begin(char** str, int str_s)
{
    if (str_s < 2) { return ERROR(BT_FMT_ERROR_PAYLOAD); }
    Popup p = (Popup){.type = POPUP_CALL, .value.caller.is_over = false};
    strncpy(p.value.caller.name, str[1], strnlen(str[1], 24));
    os_request_popup(p);
    PRINT("received_call(%s)", , str[1]);
    return BT_FMT_OK;
}

static enum bt_fmt_t _handle_call_end()
{
    if (state.popup.type == POPUP_CALL) {
        state.popup.value.caller.is_over = true;
    }
    PRINT("ended_call()");
    return BT_FMT_OK;
}

static enum bt_fmt_t _handle_notify(char** str, int str_s)
{
    if (str_s < 3) { return ERROR(BT_FMT_ERROR_PAYLOAD); }
    Popup p = (Popup){.type = POPUP_NOTIFY};
    strncpy(p.value.notify.title, str[1], strnlen(str[1], 12));
    strncpy(p.value.notify.text, str[2], strnlen(str[2], 127));
    os_request_popup(p);
    PRINT("notify(%s, %s)", , p.value.notify.title, p.value.notify.text);
    return BT_FMT_OK;
}

static enum bt_fmt_t _handle_reminder(char** str, int str_s)
{
    if (str_s < 2) { return ERROR(BT_FMT_ERROR_PAYLOAD); }
    Popup p = (Popup){.type = POPUP_NOTIFY};
    strncpy(p.value.notify.title, "Reminder", strlen("Reminder"));
    strncpy(p.value.notify.text, str[1], strnlen(str[1], 127));
    os_request_popup(p);
    PRINT("reminder(%s)", , p.value.notify.text);
    return BT_FMT_OK;
}

static enum bt_fmt_t _handle_osc(char** str, int str_s)
{
    if (str_s < 4) return ERROR(BT_FMT_ERROR_PAYLOAD);
    state.media.is_playing =
        str[1][0] == 't' || str[1][0] == 'T' || str[1][0] == '1';

    int len_song = strnlen(str[2], 30);
    strncpy(state.media.song, str[2], len_song);
    if (len_song < 30) { state.media.song[len_song] = '\0'; }
    int len_artist = strnlen(str[3], 30);
    strncpy(state.media.artist, str[3], len_artist);
    if (len_artist < 30) { state.media.artist[len_artist] = '\0'; }

    state.media.is_fetched = false;
    PRINT("media(%s by %s)", , state.media.song, state.media.artist);
    return BT_FMT_OK;
}

void str_to_alarm(const char* str, DateTime* dt)
{
    int digits[5];
    for (int i = 0; i < 4; i++) {
        if (str[i] < '0' || str[i] > '9') return;
        digits[i] = str[i] - '0';
    }
    int hour = digits[0] * 10 + digits[1];
    int min = digits[2] * 10 + digits[3];
    if (hour > 23 || min > 59) return;
    dt->hour = hour;
    dt->minute = min;
}

static enum bt_fmt_t _handle_fetch_alarm(char** str, int str_s)
{
    if (str_s < 2) { return ERROR(BT_FMT_ERROR_PAYLOAD); }
    if (str[1][0] < '0' || str[1][0] > '4') {
        return ERROR(BT_FMT_ERROR_ALARM_PARSE);
    }
    int alarm_count = str[1][0] - '0';

    state.alarms.len = alarm_count;
    for (int i = 0; i < alarm_count; i++) {
        /* is_active value is set by the watch */
        str_to_alarm(str[i + 2], &state.alarms.list[i].at);
        PRINT("alarm_%d(%d:%d)", , i, state.alarms.list[i].at.hour,
              state.alarms.list[i].at.minute);
    }
    state.alarms.is_fetched = false;
    return BT_FMT_OK;
}

static enum bt_fmt_t _handle_step(char** str, int str_s)
{
    if (str_s < 1) { return ERROR(BT_FMT_ERROR_PAYLOAD); }
    bt_send_resp(BT_RESP_STEP);
    PRINT("step()");
    return BT_FMT_OK;
}

static enum bt_fmt_t _handle_hb(char** str, int str_s)
{
    if (str_s < 1) { return ERROR(BT_FMT_ERROR_PAYLOAD); }
    bt_send_resp(BT_RESP_OK);
    PRINT("heartbeat()");
    return BT_FMT_OK;
}

static enum bt_fmt_t _req_route(char** arr, int arr_s,
                                enum bt_req_t request_type)
{
    switch (request_type) {
    case BT_REQ_CALL_BEGIN: return _handle_call_begin(arr, arr_s);
    case BT_REQ_CALL_END: return _handle_call_end();
    case BT_REQ_NOTIFY: return _handle_notify(arr, arr_s);
    case BT_REQ_REMINDER: return _handle_reminder(arr, arr_s);
    case BT_REQ_OSC: return _handle_osc(arr, arr_s);
    case BT_REQ_FETCH_DATE: return _handle_fetch_date(arr, arr_s);
    case BT_REQ_FETCH_ALARM: return _handle_fetch_alarm(arr, arr_s);
    case BT_REQ_STEP: return _handle_step(arr, arr_s);
    case BT_REQ_HB: return _handle_hb(arr, arr_s);
    default: return ERROR(BT_FMT_ERROR_REQ_TYPE);
    }
}
