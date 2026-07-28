#include "share_user.h"
#include "gui_rect.h"
#include "gui_text.h"
#include "gui_list.h"

/**
 * User-defined implementation
 * This file is generated once only, feel free to modify
 */

// Add custom implementations here

/***
 * Template function
 * Distinguish development environments
 */
// void user_defined_func_called_by_event(void *obj, gui_event_t *e)
// {
//     GUI_UNUSED(obj);
//     GUI_UNUSED(e);
// #ifdef _HONEYGUI_SIMULATOR_
//     // TODO
// #else
//     // TODO
// #endif
// }

// void user_defined_func_called_by_msg(gui_obj_t *obj, const char *topic, void *data, uint16_t len)
// {
//     GUI_UNUSED(obj);
//     GUI_UNUSED(topic);
//     GUI_UNUSED(data);
//     GUI_UNUSED(len);
// #ifdef _HONEYGUI_SIMULATOR_
//     // TODO
// #else
//     // TODO
// #endif
// }

// void list_note_design(gui_obj_t *obj, void *param)
// {
//     GUI_UNUSED(param);
//     // Cast obj to gui_list_note_t * type
//     gui_list_note_t *note = (gui_list_note_t *)obj;
//     uint16_t index = note->index;
//     GUI_UNUSED(index);
// }

/* ----------------------------------------------------*/
char bd_addr_array[BD_NUM_MAX][20] = 
{
    "00:10:20:30:40:50",
    "00:10:20:30:40:51",
    "00:10:20:30:40:52",
};
static char bd_addr_str[20] = "11:22:33:44:55:66";
uint8_t bd_dev_num = 3;

#ifndef _HONEYGUI_SIMULATOR_
/* B: after tapping a device we only *initiate* the connection.  Wait until the
 * central actually reaches READY (link up + HMI service discovered + notify
 * enabled) before entering the file list, instead of jumping there the moment
 * connect() is issued -- otherwise the UI let the user "send" over a not-ready
 * link, the send got rejected, and the progress arc froze at 0. */
#define CONN_READY_POLL_MS       100
#define CONN_READY_TIMEOUT_TICKS 80    /* 80 * 100ms = 8s */
static uint16_t s_conn_wait_ticks = 0;

