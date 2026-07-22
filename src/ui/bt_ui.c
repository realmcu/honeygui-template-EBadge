/**
 * bt UI Implementation (Auto-generated, do not modify manually)
 */
#include "bt_ui.h"
#include "../callbacks/bt_callbacks.h"
#include "../user/bt_user.h"
#include <stddef.h>

// Component handle definitions
gui_img_t *circle_scale = NULL;
gui_img_t *bt_icon = NULL;
gui_text_t *lbl_4 = NULL;
gui_text_t *lbl_5 = NULL;
gui_qbcode_t *qbcode_1 = NULL;
gui_text_t *lbl_11 = NULL;


// Create bt_View (hg_view)
static void bt_View_switch_out(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void bt_View_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 360);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));

    gui_view_switch_on_event(view, "easy_demoMainView", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION, GUI_EVENT_TOUCH_CLICKED);


    // Create circle_scale (hg_image)
    circle_scale = gui_img_create_from_fs((gui_obj_t *)view, "circle_scale", "/image/A8/circle_360_bg_white.bin", 54, 38, 360, 360);
    gui_img_set_mode((gui_img_t *)circle_scale, IMG_BYPASS_MODE);
    gui_img_set_quality((gui_img_t *)circle_scale, true);
    gui_img_translate((gui_img_t *)circle_scale, 180.0f, 180.0f);
    gui_img_set_focus((gui_img_t *)circle_scale, 180.0f, 180.0f);
    gui_img_scale((gui_img_t *)circle_scale, 0.740000f, 0.740000f);
    // Bind timer: 动画 1
    gui_obj_create_timer((gui_obj_t *)circle_scale, 10, true, circle_scale_timer_0_cb);

    // Create bt_icon (hg_image)
    bt_icon = gui_img_create_from_fs((gui_obj_t *)view, "bt_icon", "/image/A8/bt_icon_discon.bin", 213, 130, 42, 69);
    // Bind timer: 动画 1
    gui_obj_create_timer((gui_obj_t *)bt_icon, 20, true, bt_icon_timer_0_cb);

    // Create lbl_5 (hg_label)
    lbl_5 = gui_text_create((gui_obj_t *)view, "lbl_5", 1, 358, 466, 40);
    gui_text_set((gui_text_t *)lbl_5, "Click any button to exit", GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255), 24, 28);
    gui_text_type_set((gui_text_t *)lbl_5, "/font/Inter_24pt_SemiBold_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set((gui_text_t *)lbl_5, CENTER);

    // Create lbl_4 (hg_label)
    lbl_4 = gui_text_create((gui_obj_t *)view, "lbl_4", 159, 235, 151, 35);
    gui_text_set((gui_text_t *)lbl_4, "Pair mode", GUI_FONT_SRC_BMP, gui_rgb(0, 0, 0), 9, 28);
    gui_text_type_set((gui_text_t *)lbl_4, "/font/Inter_24pt_SemiBold_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set((gui_text_t *)lbl_4, CENTER);

    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)bt_View_key_0_cb, GUI_EVENT_KB_SHORT_PRESSED, NULL);
    gui_obj_focus_set((gui_obj_t *)view);
}
GUI_VIEW_INSTANCE("bt_View", false, bt_View_switch_in, bt_View_switch_out, false);

// Create qrcode_view (hg_view)
static void qrcode_view_switch_out(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void qrcode_view_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 360);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));

    gui_view_switch_on_event(view, "easy_demoMainView", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION, GUI_EVENT_TOUCH_CLICKED);


    // Create qbcode_1 (hg_qbcode)
    qbcode_1 = gui_qbcode_create((gui_obj_t *)view, "qbcode_1", 93, 93, 280, 280, QRCODE_DISPLAY_IMAGE, QRCODE_ENCODE_TEXT);
    gui_qbcode_config(qbcode_1, (uint8_t *)"Hello, World!", strlen("Hello, World!"), 5);

    gui_obj_add_event_cb((gui_obj_t *)view, (gui_event_cb_t)qrcode_view_key_0_cb, GUI_EVENT_KB_SHORT_PRESSED, NULL);
    gui_obj_focus_set((gui_obj_t *)view);
}
GUI_VIEW_INSTANCE("qrcode_view", false, qrcode_view_switch_in, qrcode_view_switch_out, false);

// Create view_transmit (hg_view)
static void view_transmit_switch_out(gui_view_t *view)
{
    GUI_UNUSED(view);
}

static void view_transmit_switch_in(gui_view_t *view)
{
    // Set animation step
    gui_view_set_animate_step(view, 360);

    // Set opacity
    gui_view_set_opacity(view, 255);

    // Set background color
    gui_view_set_bg_color(view, gui_rgb(0, 0, 0));

    // Bind timer: 动画 1
    gui_obj_create_timer((gui_obj_t *)view, 20, true, view_transmit_timer_0_cb);
    gui_obj_start_timer((gui_obj_t *)view);

    GUI_UNUSED(view);


    // Create lbl_11 (hg_label)
    lbl_11 = gui_text_create((gui_obj_t *)view, "lbl_11", 0, 83, 466, 300);
    gui_text_set((gui_text_t *)lbl_11, "Transmitting...", GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255), 15, 48);
    gui_text_type_set((gui_text_t *)lbl_11, "/font/Inter_24pt_SemiBold_size48_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set((gui_text_t *)lbl_11, MID_CENTER);
}
GUI_VIEW_INSTANCE("view_transmit", false, view_transmit_switch_in, view_transmit_switch_out, false);
