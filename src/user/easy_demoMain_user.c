#include "easy_demoMain_user.h"
#include "tp_algo.h"
#include "gui_message.h"
#include "gui_lite3d.h"
#include "gui_vfs.h"
#include "gui_lite_video.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "stream_transport.h"   /* stp_config_t / stp_instance_create / ... */

/* ----------------------------------------------------*/
#ifdef _HONEYGUI_SIMULATOR_

#else
#include "flashdb.h"
#define USER_RESOURCE_ADDR     (FDB_BF_DATA_PART_OFFSET)
#define USER_RESOURCE_ADDR_END (FDB_BF_DATA_PART_OFFSET + FDB_BF_DATA_SIZE) // 0x00998000  //9824K Bytes

#endif

gui_win_t *win_view = NULL;

uint8_t mainface_idx = 0;
uint8_t mainface_num = 5;
mainface_src_t mainface_list[MAINFACE_NUM_MAX] =
{
    {"/image/565/wallpaper_danmu.bin", SRC_DANMU},
    {"/user/gltf_desc_Fox.bin", SRC_3D},
    {"/wallpaper_video.avi", SRC_VIDEO},
    {"/image/565/wallpaper_static_img.bin", SRC_IMG},
    {"/foreground_360.bin", SRC_IMG_SPATIAL},

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
static bool has_created_win_del = false;

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

// 3D model
static float rot_angle = 45.0f;
static l3_model_base_t *fox_3d = NULL;
static void *temp_3d_data = NULL;

/* ---------------FUNC---------------- */
static void fox_rotate_animation(void *param)
{
    (void)param;
    if (gui_view_get_next()) return;

    touch_info_t *tp = tp_get_info();

    if (tp->pressed || tp->pressing)
    {
        rot_angle += tp->deltaX / 5.0f;
    }
}

static void fox_animation_update_cb(void *obj, gui_event_t *e)
{
    (void)obj;
    (void)e;

    static uint32_t cur_anim = 0;
    uint32_t anim_count = l3_gltf_get_animation_count(fox_3d);
    if (anim_count > 0)
    {
      cur_anim = (cur_anim + 1) % anim_count;
      l3_gltf_set_active_animation(fox_3d, cur_anim);
    }

}

static void fox_global_cb(l3_model_base_t *this)
{
    l3_camera_UVN_initialize(&this->camera, l3_4d_point(0, 0, 0), l3_4d_point(0, 0, 1), 1,
                             32767,
                             90, this->viewPortWidth, this->viewPortHeight);

    l3_world_initialize(&this->world, 0, 10, 25, 0, rot_angle, 0, 5);
}

uint32_t get_img_color(uint8_t *img_data)
{
    uint32_t color = 0;
    if (img_data[0] & 0x10) // Compressed (RLE)
    {
        /* Layout: gui_rgb_data_head_t(8B) + imdc_file_header_t(12B)
         *         + row offset table((height+1)*4B) + RLE data.
         * Row offsets are relative to the imdc header start (file offset 8).
         * Each RLE node = [run length (1B)] [pixel (bpp)].
         * Use byte reads: the first pixel sits at an odd (unaligned) offset. */
        const uint8_t *row_table = img_data + 8 + 12;      /* row offset table  */
        uint32_t row0_off = (uint32_t)row_table[0]
                          | ((uint32_t)row_table[1] << 8)
                          | ((uint32_t)row_table[2] << 16)
                          | ((uint32_t)row_table[3] << 24);
        const uint8_t *pixel = img_data + 8 + row0_off + 1; /* skip run length   */
        color = (uint16_t)(pixel[0] | (pixel[1] << 8));     /* RGB565, little-end */
    }
    else
    {
        color = ((uint16_t *)img_data)[4]; //RGB565
    }
    return color;
}

static void msg_2_regenerate_view(void *msg)
{
    GUI_UNUSED(msg);
    void *view_rec = (void *)gui_view_get_current()->base.name;
    gui_obj_child_free(GUI_BASE(win_view));
    gui_view_create(GUI_BASE(win_view), view_rec, 0, 0, 0, 0);
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

void create_win_del(void)
{
    if (has_created_win_del) return;
    has_created_win_del = true;
    gui_win_t *win_del = gui_win_create(gui_obj_get_root(), "win_del", 0, 0, 360, 360);

    gui_img_t *img = gui_img_create_from_fs(win_del, 0, "/image/A8/circle_360_bg.bin", 0, 180, 360, 360);
    gui_img_set_mode(img, IMG_SRC_OVER_MODE);
    gui_img_set_opacity(img, 122);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/delete_icon.bin", 63, 231, 100, 100);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_delete_icon_detail, GUI_EVENT_TOUCH_CLICKED, NULL);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/back_icon.bin", 198, 231, 100, 100);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_back_icon, GUI_EVENT_TOUCH_CLICKED, NULL);

    gui_obj_focus_set(GUI_BASE(win_del));
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
        gui_img_set_src((gui_img_t *)lock, (const uint8_t *)"/image/lock_icon.bin", IMG_SRC_FILESYS);
        gui_img_set_src((gui_img_t *)prag_arc, prog_arc_array[cnt], IMG_SRC_FILESYS);
    }
    else
    {
        gui_img_set_src((gui_img_t *)lock, (const uint8_t *)"/image/unlock_icon.bin", IMG_SRC_FILESYS);
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
    if (!has_created_win_del) // if win_del is not created, set focus to win_view
    {
        gui_obj_focus_set(GUI_BASE(gui_view_get_current()));
    }

    if (mainface_num == 0) return;
    touch_info_t *tp = tp_get_info();
    gui_obj_t *o = obj;
    gui_obj_t *child = gui_list_entry(o->child_list.next, gui_obj_t, brother_list);
    gui_obj_t *lock = gui_list_entry(child->brother_list.next, gui_obj_t, brother_list);
    mainface_src_t * mainface_src = (mainface_src_t *)o->user_data;
    if (mainface_src->type == SRC_DANMU)
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
                gui_img_set_src((gui_img_t *)lock, (const uint8_t *)"/image/lock_icon.bin", IMG_SRC_FILESYS);
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
    win->base.user_data = &mainface_list[idx];
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
    case 8:
        timer_cb = mainface_view_8_update_idx_cb;
        break;
    case 9:
        timer_cb = mainface_view_9_update_idx_cb;
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

    gui_event_code_t event_code_l = GUI_EVENT_TOUCH_MOVE_LEFT;
    gui_event_code_t event_code_r = GUI_EVENT_TOUCH_MOVE_RIGHT;
    switch (mainface_list[idx].type)
    {
    case SRC_VIDEO:
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
        // gui_lite_video_set_frame_rate((gui_lite_video_t *)vid, 30.f);
        gui_lite_video_set_repeat_count((gui_lite_video_t *)vid, GUI_VIDEO_REPEAT_INFINITE);
        gui_lite_video_set_state((gui_lite_video_t *)vid, GUI_VIDEO_STATE_PLAYING);
        break;
    }
    case SRC_IMG:
    {
        gui_img_t *img = NULL;
#ifdef _HONEYGUI_SIMULATOR_
        img = gui_img_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
#else
        gui_log("%s %d 0x%x\n", __FUNCTION__, __LINE__, mainface_list[idx].data);
        if (((uint32_t)mainface_list[idx].data) > USER_RESOURCE_ADDR && ((uint32_t)mainface_list[idx].data) < (USER_RESOURCE_ADDR_END))
        {
            gui_log("%s %d 0x%x\n", __FUNCTION__, __LINE__, mainface_list[idx].data);
            img = gui_img_create_from_mem((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
        }
        else
        {
            img = gui_img_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
        }
#endif        
        gui_img_set_mode(img, IMG_BYPASS_MODE);
        break;
    }
    case SRC_3D:
    {
        /* code */
        event_code_l = GUI_EVENT_TOUCH_LEFT_SLIDE_QUICK;
        event_code_r = GUI_EVENT_TOUCH_RIGHT_SLIDE_QUICK;
        gui_view_set_bg_color((gui_view_t *)parent, gui_rgb(0x41, 0xAD, 0x41));
        void *addr = (void *)gui_vfs_get_file_address(mainface_list[idx].data);
        if (!addr)
        {
            /* Fallback: read file into memory */
            gui_vfs_file_t *f = gui_vfs_open(mainface_list[idx].data, GUI_VFS_READ);
            GUI_ASSERT(f != NULL);
            gui_vfs_seek(f, 0, GUI_VFS_SEEK_END);
            int size = gui_vfs_tell(f);

            if (size <= 0)
            {
                gui_vfs_close(f);
                return;
            }
            gui_vfs_seek(f, 0, GUI_VFS_SEEK_SET);
            if (temp_3d_data == NULL)
            {
                temp_3d_data = gui_malloc(size);
            }
            GUI_ASSERT(temp_3d_data != NULL);
            gui_vfs_read(f, (void *)temp_3d_data, size);
            gui_vfs_close(f);
            addr = temp_3d_data;
        }
        fox_3d = l3_create_model(addr, L3_DRAW_FRONT_AND_SORT, 0,
                            0,
                            360,
                            360);
        l3_set_global_transform(fox_3d, (l3_global_transform_cb)fox_global_cb);
        gui_lite3d_t *lite3d_fox = gui_lite3d_create(parent, "lite3d-widget",
                                                    fox_3d, 0, 0, 0, 0);
        gui_obj_create_timer(GUI_BASE(lite3d_fox), 20, true, fox_rotate_animation);
        gui_lite3d_on_click(lite3d_fox, fox_animation_update_cb, NULL);
        break;
    }
    case SRC_IMG_SPATIAL:
    {
        /* code */
        extern int spatial_wallpaper(gui_obj_t *parent);
        gui_obj_t *sw_root = gui_obj_create(win, "sw_root", 0, 0, 0, 0);
        spatial_wallpaper(sw_root);
        break;
    }
    case SRC_DANMU:
    {
#ifdef _HONEYGUI_SIMULATOR_
        gui_img_t *img_0 = gui_img_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
#else
        gui_img_t *img_0 = NULL;
        gui_log("%s %d 0x%x\n", __FUNCTION__, __LINE__, mainface_list[idx].data);
        if (((uint32_t)mainface_list[idx].data) > USER_RESOURCE_ADDR && ((uint32_t)mainface_list[idx].data) < (USER_RESOURCE_ADDR_END))
        {
            gui_log("%s %d 0x%x\n", __FUNCTION__, __LINE__, mainface_list[idx].data);
            img_0 = gui_img_create_from_mem((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
        }
        else
        {
            img_0 = gui_img_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
        }
#endif
        gui_img_set_mode(img_0, IMG_BYPASS_MODE);
        int16_t img_y = (SCREEN_SIZE - img_0->base.h) / 2;
        gui_obj_move((void *)img_0, SCREEN_SIZE, img_y);
        {
            const void *src_data = gui_img_get_image_data(img_0);
            uint32_t src_color = get_img_color((uint8_t *)src_data);
            gui_log("bg color = 0x%x\n", src_color);
            gui_color_t bg_color;
            bg_color.color.rgba.a = UINT8_MAX;
            if (((gui_rgb_data_head_t *)src_data)->type == RGB565)
            {
                bg_color.color.rgba.r = src_color >> 8 & 0xFF;
                bg_color.color.rgba.g = src_color >> 3 & 0xFF;
                bg_color.color.rgba.b = src_color << 3 & 0xFF;
            }
            else
            {
                bg_color.color.rgba.r = src_color >> 16 & 0xFF;
                bg_color.color.rgba.g = src_color >> 8 & 0xFF;
                bg_color.color.rgba.b = src_color & 0xFF;
            }
            gui_view_set_bg_color((gui_view_t *)parent, bg_color);
        }
        break;
    }
        
    default:
        break;
    }
    
    gui_img_t *img = gui_img_create_from_fs(win, 0, "/image/lock_icon.bin", 90, 90, 0, 0);
    gui_obj_hidden((gui_obj_t *)img, true);
    img = gui_img_create_from_fs(win, 0, prog_arc_array[0], 90, 90, 0, 0);
    gui_obj_hidden((gui_obj_t *)img, true);

    if (dev_mode != MODE_DELETE && !enable_switch_mainface) return;

    if (dev_mode == MODE_DELETE)
    {
        create_win_del();
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
    case 9:
        view_last = "mainface_view_8";
        break;
    case 10:
        view_last = "mainface_view_9";
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
        view_right = "mainface_view_8";
        break;
    case 8:
        view_left = "mainface_view_7";
        view_right = "mainface_view_9";
        break;
    case 9:
        view_left = "mainface_view_8";
        view_right = view_first;
        break;
        
    default:
        break;
    }
    if (idx == mainface_num - 1)
    {
        view_right = view_first;
    }
    gui_view_switch_on_event((void *)parent, view_right, SWITCH_OUT_TO_LEFT_USE_TRANSLATION, SWITCH_IN_FROM_RIGHT_USE_TRANSLATION, event_code_l);
    gui_view_switch_on_event((void *)parent, view_left, SWITCH_OUT_TO_RIGHT_USE_TRANSLATION, SWITCH_IN_FROM_LEFT_USE_TRANSLATION, event_code_r);
}

void click_auto_sleep_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    is_auto_sleep_mode = !is_auto_sleep_mode;
    if (is_auto_sleep_mode)
    {
        gui_img_set_src(icon_as, (const uint8_t *)"/image/auto_sleep_on_icon.bin", IMG_SRC_FILESYS);
        gui_obj_hidden(GUI_BASE(lbl_1), false);
    }
    else
    {
        gui_img_set_src(icon_as, (const uint8_t *)"/image/auto_sleep_off_icon.bin", IMG_SRC_FILESYS);
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

void mainface_list_delete(void *obj)
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
        dev_mode = MODE_DEFAULT;
        has_created_win_del = false;
        gui_obj_tree_free(GUI_BASE(obj)->parent);
    }

    // to do, erase flash data
    gui_log("Do file erase! 0x%x\n", addr_del);
#ifdef _HONEYGUI_SIMULATOR_
    
#else
    // extern int fdb_delete_by_addr(uintptr_t addr);
    // fdb_delete_by_addr((uintptr_t)addr_del);
    extern fdb_bf_t   app_get_bf(void);
    extern fdb_err_t fdb_bf_delete_by_addr(fdb_bf_t db, uint32_t addr);
    fdb_bf_delete_by_addr(app_get_bf(), (uint32_t)addr_del);
#endif

    void *view_target = view_left? view_left : (void *)gui_view_get_current()->base.name;
    // gui_log("view_target = %s, view_left = %s, idx = %d, num = %d, dev_mode = %d\n", view_target, view_left, mainface_idx, mainface_num, dev_mode);
    gui_obj_child_free(GUI_BASE(win_view));
    gui_view_create(GUI_BASE(win_view), view_target, 0, 0, 0, 0);
}

static void mainface_list_add(void *data)
{
    if (mainface_num == MAINFACE_NUM_MAX) return;

    uint32_t info[2];
    memcpy(info, data, 8);
    gui_log("Add resource 0x%x sz %d\n", info[0], info[1]);
    if(!info[0] || !info[1])
    {
        gui_log("New passed resource  is NULL!!!!!!!!!!!!!!!!!\n");
        return ;
    }

    
    mainface_list[mainface_num].data = (void *)info[0];
    mainface_list[mainface_num].type = (*(uint8_t *)(info[0]) == 0x52)? SRC_VIDEO: SRC_IMG;
    if(mainface_list[mainface_num].type == SRC_IMG)
    {
        uint8_t *pdata = (uint8_t *)(info[0] + info[1] - 1);
        if(*pdata)      
        {
            mainface_list[mainface_num].type = SRC_DANMU;
            gui_log("Add danmu\n");
        }  
    }
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

    gui_obj_child_free(GUI_BASE(win_view));
    gui_view_create(GUI_BASE(win_view), view, 0, 0, 0, 0);
}

void click_delete_icon_detail(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    mainface_list_delete(obj);
    
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
    has_created_win_del = false;
    gui_obj_tree_free(GUI_BASE(obj)->parent);
    gui_obj_focus_set(GUI_BASE(gui_view_get_current()));
    gui_view_switch_on_event(gui_view_get_current(), "top_view", SWITCH_INIT_STATE, SWITCH_IN_FROM_TOP_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_DOWN);
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
    if (data_list == NULL || !n) return idx;
    
    while (data_list[idx] != NULL && ((idx < MAINFACE_NUM_MAX) && (idx < n)))
    {
        void *addr = data_list[2*idx];
        

        mainface_list[idx].data = addr;
        mainface_list[idx].type = SRC_IMG;
        if (*(uint8_t *)addr == 0x52)
        {
            mainface_list[idx].type = SRC_VIDEO;
        }
#ifdef _HONEYGUI_SIMULATOR_
#else
        uint32_t size = (uint32_t)data_list[2*idx + 1];
        if (mainface_list[idx].type == SRC_IMG && ((uint32_t)addr >= USER_RESOURCE_ADDR ) && (*((uint8_t *)addr + size - 1) == 0x01))
        {
            mainface_list[idx].type = SRC_DANMU;
        }
#endif
        gui_log("list init %d, 0x%x %d", idx, mainface_list[idx].data, mainface_list[idx].type);
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
            gui_obj_child_free(GUI_BASE(win_view));
            gui_view_create(GUI_BASE(win_view), view_rec, 0, 0, 0, 0);
            get_transmit_start = false;
            return;
        }
        
        get_transmit_start = true;
        
        gui_view_t *view_cur = gui_view_get_current();
        if (view_cur != NULL && strcmp(view_cur->base.name, "view_transmit") != 0)
        {
            view_rec = view_cur->base.name;
        }
        gui_obj_child_free(GUI_BASE(win_view));
        gui_view_create(GUI_BASE(win_view), "view_transmit", 0, 0, 0, 0);
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
        if (view_cur != NULL && strcmp(view_cur->base.name, "view_cam_ctl") != 0)
        {
            view_rec = view_cur->base.name;
            gui_view_switch_direct(gui_view_get_current(), "view_cam_ctl", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
        }

        break;
    }
    case CAST_STOP:
    {
        gui_obj_child_free(GUI_BASE(win_view));
        gui_view_create(GUI_BASE(win_view), view_rec, 0, 0, 0, 0);
        break;
    }
        
    default:
        break;
    }
}


void ui_add_resource(uint32_t payload)
{
    gui_msg_t msg = {.event = GUI_EVENT_USER_DEFINE, .sub_event = ADD_MAINFACE, .cb = (gui_msg_cb)ui_process_msg, .payload = (void *)payload};    
    gui_send_msg_to_server(&msg);
}
void ui_jump_streaming(void)
{
    gui_msg_t msg = {.event = GUI_EVENT_USER_DEFINE, .sub_event = CAST_START, .cb = (gui_msg_cb)ui_process_msg};    
    gui_send_msg_to_server(&msg);
}

void free_3d_temp_data(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (temp_3d_data != NULL && mainface_list[mainface_idx].type != SRC_3D)
    {
        gui_free(temp_3d_data);
        temp_3d_data = NULL;
    }
}




/* ============================ Live-video stream ============================
 *
 * This board owns a single live-video STP transport, created once at GUI init
 * (from easy_demoEntry.c app_init(), after flashdb_prepare() and before the
 * main view is built) via stp_instance_create() -- the transport allocates its
 * own frame pool internally through the porting allocator (stp_port_malloc).
 * It is then shared, borrowed and never re-created, by both ends through
 * gui_stream_transport_get():
 *
 *   producer : app/bluetooth/hmi_app/hmi_stream_ctrl.c (BLE RX -> stp_commit)
 *   consumer : the designer-generated gui_stream widget (stp_consume -> render)
 *
 * Config for the eBadge BLE video stream (MSV1, 360x360):
 *   - one 50 KB size class (>= the largest reassembled frame)
 *   - 20 buffers in flight (ring depth absorbing producer/consumer jitter)
 *   - 8-byte buffer alignment
 *   - STP_DROP_NONE: MSV1 is continuously inter-coded, so frames must be
 *     delivered strictly oldest-first and never dropped.
 */
#define APP_STREAM_MAX_FRAME   (35u * 1024u)   /* per-buffer cap: >= largest frame */
#define APP_STREAM_BUF_COUNT   32u             /* ring depth: frames in flight     */

static const stp_class_cfg_t s_stream_classes[] =
{
    { .buf_size = APP_STREAM_MAX_FRAME, .buf_count = APP_STREAM_BUF_COUNT },
};

/* The one transport this board owns.  NULL until app_stream_transport_init()
 * has run; both ends fetch it through the getter below. */
static stp_transport_t *s_stream_tp = NULL;

/*
 * Shared accessor for the live-video transport.  Used by:
 *   - the designer gui_stream widget (consumer) to bind on creation, and
 *   - hmi_stream_ctrl.c (BLE producer) to push reassembled frames.
 * Returns NULL before the transport has been created.
 */
stp_transport_t *gui_stream_transport_get(void)
{
    return s_stream_tp;
}

/*
 * Create the shared transport exactly once.  Called from easy_demoEntry.c
 * app_init() (SOC only), after flashdb_prepare() and before the main view is
 * created -- so the consumer's getter call already sees a valid handle, and the
 * BLE producer (a separate task, with its own NULL guard) does too.
 */
int app_stream_transport_init(void)
{
    stp_config_t cfg;
    stp_config_default(&cfg);
    cfg.align              = 8;
    cfg.classes            = s_stream_classes;
    cfg.class_count        = 1;
    cfg.drop_mode          = STP_DROP_NONE;   /* MSV1: oldest-first, never drop */
    cfg.allow_oversize_fit = true;

    s_stream_tp = stp_instance_create(&cfg);
    if (s_stream_tp == NULL)
    {
        gui_log("app_stream: stp_instance_create failed\n");
        return -1;
    }

    gui_log("app_stream: transport ready (%u buffers x %u KB)\n",
            (unsigned)APP_STREAM_BUF_COUNT,
            (unsigned)(APP_STREAM_MAX_FRAME / 1024u));
    return 0;
}
