#include "easy_demoMain_user.h"
#include "tp_algo.h"
#include "gui_message.h"

/* ----------------------------------------------------*/
#ifdef _HONEYGUI_SIMULATOR_

#else

#define USER_RESOURCE_ADDR     (0x0240f400u + 0x400000u)
#define USER_RESOURCE_ADDR_END (USER_RESOURCE_ADDR + 0x300000u)

#endif


uint8_t mainface_idx = 0;
uint8_t mainface_num = 6;
mainface_src_t mainface_list[MAINFACE_NUM_MAX] =
{
    {"/image/565/wallpaper_6.bin", SRC_IMG},
    {"/wallpaper_1.avi", SRC_VIDEO},
    {"/image/565/wallpaper_4.bin", SRC_IMG},

    {"/wallpaper_2.avi", SRC_VIDEO},
    {"/wallpaper_3.avi", SRC_VIDEO},
    {"/image/565/wallpaper_5.bin", SRC_IMG},
};
bool is_auto_sleep_mode = false;
bool is_bt_connect = false;
bool is_displaying_mainface = false;
bool enable_switch_mainface = true;
MODE_TYPE dev_mode = MODE_DEFAULT;

uint8_t soc_val = 100;
uint8_t screen_light_idx = 5; // 0~5

const char *view_rec = "easy_demoMainView";

static bool has_released = true;

static void *prog_arc_array[20] = 
{
    "/image/prog_arc_0.bin",
    "/image/prog_arc_1.bin",
    "/image/prog_arc_2.bin",
    "/image/prog_arc_3.bin",
    "/image/prog_arc_4.bin",
    "/image/prog_arc_5.bin",
    "/image/prog_arc_6.bin",
    "/image/prog_arc_7.bin",
    "/image/prog_arc_8.bin",
    "/image/prog_arc_9.bin",
    "/image/prog_arc_10.bin",
    "/image/prog_arc_11.bin",
    "/image/prog_arc_12.bin",
    "/image/prog_arc_13.bin",
    "/image/prog_arc_14.bin",
    "/image/prog_arc_15.bin",
    "/image/prog_arc_16.bin",
    "/image/prog_arc_17.bin",
    "/image/prog_arc_18.bin",
    "/image/prog_arc_19.bin",
};

int8_t fl_color_idx = 0; //white, red, orange, yellow, green, blue, indigo, violet

/* ---------------FUNC---------------- */
static void msg_2_regenerate_view(void *msg)
{
    GUI_UNUSED(msg);
    void *view_rec = (void *)gui_view_get_current()->base.name;
    gui_obj_child_free(gui_obj_get_root());
    gui_view_create(gui_obj_get_root(), view_rec, 0, 0, 0, 0);
}

void set_flashlight_color(void)
{
    gui_color_t color = {0};
    switch (fl_color_idx)
    {
    case 0:
        color = gui_rgb(255, 255, 255);
        break;
    case 1:
        color = gui_rgb(255, 0, 0);
        break;
    case 2:
        color = gui_rgb(255, 165, 0);
        break;
    case 3:
        color = gui_rgb(255, 255, 0);
        break;
    case 4:
        color = gui_rgb(0, 255, 0);
        break;
    case 5:
        color = gui_rgb(0, 127, 255);
        break;
    case 6:
        color = gui_rgb(0, 0, 255);
        break;
    case 7:
        color = gui_rgb(139, 0, 255);
        break;

    default:
        break;
    }
    gui_set_bg_color(color);
    gui_fb_change();

    // TO DO: adjust screen light
    
}

void create_win_del(gui_obj_t *parent)
{
    gui_win_t *win_del = gui_win_create((gui_obj_t *)parent, "win_del", 0, 0, 360, 360);

    gui_img_t *img = gui_img_create_from_fs(win_del, 0, "/image/A8/circle_360_bg.bin", 0, 180, 360, 360);
    gui_img_set_mode(img, IMG_SRC_OVER_MODE);
    gui_img_set_opacity(img, 122);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/delete_icon.bin", 63, 231, 100, 100);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_delete_icon_detail, GUI_EVENT_TOUCH_CLICKED, NULL);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/back_icon.bin", 198, 231, 100, 100);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_back_icon, GUI_EVENT_TOUCH_CLICKED, NULL);
}

