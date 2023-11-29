/******************************************************************************

 * File: include/protocol.h
 *
 * Author: Umut Sevdi
 * Created: 10/31/23
 * Description: Messaging protocol definition.
 ┌────────────────┐                                     ┌───────────┐
 │                │─────────── Connect&DateTime ───────>│           │
 │                │<───────────────── OK ───────────────│           │
 │                │───────────────── Ping ─────────────>│           │
 │ Android Client │<─────────────── Events ─────────────│ SW Server │
 │                │───────────────── Data ─────────────>│           │
 │                │──────────────── Notify ────────────>│           │
 │                │<───────────── NotifiyResp ──────────│           │
 └────────────────┘                                     └───────────┘

*****************************************************************************/

#ifndef SW_PROTOCOL
#define SW_PROTOCOL

#include "util.h"

/******************************************************************************
                                Common
*****************************************************************************/

#define SW_PK_MAGIC_NUMBER 2791

/* Status codes related to protocol */
enum MSGFMT {
    MSGFMT_OK,
    MSGFMT_ERROR_MAGIC,
    MSGFMT_ERROR_DATE_LEN,
    MSGFMT_ERROR_DATE_PARSE,
    MSGFMT_ERROR_REQ_TYPE,
    MSGFMT_ERROR_EVENT_PARSE,
    MSGFMT_ERROR_NOTIFY_PARSE,
    MSGFMT_ERROR_EVENTLST_PARSE,
    MSGFMT_ERROR_BAT_PARSE,
    MSGFMT_ERROR_OSC_PARSE,

};

typedef enum {
    DT_WC_YEAR = 0b000001,
    DT_WC_MONTH = 0b000010,
    DT_WC_DAY = 0b000100,
    DT_WC_HOUR = 0b001000,
    DT_WC_MIN = 0b010000,
    DT_WC_SEC = 0b100000,
} DT_WC;

/**
 * Represents any date or time with optional fields.
 *
 * String Format:  YYYYmmDDHHMMSS
 *
 * - DateTime structure is generated from an array of 14 characters where each
 * can be a number or '?'.
 * If any date time field starts with a '?' that section is ignored.
 * - After the sturcture is generated DT_WC enum can be used on
 * DateTime.flag to obtain ignored fields.
 *
 *   Example:
 *   - 202112201830??  20 December 2012 18:30
 *   - ????????123015  12:30:15
 */
typedef struct {
    int16_t flag;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} DateTime;

#define DATETIME_DAY(d)                                                        \
    d == 1   ? "Monday"                                                        \
    : d == 2 ? "Tuesday"                                                       \
    : d == 3 ? "Wednesday"                                                     \
    : d == 4 ? "Thursday"                                                      \
    : d == 5 ? "Friday"                                                        \
    : d == 6 ? "Saturday"                                                      \
    : d == 7 ? "Sunday"                                                        \
             : "Null"
#define DATETIME_MONTH(m)                                                      \
    m == 1    ? "Jan"                                                          \
    : m == 2  ? "Feb"                                                          \
    : m == 3  ? "Mar"                                                          \
    : m == 4  ? "Apr"                                                          \
    : m == 5  ? "May"                                                          \
    : m == 6  ? "Jun"                                                          \
    : m == 7  ? "Jul"                                                          \
    : m == 8  ? "Aug"                                                          \
    : m == 9  ? "Sep"                                                          \
    : m == 10 ? "Oct"                                                          \
    : m == 11 ? "Nov"                                                          \
    : m == 12 ? "Dec"                                                          \
              : "Null"

/**
 * Converts the given null terminated string of numbers into a DateTime struct.
 * @buffer - a string that contains 0-9 or ?. *
 * @dt - to assign
 * @return - Whether mapping succeeded or not
 */
enum MSGFMT sw_str_to_date(const char buffer[15], DateTime* dt);
/**
 * Converts the given DateTime struct into its string representation.
 * @dt - to map
 * @buffer - to insert
 * @return - Whether mapping succeeded or not
 */
enum MSGFMT sw_date_to_str(const DateTime* dt, char buffer[15]);

typedef struct {
    char* name;
    char* desc;
    char* location;
    DateTime start;
    DateTime end;
} Event;

/**
 * Constructs an event using the given buffer.
 *
 * String Format:  start|end|name|desc|location
 * @str - Event string to parse. Modifies the original buffer while
 * generating the event.
 * @return Returns an error code for any failure.
 */
enum MSGFMT sw_event_new(Event* event, char* str, size_t str_s);

