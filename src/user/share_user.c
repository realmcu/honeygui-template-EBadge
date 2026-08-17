#include "share_user.h"

#include <stdio.h>
#include <string.h>

#include "gui_img.h"
#include "gui_list.h"
#include "gui_rect.h"
#include "gui_text.h"
#include "gui_win.h"

/**
 * User-defined sharing view implementation.
 *
 * This file is generated only once and can be modified freely.
 */

#ifndef _HONEYGUI_SIMULATOR_
#define CONN_READY_POLL_MS       20
#define CONN_READY_TIMEOUT_TICKS 400 /* 400 * 20 ms = 8 s */

extern bool hmi_ble_central_is_ready(void);
extern bool hmi_ble_central_is_active(void);
extern bool hmi_ble_central_start_scan(void);
extern bool hmi_ble_central_connect(uint8_t idx);
extern bool hmi_ble_gap_get_local_addr(uint8_t bd_addr[6]);
#endif

/*============================================================================*
 * Device state
 *============================================================================*/

char bd_addr_array[BD_NUM_MAX][20] =
{
    "00:10:20:30:40:50",
    "00:10:20:30:40:51",
    "00:10:20:30:40:52",
    "00:10:20:30:40:50",
    "00:10:20:30:40:51",
    "00:10:20:30:40:52",
};
uint8_t bd_dev_num = 6;

static char bd_addr_str[20] = "11:22:33:44:55:66";
static bool is_connecting = false;

#ifndef _HONEYGUI_SIMULATOR_
static uint16_t conn_wait_ticks = 0;
#endif

/*============================================================================*
 * File-local functions
 *============================================================================*/

/* Animate the connection indicator between -45 and 45 degrees. */
static void wait_conn_animation(gui_img_t *img)
{
    static uint8_t count = 0;
    const uint16_t segment_ticks = 15;
    const uint16_t total_ticks = segment_ticks * 2;
    float angle_origin;
    float angle_target;
    uint16_t segment_count;

    count++;
    if (count <= segment_ticks)
    {
        angle_origin = -45.0f;
        angle_target = 45.0f;
        segment_count = count;
    }
    else
    {
        angle_origin = 45.0f;
        angle_target = -45.0f;
        segment_count = count - segment_ticks;
    }

    float angle = angle_origin +
                  (angle_target - angle_origin) * segment_count / segment_ticks;
    gui_img_rotation(img, angle);

    if (count >= total_ticks)
    {
        count = 0;
    }
}

