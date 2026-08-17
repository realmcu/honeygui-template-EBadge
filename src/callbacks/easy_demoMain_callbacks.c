#include "easy_demoMain_callbacks.h"
#include "../ui/easy_demoMain_ui.h"
#include "../user/easy_demoMain_user.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Timer animation counters
uint16_t top_view_timer_cnt = 0;
uint16_t bg_circle_timer_cnt = 0;
uint16_t icon_bat_timer_cnt = 0;
uint16_t lbl_1_timer_cnt = 0;
uint16_t view_fl_timer_cnt = 0;

// Event callback function implementations

void easy_demoMainView_key_0_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    // Check key name
    if (strcmp(e->indev_name, "Menu") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "bt_View", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
    }
    else if (strcmp(e->indev_name, "Power") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "menuMainView", SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
    }
}

void mainface_view_1_key_0_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    // Check key name
    if (strcmp(e->indev_name, "Menu") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "bt_View", SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
    }
    else if (strcmp(e->indev_name, "Power") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "menuMainView", SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
    }
}

void mainface_view_2_key_0_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    // Check key name
    if (strcmp(e->indev_name, "Menu") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "bt_View", SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
    }
    else if (strcmp(e->indev_name, "Power") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "menuMainView", SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
    }
}

void top_view_key_0_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    // Check key name
    if (strcmp(e->indev_name, "Menu") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "bt_View", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
    }
}

void top_view_key_1_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    // Check key name
    if (strcmp(e->indev_name, "Power") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "menuMainView", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
    }
}

void icon_del_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    click_delete_icon(obj, e);
}

void icon_fl_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    gui_view_switch_direct(gui_view_get_current(), "view_fl", SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
}

void icon_connect_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    click_share_icon(obj, e);
}

void icon_sl_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    click_screen_light_icon(obj, e);
}

void icon_as_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    click_auto_sleep_icon(obj, e);
}

void icon_cam_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    gui_view_switch_direct(gui_view_get_current(), "view_cam_ctl", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
}

void img_18_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    click_camera_shutter(obj, e);
}

void img_1x_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    click_camera_1x(obj, e);
}

void img_2x_clicked_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    click_camera_2x(obj, e);
}

void view_fl_key_0_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    // Check key name
    if (strcmp(e->indev_name, "Menu") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
    }
    else if (strcmp(e->indev_name, "Power") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
    }
}

// Preset timer callback functions

/**
 * 动画 1
 * Component: top_view
 */
void top_view_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);
    // Call the implementation function in protected area (if exists)
    // Define top_view_timer_0_cb_impl() in custom_functions protected area for custom logic
#ifdef __cplusplus
    extern "C" {
#endif
    extern void top_view_timer_0_cb_impl(void) __attribute__((weak));
#ifdef __cplusplus
    }
#endif
    
    if (top_view_timer_0_cb_impl) {
        top_view_timer_0_cb_impl();
    } else {
        // TODO: Implement timer callback logic
        // Or define top_view_timer_0_cb_impl() in custom_functions protected area
    }
}

/* @protected start custom_functions */
// Custom functions
#include "tp_algo.h"

void view_cam_ctl_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);
    // gui_obj_focus_set(GUI_BASE(obj));
    if (is_bt_connect)
    {
//        gui_img_set_opacity((gui_img_t *)icon_cam_ctl, 255);
//        gui_obj_hidden(GUI_BASE(lbl_12), true);
    }
    else
    {
//        gui_img_set_opacity((gui_img_t *)icon_cam_ctl, 75);
//        gui_obj_hidden(GUI_BASE(lbl_12), false);
    }

    touch_info_t *tp = tp_get_info();
    switch (tp->type)
    {
    case TOUCH_LEFT_SLIDE_QUICK:
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    case TOUCH_RIGHT_SLIDE_QUICK:
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    case TOUCH_LEFT_SLIDE:
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    case TOUCH_RIGHT_SLIDE:
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    default:
        break;
    }

}

void view_fl_timer_1_cb(void *obj)
{
    GUI_UNUSED(obj);
    touch_info_t *tp = tp_get_info();
    switch (tp->type)
    {
    case TOUCH_UP_SLIDE:
        fl_color_idx = (fl_color_idx + 1) % 8;
        set_flashlight_color(obj);
        break;
    case TOUCH_DOWN_SLIDE:
        fl_color_idx = (fl_color_idx - 1 + 8) % 8;
        set_flashlight_color(obj);
        break;
    case TOUCH_UP_SLIDE_QUICK:
        fl_color_idx = (fl_color_idx + 1) % 8;
        set_flashlight_color(obj);
        break;
    case TOUCH_DOWN_SLIDE_QUICK:
        fl_color_idx = (fl_color_idx - 1 + 8) % 8;
        set_flashlight_color(obj);
        break;
    case TOUCH_LEFT_SLIDE_QUICK:
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    case TOUCH_RIGHT_SLIDE_QUICK:
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    case TOUCH_LEFT_SLIDE:
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    case TOUCH_RIGHT_SLIDE:
        gui_view_switch_direct(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    default:
        break;
    }
}

void view_fl_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);
    set_flashlight_color(obj);
    gui_obj_create_timer(obj, 1, true, view_fl_timer_1_cb);
    gui_obj_start_timer(obj);
}

void bg_circle_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);
    gui_obj_focus_set(GUI_BASE(obj)->parent);
    is_displaying_mainface = false;
}

void lbl_1_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);
    gui_obj_hidden(obj, !is_auto_sleep_mode);
    gui_obj_stop_timer(obj);
}

void icon_bat_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);

#ifdef _HONEYGUI_SIMULATOR_
    soc_val--;
    if (soc_val == 0) soc_val = 100;
#endif
    void *img_src = "/image/A8/bat_100_icon.bin";
    switch (soc_val / 20)
    {
    case 0:
        img_src = "/image/A8/bat_20_icon.bin";
        break;
    case 1:
        img_src = "/image/A8/bat_40_icon.bin";
        break;
    case 2:
        img_src = "/image/A8/bat_60_icon.bin";
        break;
    case 3:
        img_src = "/image/A8/bat_80_icon.bin";
        break;
    
    default:
        break;
    }
    gui_img_set_src(icon_bat, img_src, IMG_SRC_FILESYS);
}
/* @protected end custom_functions */
