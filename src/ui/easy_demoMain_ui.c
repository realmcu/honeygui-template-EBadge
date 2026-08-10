/**
 * easy_demoMain UI Implementation (Auto-generated, do not modify manually)
 */
#include "easy_demoMain_ui.h"
#include "../callbacks/easy_demoMain_callbacks.h"
#include "../user/easy_demoMain_user.h"
#include <stddef.h>

// Component handle definitions
gui_img_t *bg_circle = NULL;
gui_img_t *icon_del = NULL;
gui_img_t *icon_fl = NULL;
gui_img_t *icon_connect = NULL;
gui_img_t *icon_bat = NULL;
gui_img_t *icon_sl = NULL;
gui_img_t *icon_as = NULL;
gui_img_t *icon_cam = NULL;
gui_text_t *lbl_1 = NULL;
gui_stream_t *streaming_1 = NULL;
gui_img_t *img_shutter = NULL;
gui_img_t *img_19 = NULL;
gui_img_t *img_1x = NULL;
gui_img_t *img_2x = NULL;
gui_list_t *lst_mainface = NULL;

// List component note_design callback functions

// Create easy_demoMainView (hg_view)
static void easy_demoMainView_switch_out(gui_view_t *view)
{
    switch_out_mainface(view);
}

static void easy_demoMainView_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 36);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));


    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)easy_demoMainView_key_0_cb, GUI_EVENT_KB_SHORT_PRESSED, NULL);
    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)easy_demoMainView_key_1_cb, GUI_EVENT_KB_LONG_PRESSED, NULL);
    gui_obj_focus_set((gui_obj_t *)view);

    switch_in_mainface_0(view);
}
GUI_VIEW_INSTANCE("easy_demoMainView", false, easy_demoMainView_switch_in, easy_demoMainView_switch_out, true);

// Create mainface_view_1 (hg_view)
static void mainface_view_1_switch_out(gui_view_t *view)
{
    switch_out_mainface(view);
}

static void mainface_view_1_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 36);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));


    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)mainface_view_1_key_0_cb, GUI_EVENT_KB_SHORT_PRESSED, NULL);
    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)mainface_view_1_key_1_cb, GUI_EVENT_KB_LONG_PRESSED, NULL);
    gui_obj_focus_set((gui_obj_t *)view);

    switch_in_mainface_1(view);
}
GUI_VIEW_INSTANCE("mainface_view_1", false, mainface_view_1_switch_in, mainface_view_1_switch_out, true);

// Create mainface_view_2 (hg_view)
static void mainface_view_2_switch_out(gui_view_t *view)
{
    switch_out_mainface(view);
}

static void mainface_view_2_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 36);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));


    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)mainface_view_2_key_0_cb, GUI_EVENT_KB_SHORT_PRESSED, NULL);
    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)mainface_view_2_key_1_cb, GUI_EVENT_KB_LONG_PRESSED, NULL);
    gui_obj_focus_set((gui_obj_t *)view);

    switch_in_mainface_2(view);
}
GUI_VIEW_INSTANCE("mainface_view_2", false, mainface_view_2_switch_in, mainface_view_2_switch_out, true);