#ifdef _HONEYGUI_SIMULATOR_
/* Simulate a successful connection after the loading animation. */
static void conn_ready_poll_cb(void *obj)
{
    static uint8_t count = 0;
    gui_img_t *img = (gui_img_t *)gui_list_entry(
                         GUI_BASE(obj)->child_list.prev, gui_obj_t, brother_list);

    count++;
    wait_conn_animation(img);
    if (count < 100)
    {
        return;
    }

    count = 0;
    gui_obj_stop_timer(obj);
    dev_mode = MODE_SHARE;
    is_dev_connect = true;
    is_connecting = false;
    gui_view_switch_direct(gui_view_get_current(), "view_mainface_list",
                           SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
}
#else
/*
 * Wait until the central link and HMI service are ready before opening the
 * mainface list. Keep the device list visible when connection setup fails.
 */
static void conn_ready_poll_cb(void *obj)
{
    gui_img_t *img = (gui_img_t *)gui_list_entry(
                         GUI_BASE(obj)->child_list.prev, gui_obj_t, brother_list);

    conn_wait_ticks++;
    wait_conn_animation(img);

    if (hmi_ble_central_is_ready())
    {
        gui_obj_stop_timer((gui_obj_t *)obj);
        is_dev_connect = true;
        is_connecting = false;
        gui_log("central READY -> enter file list\n");
        gui_view_switch_direct(gui_view_get_current(), "view_mainface_list",
                               SWITCH_OUT_NONE_ANIMATION,
                               SWITCH_IN_NONE_ANIMATION);
        return;
    }

    if (!hmi_ble_central_is_active() ||
        conn_wait_ticks >= CONN_READY_TIMEOUT_TICKS)
    {
        gui_obj_stop_timer((gui_obj_t *)obj);
        is_dev_connect = false;
        is_connecting = false;
        gui_obj_tree_free_async(obj);
        gui_list_enable_scroll(lst_bd, true);
        gui_log("central connect failed/timeout (active=%d, ticks=%d), "
                "stay on dev list\n",
                hmi_ble_central_is_active(), conn_wait_ticks);
    }
}
#endif

/* Create the modal loading overlay used while establishing a connection. */
static void create_connection_overlay(gui_view_t *view, uint16_t screen_size,
                                      uint32_t timer_period_ms)
{
    gui_win_t *win = gui_win_create(view, 0, 0, 0, 0, 0);
    gui_img_t *img = gui_img_create_from_fs(
                         win, 0, "/image/A8/circle_360_bg.bin", 0, 0, 0, 0);
    gui_img_set_mode(img, IMG_SRC_OVER_MODE);
    gui_img_set_opacity(img, 122);

    img = gui_img_create_from_fs(
              win, 0, "/image/circle_anime.bin", 0, 0, 0, 0);
    gui_img_set_mode(img, IMG_SRC_OVER_MODE);
    gui_img_set_focus(img, img->base.w / 2, img->base.h / 2);
    gui_img_translate(img, img->base.w / 2, img->base.h / 2);
    gui_obj_move(GUI_BASE(img), screen_size / 2 - img->base.w / 2,
                 screen_size / 2 - img->base.h / 2);
    gui_img_set_quality(img, true);

    gui_obj_create_timer((gui_obj_t *)win, timer_period_ms, true,
                         conn_ready_poll_cb);
    gui_obj_start_timer((gui_obj_t *)win);
}

/* Rebuild the device connection view when the idle page is tapped. */
static void rescan_device_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(e);

    if (dev_mode != MODE_DEFAULT)
    {
        return;
    }

    gui_obj_t *parent = ((gui_obj_t *)obj)->parent;
    gui_obj_child_free(parent);
    dev_mode = MODE_SHARE;
    gui_view_create(parent, "ShareConnView", 0, 0, 0, 0);
}

/* Start connecting to the device represented by the selected list entry. */
static void connect_device_by_index_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(e);

    if (is_connecting)
    {
        return;
    }

    is_connecting = true;
    gui_list_enable_scroll(lst_bd, false);

    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    gui_view_t *view = gui_view_get_current();

#ifdef _HONEYGUI_SIMULATOR_
    GUI_UNUSED(obj);
    create_connection_overlay(view, screen_size, 10);
#else
    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index;

    if (hmi_ble_central_connect(index))
    {
        gui_log("hmi_ble_central_connect initiated, wait for READY...\n");
        dev_mode = MODE_SHARE;
        conn_wait_ticks = 0;
        create_connection_overlay(view, screen_size, CONN_READY_POLL_MS);
    }
    else
    {
        is_connecting = false;
        gui_list_enable_scroll(lst_bd, true);
        gui_log("hmi_ble_central_connect %d failed\n", index);
    }
#endif
}

/* Handle Menu and Home keys while the device list owns input focus. */
static void select_dev_view_key_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);

    if (is_connecting)
    {
        return;
    }

    if (strcmp(e->indev_name, "Menu") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "top_view",
                               SWITCH_OUT_NONE_ANIMATION,
                               SWITCH_IN_NONE_ANIMATION);
    }
    else if (strcmp(e->indev_name, "Home") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "easy_demoMainView",
                               SWITCH_OUT_NONE_ANIMATION,
                               SWITCH_IN_NONE_ANIMATION);
    }
}

/* Return to the top view on a quick horizontal slide. */
static void select_dev_view_slide_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    if (is_connecting)
    {
        return;
    }

    gui_view_switch_direct(gui_view_get_current(), "top_view",
                           SWITCH_OUT_NONE_ANIMATION,
                           SWITCH_IN_NONE_ANIMATION);
}

