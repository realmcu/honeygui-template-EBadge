#include "easy_demoMain_callbacks.h"
#include "../ui/easy_demoMain_ui.h"
#include "../user/easy_demoMain_user.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Timer animation counters
uint16_t easy_demoMainView_timer_cnt = 0;
uint16_t icon_bat_timer_cnt = 0;
uint16_t lbl_1_timer_cnt = 0;

// Event callback function implementations

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

/* @protected start custom_functions */
// Custom functions
#include "tp_algo.h"

static int16_t y_rec = -SCREEN_H;

void switch_mainface(void *obj)
{
    GUI_UNUSED(obj);
    gui_obj_tree_free(vid_1);

    vid_1 = gui_msv1_create_from_fs(win_1, "vid_1", mainface_list[mainface_idx], 0, 0, SCREEN_H, SCREEN_H);
    gui_msv1_set_frame_rate((gui_msv1_t *)vid_1, 30.f);
    gui_msv1_set_repeat_count((gui_msv1_t *)vid_1, GUI_VIDEO_REPEAT_INFINITE);
    gui_msv1_set_state((gui_msv1_t *)vid_1, GUI_VIDEO_STATE_PLAYING);
}

void easy_demoMainView_timer_init_cb(void *obj)
{
    GUI_UNUSED(obj);
    y_rec = -SCREEN_H;
    win_menu->base.y = -SCREEN_H;
    switch_mainface(win_1);
    gui_obj_create_timer(obj, 10, true, easy_demoMainView_timer_0_cb);
    gui_obj_start_timer(obj);
}

void easy_demoMainView_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);
    touch_info_t *tp = tp_get_info();
    
    if (tp->pressing && tp->type == TOUCH_HOLD_Y)
    {
        int16_t y = y_rec + tp->deltaY;
        gui_obj_hidden(GUI_BASE(win_menu), false);
        if (y < -SCREEN_H)
        {
            y = -SCREEN_H;
            gui_obj_hidden(GUI_BASE(win_menu), true);
        }
        else if (y > 0)
        {
            y = 0;
        }
        gui_obj_move(GUI_BASE(win_menu), 0, y);
        // gui_log("win_menu.y = %d\n", y);
    }
    else if (tp->type == TOUCH_LEFT_SLIDE_QUICK || tp->type == TOUCH_LEFT_SLIDE)
    {
        if (win_menu->base.y != -SCREEN_H) return;
        mainface_idx++;
        mainface_idx %= mainface_num;
        switch_mainface(win_1);
    }
    else if (tp->type == TOUCH_RIGHT_SLIDE_QUICK || tp->type == TOUCH_RIGHT_SLIDE)
    {
        if (win_menu->base.y != -SCREEN_H) return;
        mainface_idx += mainface_num;
        mainface_idx--;
        mainface_idx %= mainface_num;
        switch_mainface(win_1);
    }
    else if ((int16_t)(win_menu->base.y) % SCREEN_H != 0)
    {
        int16_t y = win_menu->base.y;
        const int16_t step = 20;
        if (y >= -180)
        {
            y += step;
        }
        else
        {
            y -= step;
        }
        if (y <= -SCREEN_H)
        {
            y = -SCREEN_H;
            gui_obj_hidden(GUI_BASE(win_menu), true);
        }
        else if (y >= 0)
        {
            y = 0;
        }
        y_rec = y;
        gui_obj_move(GUI_BASE(win_menu), 0, y);
        // gui_log("win_menu.y = %d\n", y);
    }
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
