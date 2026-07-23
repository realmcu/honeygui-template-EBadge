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
uint8_t bd_idx_array[BD_NUM_MAX] = {0, 1, 2};
uint8_t bd_dev_num = 3;

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
    if (dev_mode == MODE_DEFAULT) return;
    gui_obj_t *parent = ((gui_obj_t *)obj)->parent;
    gui_obj_child_free(parent);
    gui_view_create(parent, "ShareConnView", 0, 0, 0, 0);
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
        gui_log("hmi_ble_central_start_scan\n");
#endif
    }

    gui_obj_add_event_cb(view, (gui_event_cb_t)re_scan_dev, GUI_EVENT_TOUCH_CLICKED, NULL);
}

void switch_out_share_view(gui_view_t *view)
{
    GUI_UNUSED(view);
    dev_mode = MODE_DEFAULT;
}

void click_2_conn_dev_by_idx(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    
#ifdef _HONEYGUI_SIMULATOR_
    dev_mode = MODE_SHARE;
    gui_view_switch_direct(gui_view_get_current(), "view_mainface_list", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
#else
    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index;
    extern bool hmi_ble_central_connect(uint8_t idx);
    bool res = hmi_ble_central_connect(bd_idx_array[index]);
    if (res)
    {
        dev_mode = MODE_SHARE;
        gui_view_switch_direct(gui_view_get_current(), "view_mainface_list", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
    }
    else
    {
        gui_log("hmi_ble_central_connect %d failed\n", bd_idx_array[index]);
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