/*============================================================================*
 * Public callbacks and APIs
 *============================================================================*/

void switch_in_share_view(gui_view_t *view)
{
    if (dev_mode == MODE_SHARE)
    {
        gui_obj_create_timer((gui_obj_t *)win_share, 10, true,
                             win_share_timer_0_cb);
#ifndef _HONEYGUI_SIMULATOR_
        hmi_ble_central_start_scan();
        gui_log("hmi_ble_central_start_scan....\n");
#endif
    }

#ifndef _HONEYGUI_SIMULATOR_
    uint8_t local_addr[6];
    hmi_ble_gap_get_local_addr(local_addr);
    snprintf(bd_addr_str, sizeof(bd_addr_str),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             local_addr[5], local_addr[4], local_addr[3],
             local_addr[2], local_addr[1], local_addr[0]);
#endif

    gui_text_content_set(bd_addr_self, bd_addr_str, strlen(bd_addr_str));
    gui_obj_add_event_cb(view, (gui_event_cb_t)rescan_device_cb,
                         GUI_EVENT_TOUCH_CLICKED, NULL);
}

void switch_out_share_view(gui_view_t *view)
{
    GUI_UNUSED(view);
    dev_mode = MODE_DEFAULT;
}

void list_bd_note_design(gui_obj_t *obj, void *param)
{
    GUI_UNUSED(param);

    gui_list_note_t *note = (gui_list_note_t *)obj;
    int16_t index = note->index % (bd_dev_num + 1);
    if (index == 0)
    {
        return;
    }
    index--;

    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;

    gui_rect_create(obj, 0, screen_size / 8, note->base.h,
                    screen_size * 3 / 4, 4, 0, gui_rgb(97, 103, 107));

    gui_text_t *text = gui_text_create(obj, 0, 0, 0,
                                       screen_size, note->base.h);
    gui_text_set(text, bd_addr_array[index], GUI_FONT_SRC_BMP,
                 gui_rgb(255, 255, 255), strlen(bd_addr_array[index]), 24);
    gui_text_type_set(text,
                      "/font/Inter_24pt_SemiBold_size24_bits4_bitmap.bin",
                      FONT_SRC_FILESYS);
    gui_text_mode_set(text, MID_CENTER);

    gui_obj_add_event_cb(obj, (gui_event_cb_t)connect_device_by_index_cb,
                         GUI_EVENT_TOUCH_CLICKED, NULL);
}

void switch_in_select_dev_view(gui_view_t *view)
{
    is_connecting = false;

    /* Add one spacer entry at both the top and bottom of the list. */
    gui_list_set_note_num(lst_bd, bd_dev_num + 2);
    gui_obj_add_event_cb((gui_obj_t *)view,
                         (gui_event_cb_t)select_dev_view_slide_cb,
                         GUI_EVENT_TOUCH_LEFT_SLIDE_QUICK, NULL);
    gui_obj_add_event_cb((gui_obj_t *)view,
                         (gui_event_cb_t)select_dev_view_slide_cb,
                         GUI_EVENT_TOUCH_RIGHT_SLIDE_QUICK, NULL);
    gui_obj_add_event_cb((gui_obj_t *)view,
                         (gui_event_cb_t)select_dev_view_key_cb,
                         GUI_EVENT_KB_SHORT_PRESSED, NULL);
    gui_obj_focus_set((gui_obj_t *)view);
}

void switch_out_select_dev_view(gui_view_t *view)
{
    GUI_UNUSED(view);

    gui_view_t *current_view = gui_view_get_current();
    if (strcmp(current_view->base.name, "view_mainface_list") != 0)
    {
        dev_mode = MODE_DEFAULT;
    }
}

void click_share_image_button(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    if (mainface_num == 0)
    {
        return;
    }

    dev_mode = MODE_SHARE;
    gui_view_switch_direct(gui_view_get_current(), "ShareConnView",
                           SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
}

void click_receive_image_button(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    dev_mode = MODE_RECEIVE;
    gui_view_switch_direct(gui_view_get_current(), "ShareConnView",
                           SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
}
