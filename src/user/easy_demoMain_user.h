#ifndef EASY_DEMOMAIN_USER_H
#define EASY_DEMOMAIN_USER_H

#include "../callbacks/easy_demoMain_callbacks.h"
#include "../ui/easy_demoMain_ui.h"

/**
 * User-defined header file
 * This file is generated once only, feel free to modify
 */


// Add custom declarations here
#define  MAINFACE_NUM_MAX  10
#define  BD_NUM_MAX        10

#define COLOR_TRANSITION_SRGB          0
#define COLOR_TRANSITION_GAMMA_LINEAR  1
#define COLOR_TRANSITION_HSL           2
#define COLOR_TRANSITION_CIELAB        3
#define COLOR_TRANSITION_OKLAB         4

#ifndef COLOR_TRANSITION_METHOD
#define COLOR_TRANSITION_METHOD COLOR_TRANSITION_GAMMA_LINEAR
#endif

#define COLOR_EASING_LINEAR             0
#define COLOR_EASING_SMOOTHSTEP         1
#define COLOR_EASING_EASE_IN_OUT_CUBIC  2
#define COLOR_EASING_EASE_OUT_CUBIC     3

#ifndef COLOR_TRANSITION_EASING
#define COLOR_TRANSITION_EASING COLOR_EASING_EASE_IN_OUT_CUBIC
#endif


typedef enum
{
    SRC_IMG = 0,
    SRC_VIDEO,
    SRC_3D,
    SRC_IMG_SPATIAL,
    SRC_DANMU,
} MAINFACE_SRC_TYPE;

#pragma pack(push, 1)  
typedef struct 
{
    uint8_t type;
    uint8_t num;
    uint32_t color;
    uint32_t size;
    uint32_t offset[];
}PACKET_HEADER_T;
#pragma pack(pop)

typedef struct mainface_src
{
    void *data;
    MAINFACE_SRC_TYPE type;
    void *raw;
    void *img_preview;
    uint32_t color;
} mainface_src_t;

#define RES_TYPE(raw) (((PACKET_HEADER_T *)raw)->type)
#define RES_COLOR_BG(raw) (((PACKET_HEADER_T *)raw)->color)
#define RES_NUM(raw) (((PACKET_HEADER_T *)raw)->num)
#define RES_SIZE(raw) (((PACKET_HEADER_T *)raw)->size)
#define RES_OFFSET_X(raw, n) (uint32_t)(((PACKET_HEADER_T *)raw)->offset[n])
#define RES_DATA_X(raw, n) (void*)((uint8_t*)raw + RES_OFFSET_X(raw, n))

typedef enum
{
    ADD_MAINFACE = 0,
    BT_ON,
    BT_OFF,
    TRANSMIT_START,
    TRANSMIT_ABORT,
    SWITCH_LEFT_MAINFACE,
    SWITCH_RIGHT_MAINFACE,
    CAST_START,
    CAST_STOP,
    CONNECT_DEV,
    DISCONNECT_DEV,

} UI_SUBEVENT_TYPE;

typedef enum
{
    MODE_DEFAULT = 0,
    MODE_DELETE,
    MODE_SHARE,
    MODE_RECEIVE,
} MODE_TYPE;

extern uint8_t mainface_idx;
extern uint8_t mainface_num;
extern mainface_src_t mainface_list[MAINFACE_NUM_MAX];

extern bool is_auto_sleep_mode;
extern bool is_bt_connect;
extern bool is_dev_connect;
extern bool is_displaying_mainface;
extern MODE_TYPE dev_mode;

extern uint8_t soc_val;
extern uint8_t screen_light_idx;
extern int8_t fl_color_idx; //white, red, orange, yellow, green, blue, indigo, violet

extern char bd_addr_array[BD_NUM_MAX][20];
extern uint8_t bd_dev_num;

void switch_mainface(gui_obj_t *parent, uint8_t idx);
void set_flashlight_color(void *obj);

void win_timer_0_cb(void *obj); // move long image
void click_auto_sleep_icon(void *obj, gui_event_t *e);
void click_screen_light_icon(void *obj, gui_event_t *e);
void click_share_icon(void *obj, gui_event_t *e);
void click_delete_icon(void *obj, gui_event_t *e);
void click_delete_icon_detail(void *obj, gui_event_t *e);
void click_back_icon(void *obj, gui_event_t *e);
void click_camera_ctl_icon(void *obj, gui_event_t *e);

void switch_in_mainface_0(gui_view_t *view);
void switch_in_mainface_1(gui_view_t *view);
void switch_in_mainface_2(gui_view_t *view);
void switch_in_mainface_3(gui_view_t *view);
void switch_in_mainface_4(gui_view_t *view);
void switch_in_mainface_5(gui_view_t *view);
void switch_in_mainface_6(gui_view_t *view);
void switch_in_mainface_7(gui_view_t *view);
void switch_in_mainface_8(gui_view_t *view);
void switch_in_mainface_9(gui_view_t *view);
void switch_in_mainface_list(gui_view_t *view);

/* Interact api */ 
uint8_t mainface_list_init(void **data_list, uint32_t n); //ret: user's mainface num
void ui_process_msg(void *arg);

/* Live-video stream transport accessor.  Implemented in
 * app/example_gui_stream.c; the gui_stream widget (streaming_1) binds to the
 * transport it returns, shared with the BLE producer (hmi_stream_ctrl.c). */
stp_transport_t *gui_stream_transport_get(void);


#endif // EASY_DEMOMAIN_USER_H