// Create top_view (hg_view)
static void top_view_switch_out(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void top_view_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 360);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));



    // Create bg_circle (hg_image)
    bg_circle = gui_img_create_from_fs((gui_obj_t *)view, "bg_circle", "/image/A8/circle_360_bg.bin", 0, 0, 360, 360);
    gui_img_set_mode((gui_img_t *)bg_circle, IMG_SRC_OVER_MODE);
    gui_img_set_opacity((gui_img_t *)bg_circle, 100);
    // Bind timer: 动画 1
    gui_obj_create_timer((gui_obj_t *)bg_circle, 20, true, bg_circle_timer_0_cb);

    // Create icon_del (hg_image)
    icon_del = gui_img_create_from_fs((gui_obj_t *)view, "icon_del", "/image/A8/delete_icon.bin", 230, 75, 100, 100);
    gui_img_set_mode((gui_img_t *)icon_del, IMG_SRC_OVER_MODE);
    gui_obj_add_event_cb(icon_del, (gui_event_cb_t)icon_del_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    // Create icon_fl (hg_image)
    icon_fl = gui_img_create_from_fs((gui_obj_t *)view, "icon_fl", "/image/A8/flashlight_icon.bin", 30, 75, 100, 100);
    gui_img_set_mode((gui_img_t *)icon_fl, IMG_SRC_OVER_MODE);
    gui_obj_add_event_cb(icon_fl, (gui_event_cb_t)icon_fl_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    // Create icon_connect (hg_image)
    icon_connect = gui_img_create_from_fs((gui_obj_t *)view, "icon_connect", "/image/A8/connect_icon.bin", 230, 190, 100, 100);
    gui_img_set_mode((gui_img_t *)icon_connect, IMG_SRC_OVER_MODE);
    gui_obj_add_event_cb(icon_connect, (gui_event_cb_t)icon_connect_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    // Create icon_bat (hg_image)
    icon_bat = gui_img_create_from_fs((gui_obj_t *)view, "icon_bat", "/image/A8/bat_100_icon.bin", 151, 142, 58, 34);
    gui_img_set_mode((gui_img_t *)icon_bat, IMG_SRC_OVER_MODE);
    // Bind timer: 动画 1
    gui_obj_create_timer((gui_obj_t *)icon_bat, 1000, true, icon_bat_timer_0_cb);

    // Create icon_sl (hg_image)
    icon_sl = gui_img_create_from_fs((gui_obj_t *)view, "icon_sl", "/image/screen_light_6_icon.bin", 30, 190, 100, 100);
    gui_img_set_mode((gui_img_t *)icon_sl, IMG_SRC_OVER_MODE);
    gui_obj_add_event_cb(icon_sl, (gui_event_cb_t)icon_sl_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    // Create icon_as (hg_image)
    icon_as = gui_img_create_from_fs((gui_obj_t *)view, "icon_as", "/image/auto_sleep_off_icon.bin", 130, 10, 100, 100);
    gui_img_set_mode((gui_img_t *)icon_as, IMG_SRC_OVER_MODE);
    gui_obj_add_event_cb(icon_as, (gui_event_cb_t)icon_as_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    // Create icon_cam (hg_image)
    icon_cam = gui_img_create_from_fs((gui_obj_t *)view, "icon_cam", "/image/A8/camera_icon.bin", 130, 250, 100, 100);
    gui_obj_add_event_cb(icon_cam, (gui_event_cb_t)icon_cam_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    // Create lbl_1 (hg_label)
    lbl_1 = gui_text_create((gui_obj_t *)view, "lbl_1", 0, 180, 360, 24);
    gui_text_set((gui_text_t *)lbl_1, "Auto sleep mode", GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255), 15, 16);
    gui_text_type_set((gui_text_t *)lbl_1, "/font/Inter_24pt_SemiBold_size16_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set((gui_text_t *)lbl_1, MID_CENTER);
    // Bind timer: 动画 1
    gui_obj_create_timer((gui_obj_t *)lbl_1, 10, false, lbl_1_timer_0_cb);

    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)top_view_key_0_cb, GUI_EVENT_KB_SHORT_PRESSED, NULL);
    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)top_view_key_1_cb, GUI_EVENT_KB_LONG_PRESSED, NULL);
    gui_obj_focus_set((gui_obj_t *)view);

    switch_in_top_view(view);
}
GUI_VIEW_INSTANCE("top_view", false, top_view_switch_in, top_view_switch_out, false);

// Create view_cam_ctl (hg_view)
static void view_cam_ctl_switch_out(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void view_cam_ctl_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 360);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));



    // Create streaming_1 (hg_streaming)
    streaming_1 = gui_stream_create((gui_obj_t *)view, "streaming_1", GUI_STREAM_CODEC_H264, gui_stream_transport_get(), 0, 0, 368, 368);
    gui_stream_set_update_interval((gui_stream_t *)streaming_1, 10);
    gui_stream_set_state((gui_stream_t *)streaming_1, GUI_VIDEO_STATE_PLAYING);

    // Create img_shutter (hg_image)
    img_shutter = gui_img_create_from_fs((gui_obj_t *)view, "img_shutter", "/image/stream/shutter.bin", 130, 248, 100, 100);
    gui_obj_add_event_cb(img_shutter, (gui_event_cb_t)img_shutter_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    // Create img_19 (hg_image)
    img_19 = gui_img_create_from_fs((gui_obj_t *)view, "img_19", "/image/stream/bg.bin", 237, 197, 110, 125);

    // Create img_1x (hg_image)
    img_1x = gui_img_create_from_fs((gui_obj_t *)view, "img_1x", "/image/stream/1x_hl.bin", 250, 259, 50, 50);
    gui_obj_add_event_cb(img_1x, (gui_event_cb_t)img_1x_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    // Create img_2x (hg_image)
    img_2x = gui_img_create_from_fs((gui_obj_t *)view, "img_2x", "/image/stream/2x_df.bin", 284, 210, 50, 50);
    gui_obj_add_event_cb(img_2x, (gui_event_cb_t)img_2x_clicked_cb, GUI_EVENT_TOUCH_CLICKED, NULL);

    switch_in_view_cam_ctl(view);
}
GUI_VIEW_INSTANCE("view_cam_ctl", false, view_cam_ctl_switch_in, view_cam_ctl_switch_out, false);

// Create view_mainface_list (hg_view)
static void view_mainface_list_switch_out(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void view_mainface_list_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 360);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));



    // Create lst_mainface (hg_list)
    lst_mainface = gui_list_create((gui_obj_t *)view, "lst_mainface", -80, 0, 360, 360, 160, 20, HORIZONTAL, lst_mainface_note_design, NULL, false);
    gui_list_set_style(lst_mainface, LIST_CIRCLE);
    gui_list_set_note_num(lst_mainface, 3);
    gui_list_set_auto_align(lst_mainface, true);
    gui_list_set_inertia(lst_mainface, false);
    gui_list_enable_loop(lst_mainface, true);
    gui_list_set_circle_radius(lst_mainface, 360);

    switch_in_mainface_list(view);
}
GUI_VIEW_INSTANCE("view_mainface_list", false, view_mainface_list_switch_in, view_mainface_list_switch_out, false);

// Create view_fl (hg_view)
static void view_fl_switch_out(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void view_fl_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 360);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(255, 255, 255));

    // Bind timer: 动画 1
    gui_obj_create_timer((gui_obj_t *)view, 10, true, view_fl_timer_0_cb);

    GUI_UNUSED(view);

    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)view_fl_key_0_cb, GUI_EVENT_KB_SHORT_PRESSED, NULL);
    gui_obj_focus_set((gui_obj_t *)view);
}
GUI_VIEW_INSTANCE("view_fl", false, view_fl_switch_in, view_fl_switch_out, false);
