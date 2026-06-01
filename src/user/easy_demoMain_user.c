#include "easy_demoMain_user.h"

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
uint8_t mainface_idx = 0;
uint8_t mainface_num = 5;
MAINFACE_SRC_TYPE mainface_src_t[MAINFACE_NUM] =
{
    SRC_VIDEO,
    SRC_VIDEO,
    SRC_VIDEO,
    SRC_IMG,
    SRC_IMG,
};
void *mainface_list[MAINFACE_NUM] = 
{
    "/wallpaper_1.avi", 
    "/wallpaper_2.avi",
    "/wallpaper_3.avi",
    "/image/wallpaper_4.bin",
    "/image/wallpaper_5.bin",
};
bool is_auto_sleep_mode = false;
bool is_bt_connect = false;
MODE_TYPE mode = MODE_DEFAULT;

uint8_t soc_val = 100;
uint8_t screen_light_idx = 5; // 0~5

void switch_mainface(void)
{
    gui_obj_child_free((void *)win_mainface);
    if (mainface_num == 0) return;
    if (mainface_src_t[mainface_idx] == SRC_VIDEO)
    {
        vid_1 = gui_msv1_create_from_fs(win_mainface, "vid_1", mainface_list[mainface_idx], 0, 0, SCREEN_H, SCREEN_H);
        gui_msv1_set_frame_rate((gui_msv1_t *)vid_1, 30.f);
        gui_msv1_set_repeat_count((gui_msv1_t *)vid_1, GUI_VIDEO_REPEAT_INFINITE);
        gui_msv1_set_state((gui_msv1_t *)vid_1, GUI_VIDEO_STATE_PLAYING);
    }
    else
    {
        gui_img_t *img = gui_img_create_from_fs(win_mainface, 0, mainface_list[mainface_idx], 0, 0, SCREEN_H, SCREEN_H);
        gui_img_set_mode(img, IMG_SRC_OVER_MODE);
    }
}

void click_auto_sleep_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    is_auto_sleep_mode = !is_auto_sleep_mode;
    if (is_auto_sleep_mode)
    {
        gui_img_set_src(icon_as, "/image/auto_sleep_on_icon.bin", IMG_SRC_FILESYS);
        gui_obj_hidden(GUI_BASE(lbl_1), false);
    }
    else
    {
        gui_img_set_src(icon_as, "/image/auto_sleep_off_icon.bin", IMG_SRC_FILESYS);
        gui_obj_hidden(GUI_BASE(lbl_1), true);
    }
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
    
}

void click_screen_light_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    screen_light_idx++;
    screen_light_idx %= 6;
    void *img_src = NULL;
    switch (screen_light_idx)
    {
    case 0:
        img_src = "/image/screen_light_1_icon.bin";
        break;
    case 1:
        img_src = "/image/screen_light_2_icon.bin";
        break;
    case 2:
        img_src = "/image/screen_light_3_icon.bin";
        break;
    case 3:
        img_src = "/image/screen_light_4_icon.bin";
        break;
    case 4:
        img_src = "/image/screen_light_5_icon.bin";
        break;
    case 5:
        img_src = "/image/screen_light_6_icon.bin";
        break;
    default:
        break;
    }
    gui_img_set_src(icon_sl, img_src, IMG_SRC_FILESYS);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
}

void click_delete_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    mode = MODE_DELETE;
    y_rec = -SCREEN_H;
    win_menu->base.y = y_rec;

    gui_obj_t *parent = (void *)gui_view_get_current();
    gui_win_t *win_del = gui_win_create((gui_obj_t *)parent, "win_del", 0, 0, 360, 360);

    gui_img_t *img = gui_img_create_from_fs(win_del, 0, "/image/A8/circle_360_bg.bin", 0, 180, 360, 360);
    gui_img_set_mode(img, IMG_2D_SW_FIX_A8_FG);
    gui_img_a8_recolor(img, 0xFF000000);
    gui_img_set_opacity(img, 122);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/delete_iocn.bin", 63, 231, 100, 100);
    gui_img_set_mode(img, IMG_2D_SW_FIX_A8_FG);
    gui_img_a8_recolor(img, 0xFFFFFFFF);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_delete_icon_detail, GUI_EVENT_TOUCH_CLICKED, NULL);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/back_icon.bin", 198, 231, 100, 100);
    gui_img_set_mode(img, IMG_2D_SW_FIX_A8_FG);
    gui_img_a8_recolor(img, 0xFFFFFFFF);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_back_icon, GUI_EVENT_TOUCH_CLICKED, NULL);
    
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
    
}

void mainface_list_delete(void)
{
    if (mainface_num == 0) return;
    for (uint8_t i = mainface_idx; i < mainface_num - 1; i++)
    {
        mainface_list[i] = mainface_list[i + 1];
        mainface_src_t[i] = mainface_src_t[i + 1];
    }
    mainface_num--;

    if (mainface_num != 0) mainface_idx %= mainface_num;
    switch_mainface();
}

void click_delete_icon_detail(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    mainface_list_delete();
    
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
}

void click_back_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    mode = MODE_DEFAULT;
    gui_obj_tree_free(GUI_BASE(obj)->parent);
    
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
}