void lcok_icon_timer_0_cb(void *obj)
{
    touch_info_t *tp = tp_get_info();
    
    gui_obj_t *lock = obj;
    gui_obj_t *prag_arc = gui_list_entry(lock->brother_list.next, gui_obj_t, brother_list);
    gui_obj_hidden(lock, false);
    gui_obj_hidden(prag_arc, false);
    
    static uint8_t cnt = 0;
    if (enable_switch_mainface)
    {
        gui_img_set_src((gui_img_t *)lock, "/image/lock_icon.bin", IMG_SRC_FILESYS);
        gui_img_set_src((gui_img_t *)prag_arc, prog_arc_array[cnt], IMG_SRC_FILESYS);
    }
    else
    {
        gui_img_set_src((gui_img_t *)lock, "/image/unlock_icon.bin", IMG_SRC_FILESYS);
        gui_img_set_src((gui_img_t *)prag_arc, prog_arc_array[19 - cnt], IMG_SRC_FILESYS);
    }

    cnt++;
    if (cnt < 19)
    {
        if (!tp->pressing)
        {
            gui_obj_hidden(lock, true);
            gui_obj_hidden(prag_arc, true);
            cnt = 0;
            gui_obj_delete_timer(lock);
        }
    }
    else if (cnt >= 19)
    {
        enable_switch_mainface = !enable_switch_mainface;
        gui_msg_t msg = 
        {
            .event = GUI_EVENT_USER_DEFINE,
            .cb = msg_2_regenerate_view,
        };
        gui_send_msg_to_server(&msg);
        cnt = 0;
    }
    // gui_log("cnt: %d\n", cnt);
}

void win_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);
    is_displaying_mainface = true;

    if (mainface_num == 0) return;
    touch_info_t *tp = tp_get_info();
    gui_obj_t *o = obj;
    gui_obj_t *child = gui_list_entry(o->child_list.next, gui_obj_t, brother_list);
    gui_obj_t *lock = gui_list_entry(child->brother_list.next, gui_obj_t, brother_list);

    if (child->type == IMAGE_FROM_MEM && child->w > SCREEN_SIZE)
    {
        int16_t img_x = child->x;
        int16_t img_w = child->w;
        img_x -= 5;
        if (img_x + img_w <= 0)
        {
            child->x = SCREEN_SIZE;
        }
        else
        {
            child->x = img_x;
        }
    }

    if (dev_mode != MODE_DELETE && tp->pressing)
    {
        if (tp->type == TOUCH_LONG && has_released)
        {
            has_released = false;
            gui_obj_create_timer(lock, 50, true, lcok_icon_timer_0_cb);
            gui_obj_start_timer(lock);
            gui_log("TOUCH_LONG\n");
        }
        if (lock->timer == NULL)
        {
            if (!enable_switch_mainface)
            {
                gui_img_set_src((gui_img_t *)lock, "/image/lock_icon.bin", IMG_SRC_FILESYS);
                gui_obj_hidden(lock, false);
            }
            else
            {
                gui_obj_hidden(lock, true);
            }
        }
    }
    if (!tp->pressing)
    {
        has_released = true;
        gui_obj_hidden(lock, true);
        // gui_log("touch released\n");
    }
}