static void conn_ready_poll_cb(void *obj)
{
    extern bool hmi_ble_central_is_ready(void);
    extern bool hmi_ble_central_is_active(void);

    s_conn_wait_ticks++;

    if (hmi_ble_central_is_ready())
    {
        gui_obj_stop_timer((gui_obj_t *)obj);
        is_dev_connect = true;
        gui_log("central READY -> enter file list\n");
        gui_view_switch_direct(gui_view_get_current(), "view_mainface_list",
                               SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        return;
    }

    /* Link never came up / dropped back to IDLE (e.g. controller rejected with
     * cause 0x109 = max connections), or discovery stalled past the timeout:
     * give up and stay on the device list so the user can tap to retry. */
    if (!hmi_ble_central_is_active() || s_conn_wait_ticks >= CONN_READY_TIMEOUT_TICKS)
    {
        gui_obj_stop_timer((gui_obj_t *)obj);
        is_dev_connect = false;
        gui_log("central connect failed/timeout (active=%d, ticks=%d), stay on dev list\n",
                hmi_ble_central_is_active(), s_conn_wait_ticks);
        /* Deliberately no view switch: SelectDevView stays up for a retry tap. */
        return;
    }
}
#endif

void click_share_image_button(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    dev_mode = MODE_SHARE;
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
}

void click_receive_image_button(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    dev_mode = MODE_RECEIVE;
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
}

static void re_scan_dev(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    if (dev_mode == MODE_DEFAULT)
    {
        gui_obj_t *parent = ((gui_obj_t *)obj)->parent;
        gui_obj_child_free(parent);
        dev_mode = MODE_SHARE;
        gui_view_create(parent, "ShareConnView", 0, 0, 0, 0);
    }
}

void switch_in_share_view(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (dev_mode == MODE_SHARE)
    {
        gui_obj_create_timer((gui_obj_t *)win_share, 10, true, win_share_timer_0_cb);
#ifndef _HONEYGUI_SIMULATOR_
        extern bool hmi_ble_central_start_scan(void);
        hmi_ble_central_start_scan(); 
        gui_log("hmi_ble_central_start_scan....\n");
#endif
    }

#ifndef _HONEYGUI_SIMULATOR_
    extern bool hmi_ble_gap_get_local_addr(uint8_t bd_addr[6]);
    uint8_t bd_local_addr[6];
    hmi_ble_gap_get_local_addr(bd_local_addr);
    sprintf(bd_addr_str, "%02x:%02x:%02x:%02x:%02x:%02x", bd_local_addr[5]&0xff, bd_local_addr[4]&0xff, bd_local_addr[3]&0xff,bd_local_addr[2]&0xff, bd_local_addr[1]&0xff, bd_local_addr[0]&0xff);

    // extern bool hmi_ble_gap_get_local_name(char *buf, uint8_t buf_len);
    // char local_name[32];
    // hmi_ble_gap_get_local_name(local_name, sizeof(local_name));
    // gui_log("local name %s\n", local_name);
    // gui_log("local addr %02x:%02x:%02x:%02x:%02x:%02x\n", bd_local_addr[5]&0xff, bd_local_addr[4]&0xff, bd_local_addr[3]&0xff,bd_local_addr[2]&0xff, bd_local_addr[1]&0xff, bd_local_addr[0]&0xff);
#endif
    gui_text_content_set(bd_addr_self, bd_addr_str, strlen(bd_addr_str));

    gui_obj_add_event_cb(view, (gui_event_cb_t)re_scan_dev, GUI_EVENT_TOUCH_CLICKED, NULL);
}

void switch_out_share_view(gui_view_t *view)
{
    GUI_UNUSED(view);

// #ifndef _HONEYGUI_SIMULATOR_
//     bool hmi_ble_central_stop_scan(void);
//     hmi_ble_central_stop_scan();
// #endif
    // gui_log("hmi_ble_central_stop_scan....\n");
    dev_mode = MODE_DEFAULT;
}

void click_2_conn_dev_by_idx(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    
#ifdef _HONEYGUI_SIMULATOR_
    dev_mode = MODE_SHARE;
    is_dev_connect = true;
    gui_view_switch_direct(gui_view_get_current(), "view_mainface_list", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
#else
    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index;

    extern bool hmi_ble_central_connect(uint8_t idx);
    bool res = hmi_ble_central_connect(index);
    if (res)
    {
        gui_log("hmi_ble_central_connect initiated, wait for READY...\n");
        dev_mode = MODE_SHARE;
        /* B: connect() only *started* the link.  Do NOT enter the file list yet
         * -- poll on this (SelectDev) view until CEN_READY, then advance.  On
         * failure/timeout conn_ready_poll_cb stays here for a retry tap. */
        s_conn_wait_ticks = 0;
        gui_obj_create_timer((gui_obj_t *)gui_view_get_current(),
                             CONN_READY_POLL_MS, true, conn_ready_poll_cb);
        gui_obj_start_timer((gui_obj_t *)gui_view_get_current());
    }
    else
    {
        gui_log("hmi_ble_central_connect %d failed\n", index);
    }
#endif
}

static void list_note_design(gui_obj_t *obj, void *param)
{
    GUI_UNUSED(param);
    
    // Cast obj to gui_list_note_t * type
    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index % bd_dev_num;
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;

    gui_rect_create((gui_obj_t *)obj, 0, screen_size / 8, note->base.h, screen_size * 3/4, 4, 0, gui_rgb(97, 103, 107));

    gui_text_t *text = gui_text_create((gui_obj_t *)obj, 0, 0, 0, screen_size, note->base.h);
    gui_text_set(text, bd_addr_array[index], GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255), strlen(bd_addr_array[index]), 24);
    gui_text_type_set(text, "/font/Inter_24pt_SemiBold_size24_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set(text, MID_CENTER);

    gui_obj_add_event_cb(obj, (gui_event_cb_t)click_2_conn_dev_by_idx, GUI_EVENT_TOUCH_CLICKED, NULL);
}

void switch_in_select_dev_view(gui_view_t *view)
{
    GUI_UNUSED(view);
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    gui_list_t *list = gui_list_create((gui_obj_t *)view, 0, 0, screen_size / 6, screen_size, screen_size, 
                                screen_size / 6, 5, VERTICAL, list_note_design, NULL, false);
    gui_list_set_style(list, LIST_CLASSIC);
    gui_list_set_note_num(list, bd_dev_num);
    gui_list_set_auto_align(list, true);
    gui_list_enable_loop(list, false);
}

void switch_out_select_dev_view(gui_view_t *view)
{
    GUI_UNUSED(view);
    gui_view_t *view_c = gui_view_get_current();
    if (strcmp(view_c->base.name, "view_mainface_list") != 0)
    {
        dev_mode = MODE_DEFAULT;
    }
}