/******************************************************************************
                                Request
*****************************************************************************/
// Describes the request send by the connected device.

/**
 * Common types for request and response type enums:
 * # Option: Used on custom buttons. Payload is separated by '|' symbol.
 *   Example:
 *   - Accept|Deny
 *   - Previous|Stop|Next
 *
 */
enum SW_REQ {
    SW_REQ_CONNECT, /* Connection request. Payload: "{DateTime}" */
    SW_REQ_DISCONNECT,
    SW_REQ_NOTIFY,     /* Notification event. Triggers an interrupt.
                        * Payload: App Name\nTitle\nMessage\nOption{Option} */
    SW_REQ_CALL_BEGIN, /* Call start event. Payload: "Caller" */
    SW_REQ_CALL_EXIT,  /* Occurs when the Call event is handled by the phone. */
    SW_REQ_PING, /* Client periodically asks to server, requested data. Every sec */
    SW_REQ_SYNC_EVENT,  /* Calendar events info. Sends a list of events.
                        * Payload: "Name|{DateTime}|{DateTime}|Desc|Location"*/
    SW_REQ_SYNC_BAT,    /* Sends phone's battery information. 
                        * Payload: "Pct\nStatus" */
    SW_REQ_SYNC_NOTIFY, /* Sends all notifications exist on Android lock screen.
                         * Payload: "Title\nMessage\n{Option}"*/
    SW_REQ_OSC, /* On Song Change Event. Payload: "Song\nAlbum\nArtist" */

    SW_REQ_SIZE, /* Number of options in SW_REQ_TYPE */
};

#define SW_PARAM_MAX_SIZE 4
/*
 * Notify parameters are pointers to the payload addresses after all \n
 * characters are set to 0.
 *
 * Maximum number of notification option can be no higher than SW_PARAM_MAX_SIZE
 *
* String Format:
* title\n
* message\n
* option_s\n
* option_1\n
* option_2\n
* ...
* option_{option_s}
*/
typedef struct {
    char* app;
    char* title;
    char* message;
    /* Option array */
    char* option[SW_PARAM_MAX_SIZE];
    int32_t option_s;
} SwReqNotify;

enum MSGFMT sw_param_notify_new(SwReqNotify* notify, char* str, size_t str_s);

typedef struct {
    Event* list;
    int32_t size;
} SwReqEvent;

enum MSGFMT sw_event_list_new(SwReqEvent* events, char* str, size_t str_s);
void sw_event_list_free(SwReqEvent* events);

typedef struct {
    float pct;
    bool is_ac;
} SwReqBat;

enum MSGFMT sw_bat_new(SwReqBat* bat, char* str, size_t str_s);

/* Media On Song Change parameters are pointers to the payload addresses after
 * all \n characters are set to 0 */
typedef struct {
    char* song;
    char* album;
    char* artist;
} SwReqOSC;

enum MSGFMT sw_osc_new(SwReqOSC* osc, char* str, size_t str_s);

typedef struct {
    uint16_t __magic;
    enum SW_REQ req_t;
    union {
        DateTime connect;
        SwReqNotify notify;
        char caller[30];
        SwReqEvent events;
        SwReqBat bat;
        SwReqOSC osc;
    } pl;
    char* _raw;
    size_t _raw_s;
} SwRequest;

/**
 * Parses a raw request, request contains a text in the following format
 * 00000\n <- __magic
 * 11111\n <_ SW_REQ_TYPE
 * ... <- PAYLOAD
 */
enum MSGFMT sw_parse_request(SwRequest* req, char* msg, size_t msg_s);

/******************************************************************************
                                Response
*****************************************************************************/
// Describes the response sent back to the connected device.

enum SW_RESP {
    SW_RESP_NOTIFY,       /* Notification response. Payload: {Index} */
    SW_RESP_CALL_ACCEPT,  /* Accept the call */
    SW_RESP_CALL_DISMISS, /* Reject the call */
    SW_RESP_SYNC,         /* Ask for sync. Payload: EVENT|BAT|NOTIFY|*/
    SW_RESP_MDI_NEXT,     /* Skip to the next song */
    SW_RESP_MDI_PLYPS,    /* Play or pause */
    SW_RESP_MDI_PREV,     /* Skip to the previous song */

    SW_RESP_SIZE, /* Number of options in SW_RESP_TYPE */
};

typedef struct {
    uint16_t __magic;
    enum SW_RESP resp_t;
    char* payload;
    size_t payload_s;
} SwResponse;

#endif