void switch_mainface(gui_obj_t *parent, uint8_t idx)
{
    gui_win_t *win = gui_win_create(parent, 0, 0, 0, 360, 360);
    mainface_idx = idx;
    void (*timer_cb)(void *) = NULL;
    switch (idx)
    {
    case 0:
        timer_cb = easy_demoMainView_update_idx_cb;
        break;
    case 1:
        timer_cb = mainface_view_1_update_idx_cb;
        break;
    case 2:
        timer_cb = mainface_view_2_update_idx_cb;
        break;
    case 3:
        timer_cb = mainface_view_3_update_idx_cb;
        break;
    case 4:
        timer_cb = mainface_view_4_update_idx_cb;
        break;
    case 5:
        timer_cb = mainface_view_5_update_idx_cb;
        break;
    case 6:
        timer_cb = mainface_view_6_update_idx_cb;
        break;
    case 7:
        timer_cb = mainface_view_7_update_idx_cb;
        break;

       default:
        break;
    }
    gui_obj_create_timer((void *)win, 20, true, timer_cb);

    if (mainface_num == 0)
    {
        gui_text_t *text = gui_text_create((gui_obj_t *)win, 0, 0, 0, 360, 360);
        gui_text_set((gui_text_t *)text, "Please add a mainface", GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255), 21, 20);
        gui_text_type_set((gui_text_t *)text, "/font/Inter_24pt_SemiBold_size20_bits4_bitmap.bin", FONT_SRC_FILESYS);
        gui_text_mode_set((gui_text_t *)text, MID_CENTER);

        gui_view_switch_on_event((void *)parent, "top_view", SWITCH_INIT_STATE, SWITCH_IN_FROM_TOP_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_DOWN);
        dev_mode = MODE_DEFAULT;
        return;
    }

    if (mainface_list[idx].type == SRC_VIDEO)
    {
        gui_lite_video_t *vid = NULL;
#ifdef _HONEYGUI_SIMULATOR_
        vid = gui_lite_video_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
#else
        if (((uint32_t)mainface_list[idx].data) > USER_RESOURCE_ADDR && ((uint32_t)mainface_list[idx].data) < (USER_RESOURCE_ADDR_END))
        {
            vid = gui_lite_video_create_from_mem((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
        }
        else
        {
            vid = gui_lite_video_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
        }
#endif
        gui_lite_video_set_frame_rate((gui_lite_video_t *)vid, 30.f);
        gui_lite_video_set_repeat_count((gui_lite_video_t *)vid, GUI_VIDEO_REPEAT_INFINITE);
        gui_lite_video_set_state((gui_lite_video_t *)vid, GUI_VIDEO_STATE_PLAYING);
    }
    else
    {
#ifdef _HONEYGUI_SIMULATOR_
        gui_img_t *img_0 = gui_img_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
        gui_img_set_mode(img_0, IMG_BYPASS_MODE);
        if (img_0->base.w > SCREEN_SIZE)
        {
            int16_t img_y = (SCREEN_SIZE - img_0->base.h) / 2;
            gui_obj_move((void *)img_0, SCREEN_SIZE, img_y);
            gui_list_remove(&GUI_BASE(win)->brother_list);
            uint16_t color = (uint16_t)(((uint16_t *)img_0->src_data)[4]);
            void *img_bg_data = NULL;
            switch (color)
            {
            case 0xFFFF:
                img_bg_data = "/image/565/rect_360_white.bin";
                break;

            case 0x001F:
                img_bg_data = "/image/565/rect_360_blue.bin";
                break;
            
            case 0x03E0:
                img_bg_data = "/image/565/rect_360_green.bin";
                break;
                
            case 0xF800:
                img_bg_data = "/image/565/rect_360_red.bin";
                break;
            default:
                break;
            }

            if (img_bg_data != NULL)
            {
                gui_img_t *img_bg = gui_img_create_from_fs(parent, 0, img_bg_data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
                gui_img_set_mode(img_bg, IMG_BYPASS_MODE);
                gui_list_insert(&GUI_BASE(img_bg)->brother_list, &GUI_BASE(win)->brother_list);
            }
        }
#else
        gui_img_t *img_0 = NULL;
        gui_log("%s %d 0x%x\n", __FUNCTION__, __LINE__, mainface_list[idx].data);
        if (((uint32_t)mainface_list[idx].data) > USER_RESOURCE_ADDR && ((uint32_t)mainface_list[idx].data) < (USER_RESOURCE_ADDR_END))
        {
            img_0 = gui_img_create_from_mem((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
        }
        else
        {
            img_0 = gui_img_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
        }
        gui_img_set_mode(img_0, IMG_BYPASS_MODE);
        if (img_0->base.w > SCREEN_SIZE)
        {
            int16_t img_y = (SCREEN_SIZE - img_0->base.h) / 2;
            gui_obj_move((void *)img_0, SCREEN_SIZE, img_y);
            gui_list_remove(&GUI_BASE(win)->brother_list);
            uint16_t color = (uint16_t)(((uint16_t *)img_0->src_data)[4]);
            void *img_bg_data = NULL;
            switch (color)
            {
            case 0xFFFF:
                img_bg_data = "/image/565/rect_360_white.bin";
                break;

            case 0x001F:
                img_bg_data = "/image/565/rect_360_blue.bin";
                break;
            
            case 0x03E0:
                img_bg_data = "/image/565/rect_360_green.bin";
                break;
                
            case 0xF800:
                img_bg_data = "/image/565/rect_360_red.bin";
                break;
            default:
                break;
            }

            if (img_bg_data != NULL)
            {
                gui_img_t *img_bg = gui_img_create_from_fs(parent, 0, img_bg_data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
                gui_img_set_mode(img_bg, IMG_BYPASS_MODE);
                gui_list_insert(&GUI_BASE(img_bg)->brother_list, &GUI_BASE(win)->brother_list);
            }
        }
#endif
    }
    gui_img_t *img = gui_img_create_from_fs(win, 0, "/image/lock_icon.bin", 90, 90, 0, 0);
    gui_obj_hidden((gui_obj_t *)img, true);
    img = gui_img_create_from_fs(win, 0, prog_arc_array[0], 90, 90, 0, 0);
    gui_obj_hidden((gui_obj_t *)img, true);

    if (dev_mode != MODE_DELETE && !enable_switch_mainface) return;

    if (dev_mode == MODE_DELETE)
    {
        create_win_del(parent);
    }
    else
    {
        gui_view_switch_on_event((void *)parent, "top_view", SWITCH_INIT_STATE, SWITCH_IN_FROM_TOP_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_DOWN);
    }

    void *view_first = "easy_demoMainView";
    void *view_last = NULL;
    switch (mainface_num)
    {
    case 1:
        return;
        break;
    case 2:
        view_last = "mainface_view_1";
        break;
    case 3:
        view_last = "mainface_view_2";
        break;
    case 4:
        view_last = "mainface_view_3";
        break;
    case 5:
        view_last = "mainface_view_4";
        break;
    case 6:
        view_last = "mainface_view_5";
        break;
    case 7:
        view_last = "mainface_view_6";
        break;
    case 8:
        view_last = "mainface_view_7";
        break;

    default:
        break;
    }

    void *view_left = view_first;
    void *view_right = view_first;
    switch (idx)
    {
    case 0:
        view_left = view_last;
        view_right = "mainface_view_1";
        break;
    case 1:
        view_left = view_first;
        view_right = "mainface_view_2";
        break;
    case 2:
        view_left = "mainface_view_1";
        view_right = "mainface_view_3";
        break;
    case 3:
        view_left = "mainface_view_2";
        view_right = "mainface_view_4";
        break;
    case 4:
        view_left = "mainface_view_3";
        view_right = "mainface_view_5";
        break;
    case 5:
        view_left = "mainface_view_4";
        view_right = "mainface_view_6";
        break;
    case 6:
        view_left = "mainface_view_5";
        view_right = "mainface_view_7";
        break;
    case 7:
        view_left = "mainface_view_6";
        view_right = view_first;
        break;
        
    default:
        break;
    }
    if (idx == mainface_num - 1)
    {
        view_right = view_first;
    }
    gui_view_switch_on_event((void *)parent, view_right, SWITCH_OUT_TO_LEFT_USE_TRANSLATION, SWITCH_IN_FROM_RIGHT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_LEFT);
    gui_view_switch_on_event((void *)parent, view_left, SWITCH_OUT_TO_RIGHT_USE_TRANSLATION, SWITCH_IN_FROM_LEFT_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_RIGHT);
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
    if (mainface_num == 0) return;

    dev_mode = MODE_DELETE;
    void *view_mainface = "easy_demoMainView";
    switch (mainface_idx)
    {
    case 1:
        view_mainface = "mainface_view_1";
        break;
    case 2:
        view_mainface = "mainface_view_2";
        break;
    case 3:
        view_mainface = "mainface_view_3";
        break;
    case 4:
        view_mainface = "mainface_view_4";
        break;
    case 5:
        view_mainface = "mainface_view_5";
        break;
    case 6:
        view_mainface = "mainface_view_6";
        break;
    case 7:
        view_mainface = "mainface_view_7";
        break;

    default:
        break;
    }
    gui_view_switch_direct(gui_view_get_current(), view_mainface, SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
    
}

void mainface_list_delete(void)
{
    if (mainface_num == 0) return;

    void *addr_del = mainface_list[mainface_idx].data;
    for (uint8_t i = mainface_idx; i < mainface_num - 1; i++)
    {
        memcpy(&mainface_list[i], &mainface_list[i + 1], sizeof(mainface_src_t));
    }
    mainface_num--;
    void *view_left = NULL;
    if (mainface_num != 0) 
    {
        if (mainface_idx == mainface_num)
        {
            switch (mainface_num)
            {
            case 1:
                view_left = "easy_demoMainView";
                break;
            case 2:
                view_left = "mainface_view_1";
                break;  
            case 3:
                view_left = "mainface_view_2";
                break;
            case 4:
                view_left = "mainface_view_3";
                break;
            case 5:
                view_left = "mainface_view_4";
                break;
            case 6:
                view_left = "mainface_view_5";
                break;
            case 7:
                view_left = "mainface_view_6";
                break;
            case 8:
                view_left = "mainface_view_7";
                break;

            default:
                break;
            }
            if (mainface_idx != 0) mainface_idx--;
        }
    }
    else
    {
        mainface_idx = 0;
    }

    // to do, erase flash data
    gui_log("Do file erase! 0x%x\n", addr_del);
#ifdef _HONEYGUI_SIMULATOR_
    
#else
    extern int fdb_delete_by_addr(uintptr_t addr);
    fdb_delete_by_addr((uintptr_t)addr_del);
#endif

    void *view_target = view_left? view_left : (void *)gui_view_get_current()->base.name;
    // gui_log("view_target = %s, view_left = %s, idx = %d, num = %d, dev_mode = %d\n", view_target, view_left, mainface_idx, mainface_num, dev_mode);
    gui_obj_child_free(gui_obj_get_root());
    gui_view_create(gui_obj_get_root(), view_target, 0, 0, 0, 0);
}

static void mainface_list_add(void *data)
{
    if (mainface_num == MAINFACE_NUM_MAX) return;
    gui_log("Add resource 0x%x\n", data);

    mainface_list[mainface_num].data = data;
    mainface_list[mainface_num].type = (*(uint8_t *)(data) == 0x52)? SRC_VIDEO: SRC_IMG;
    mainface_num++;

    void *view = NULL;
    mainface_idx = mainface_num - 1;
    switch (mainface_idx)
    {
    case 0:
        view = "easy_demoMainView";
        break;
    case 1:
        view = "mainface_view_1";
        break;
    case 2:
        view = "mainface_view_2";
        break;  
    case 3:
        view = "mainface_view_3";
        break;
    case 4:
        view = "mainface_view_4";
        break;
    case 5:
        view = "mainface_view_5";
        break;
    case 6:
        view = "mainface_view_6";
        break;
    case 7:
        view = "mainface_view_7";
        break;

    default:
        break;
    }

    gui_obj_child_free(gui_obj_get_root());
    gui_view_create(gui_obj_get_root(), view, 0, 0, 0, 0);
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

void click_share_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    gui_view_switch_direct(gui_view_get_current(), "shareMainView", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
    
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
    dev_mode = MODE_DEFAULT;
    gui_obj_tree_free(GUI_BASE(obj)->parent);
    gui_view_switch_on_event(gui_view_get_current(), "top_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_FROM_TOP_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_DOWN);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif
}

void click_camera_ctl_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    if (is_bt_connect)
    {
        //TO DO: send bt cmd
    }
}

uint8_t mainface_list_init(void **data_list, uint32_t n)
{
    uint8_t idx = 0;
    if (data_list == NULL) return idx;
    
    while (data_list[idx] != NULL && ((idx < MAINFACE_NUM_MAX) && (idx < n)))
    {
        
        mainface_list[idx].data = data_list[idx];
        mainface_list[idx].type = SRC_IMG;
        if (*(uint8_t *)data_list[idx] == 0x52)
        {
            mainface_list[idx].type = SRC_VIDEO;
        }
        idx++;
    }
    mainface_num = idx;
    return idx;
}

void ui_process_msg(void *arg)
{
    static bool get_transmit_start = false;
    
    gui_msg_t *msg = arg;
    UI_SUBEVENT_TYPE subevent = (UI_SUBEVENT_TYPE)msg->sub_event;
    switch (subevent)
    {
    case ADD_MAINFACE:
    {
        mainface_list_add(msg->payload);
        get_transmit_start = false;
        break;
    } 
    case BT_ON:
        is_bt_connect = true;
        break;
    case BT_OFF:
        is_bt_connect = false;
        break;
    case TRANSMIT_START:
    {
        if (get_transmit_start) // state abnormal
        {
            gui_obj_child_free(gui_obj_get_root());
            gui_view_create(gui_obj_get_root(), view_rec, 0, 0, 0, 0);
            get_transmit_start = false;
            return;
        }
        
        get_transmit_start = true;
        
        gui_view_t *view_cur = gui_view_get_current();
        if (view_cur != NULL && strcmp(view_cur->base.name, "view_transmit") != 0)
        {
            view_rec = view_cur->base.name;
        }
        gui_obj_child_free(gui_obj_get_root());
        gui_view_create(gui_obj_get_root(), "view_transmit", 0, 0, 0, 0);
        break;
    }
    case TRANSMIT_ABORT:
    {
        get_transmit_start = false;
        gui_view_switch_direct(gui_view_get_current(), view_rec, SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        break;
    }
    case SWITCH_LEFT_MAINFACE:
    {
        if (is_displaying_mainface || mainface_num <= 1) return;
        mainface_idx = (mainface_idx - 1 + mainface_num) % mainface_num;
        msg_2_regenerate_view(NULL);
        break;
    }
    case SWITCH_RIGHT_MAINFACE:
    {
        if (is_displaying_mainface || mainface_num <= 1) return;
        mainface_idx = (mainface_idx + 1) % mainface_num;
        msg_2_regenerate_view(NULL);
        break;
    }
    case CAST_START:
    {
        if (get_transmit_start) return;
        gui_view_t *view_cur = gui_view_get_current();
        if (view_cur != NULL && strcmp(view_cur->base.name, "view_cast") != 0)
        {
            view_rec = view_cur->base.name;
        }
        gui_obj_child_free(gui_obj_get_root());
        gui_view_create(gui_obj_get_root(), "view_cast", 0, 0, 0, 0);
        break;
    }
    case CAST_STOP:
    {
        gui_obj_child_free(gui_obj_get_root());
        gui_view_create(gui_obj_get_root(), view_rec, 0, 0, 0, 0);
        break;
    }
        
    default:
        break;
    }
}
