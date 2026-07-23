#include "easy_demoMain_user.h"
#include "tp_algo.h"
#include "gui_message.h"
#include "gui_lite3d.h"
#include "gui_vfs.h"
#include "gui_lite_video.h"
#include "gui_list.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#include "stream_transport.h"   /* stp_config_t / stp_instance_create / ... */

/* ----------------------------------------------------*/
#ifdef _HONEYGUI_SIMULATOR_

#else
#include "flashdb.h"
#define USER_RESOURCE_ADDR     (RTK_NOR_FLASH_XIP_BASE + FDB_BF_DATA_PART_OFFSET)
#define USER_RESOURCE_ADDR_END (USER_RESOURCE_ADDR + FDB_BF_DATA_SIZE) // 0x00998000  //9824K Bytes

#endif

gui_win_t *win_view = NULL;

uint8_t mainface_idx = 0;
uint8_t mainface_num = 5;
mainface_src_t mainface_list[MAINFACE_NUM_MAX] =
{
    {"/image/565/wallpaper_danmu.bin",      SRC_DANMU,          NULL, "/user/hello_0040F8.bin", 0xff0040F8},
    {"/user/gltf_desc_Fox.bin",             SRC_3D,             NULL, "/user/fox_40A840.bin", 0xff40A840},
    {"/foreground_360.bin",                 SRC_IMG_SPATIAL,    NULL, "/user/eva_D0C9B9.bin", 0xffD0C9B9},
    {"/wallpaper_video.avi",                SRC_VIDEO,          NULL, "/user/wsq_F4EFD9.bin", 0xffF4EFD9},
    {"/image/565/wallpaper_static_img.bin", SRC_IMG,            NULL, "/user/pig_F8C8C8.bin", 0xffF8C8C8},
    

};
uint8_t list_index = 0;
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
static uint32_t cur_anim = 1;

/* ---------------FUNC---------------- */

static void fox_rotate_animation(void *param)
{
    (void)param;

    static bool was_pressing = false;
    static int16_t last_deltaX = 0;

    if (!enable_switch_mainface) return;

    touch_info_t *tp = tp_get_info();

    if (tp->pressing)
    {
        if (!was_pressing)
        {
            /* New touch: record the baseline, don't rotate on the press itself. */
            last_deltaX = tp->deltaX;
            was_pressing = true;
        }
        else
        {
            rot_angle += (tp->deltaX - last_deltaX) / 2.0f;
            last_deltaX = tp->deltaX;
        }
    }
    else
    {
        was_pressing = false;
    }
}

static void fox_animation_update_cb(void *obj, gui_event_t *e)
{
    (void)obj;
    (void)e;
    
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

void set_flashlight_color(void *obj)
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
    gui_view_set_bg_color(obj, color);
    gui_fb_change();

    // TO DO: adjust screen light
    
}

void create_win_del(void)
{
    if (has_created_win_del) return;
    has_created_win_del = true;
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    gui_win_t *win_del = gui_win_create(gui_obj_get_root(), "win_del", 0, 0, screen_size, screen_size);

    gui_img_t *img = gui_img_create_from_fs(win_del, 0, "/image/A8/circle_360_bg.bin", 0, screen_size / 2, screen_size, screen_size);
    gui_img_set_mode(img, IMG_SRC_OVER_MODE);
    gui_img_set_opacity(img, 122);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/delete_icon.bin", screen_size / 6, screen_size * 2/3, 0, 0);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_delete_icon_detail, GUI_EVENT_TOUCH_CLICKED, NULL);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/back_icon.bin", screen_size * 5 /6  - img->base.w, screen_size * 2/3, 0, 0);
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
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
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
            child->x = screen_size;
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

void win_timer_gsensor_cb(void *obj)
{
    GUI_UNUSED(obj);

#ifndef _HONEYGUI_SIMULATOR_
    extern bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z);
    static bool sample_valid = false;
    static bool wait_for_horizontal = false;
    static uint8_t horizontal_sample_count = 0;
    static int16_t gx_rec = 0, gy_rec = 0, gz_rec = 0;
    int16_t gx = 0, gy = 0, gz = 0;

    if (gsensor_sc7a20_read_xyz(&gx, &gy, &gz))
    {
        int32_t abs_gx = gx < 0 ? -(int32_t)gx : gx;
        int32_t abs_gy = gy < 0 ? -(int32_t)gy : gy;
        int32_t abs_gz = gz < 0 ? -(int32_t)gz : gz;

        if (wait_for_horizontal)
        {
            /* Returning to the horizontal position only rearms the gesture;
             * it must not trigger another view switch. Require several stable
             * samples to prevent sensor noise from rearming too early. */
            const int32_t horizontal_margin = 100;
            if (abs_gz >= abs_gx + horizontal_margin &&
                abs_gz >= abs_gy + horizontal_margin)
            {
                horizontal_sample_count++;
                if (horizontal_sample_count >= 3)
                {
                    wait_for_horizontal = false;
                    horizontal_sample_count = 0;
                }
            }
            else
            {
                horizontal_sample_count = 0;
            }
        }
        else if (sample_valid && gui_view_get_next() == NULL && enable_switch_mainface)
        {
            int32_t dx = (int32_t)gx - gx_rec;
            int32_t dy = (int32_t)gy - gy_rec;
            int32_t dz = (int32_t)gz - gz_rec;
            int32_t abs_dx = dx < 0 ? -dx : dx;
            int32_t abs_dy = dy < 0 ? -dy : dy;
            int32_t abs_dz = dz < 0 ? -dz : dz;

            /* Sensor +y points left and +x points down on the screen. Ignore
             * small changes and a shake whose dominant component is on z. */
            const int32_t shake_threshold = 500;
            if ((abs_dx >= shake_threshold || abs_dy >= shake_threshold) &&
                (abs_dx >= abs_dz || abs_dy >= abs_dz) && abs_dy > abs_dx)
            {
                gui_view_t *view_current = gui_view_get_current();
                const char *view_l = view_current->on_event[2]->descriptor->name;
                const char *view_r = view_current->on_event[1]->descriptor->name;
                gui_view_set_animate_step(view_current, 20);
                wait_for_horizontal = true;
                if (dy > 0)
                {
                    gui_view_switch_direct(view_current, view_l, SWITCH_INIT_STATE, SWITCH_IN_ANIMATION_RASTER_HORIZONTAL);
                }
                else
                {
                    gui_view_switch_direct(view_current, view_r, SWITCH_INIT_STATE, SWITCH_IN_ANIMATION_RASTER_HORIZONTAL_REVERSE);
                }
            }
        }
        else
        {
            sample_valid = true;
        }

        gx_rec = gx;
        gy_rec = gy;
        gz_rec = gz;
    }
#endif
}

static void *get_view_name_by_index(uint8_t idx)
{
    void *view_mainface = NULL;
    switch (idx)
    {
    case 0:
        view_mainface = "easy_demoMainView";
        break;
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
    case 8:
        view_mainface = "mainface_view_8";
        break;
    case 9:
        view_mainface = "mainface_view_9";
        break;

    default:
        break;
    }
    return view_mainface;
}

void switch_mainface(gui_obj_t *parent, uint8_t idx)
{
    gui_win_t *win = gui_win_create(parent, 0, 0, 0, 0, 0);
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
        gui_text_t *text = gui_text_create((gui_obj_t *)win, 0, 0, 0, 0, 0);
        gui_text_set((gui_text_t *)text, "Please add a mainface", GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255), 21, 20);
        gui_text_type_set((gui_text_t *)text, "/font/Inter_28pt_SemiBold_size20_bits4_bitmap.bin", FONT_SRC_FILESYS);
        gui_text_mode_set((gui_text_t *)text, MID_CENTER);

        gui_view_switch_on_event((void *)parent, "top_view", SWITCH_INIT_STATE, SWITCH_IN_FROM_TOP_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_DOWN);
        dev_mode = MODE_DEFAULT;
        return;
    }

    gui_event_code_t event_code_l = GUI_EVENT_TOUCH_MOVE_LEFT;
    gui_event_code_t event_code_r = GUI_EVENT_TOUCH_MOVE_RIGHT;
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    switch (mainface_list[idx].type)
    {
    case SRC_VIDEO:
    {
        gui_lite_video_t *vid = NULL;
#ifdef _HONEYGUI_SIMULATOR_
        vid = gui_lite_video_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
#else
        if (((uint32_t)mainface_list[idx].data) >= USER_RESOURCE_ADDR && ((uint32_t)mainface_list[idx].data) < (USER_RESOURCE_ADDR_END))
        {
            vid = gui_lite_video_create_from_mem((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
        }
        else
        {
            vid = gui_lite_video_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
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
        gui_log("%s %d 0x%x 0x%x\n", __FUNCTION__, __LINE__, USER_RESOURCE_ADDR, USER_RESOURCE_ADDR_END);
        if ((((uint32_t)mainface_list[idx].data) >= USER_RESOURCE_ADDR) && ((uint32_t)mainface_list[idx].data) < (USER_RESOURCE_ADDR_END))
        {
            gui_log("%s %d 0x%x\n", __FUNCTION__, __LINE__, mainface_list[idx].data);
            img = gui_img_create_from_mem((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
        }
        else
        {
            img = gui_img_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
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
        fox_3d = l3_create_model_fs((void *)mainface_list[idx].data, L3_DRAW_FRONT_AND_SORT, 0, 0, screen_size, screen_size);

        l3_set_global_transform(fox_3d, (l3_global_transform_cb)fox_global_cb);
        l3_gltf_set_active_animation(fox_3d, cur_anim);

        gui_lite3d_t *lite3d_fox = gui_lite3d_create(win, "lite3d-widget",
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
        if (((uint32_t)mainface_list[idx].data) >= USER_RESOURCE_ADDR && ((uint32_t)mainface_list[idx].data) < (USER_RESOURCE_ADDR_END))
        {
            gui_log("%s %d 0x%x\n", __FUNCTION__, __LINE__, mainface_list[idx].data);
            img_0 = gui_img_create_from_mem((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
        }
        else
        {
            img_0 = gui_img_create_from_fs((void *)win, 0, mainface_list[idx].data, 0, 0, 0, 0);
        }
#endif
        gui_img_set_mode(img_0, IMG_BYPASS_MODE);
        int16_t img_y = (screen_size - img_0->base.h) / 2;
        gui_obj_move((void *)img_0, screen_size, img_y);
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
        gui_view_switch_on_event((void *)parent, "view_mainface_list", SWITCH_INIT_STATE, SWITCH_IN_FROM_BOTTOM_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_UP);
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


#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
#include "hmi_ble_central.h"
#include "hmi_l2.h"
extern bool hmi_ble_central_send_file(uint8_t type, const uint8_t *src, uint32_t total,
                               const char *fname, xfer_client_done_cb_t done_cb);

void done_cb(T_XFER_CLIENT_RESULT result, uint32_t bytes_sent)
{
    printf("done_cb %d, sent %d\n", result, bytes_sent);
}

#endif


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
    uint32_t addr = 0, len = 0;
    extern fdb_bf_t   app_get_bf(void);
    fdb_bf_get_addr(app_get_bf(), "bf_0", &addr, &len);   // 拿映射地址+长度
    gui_log("send addr 0x%x len %d\n", addr, len);

    hmi_ble_central_send_file(HMI_L2_XFER_TYPE_IMAGE,
                                (const uint8_t *)addr, len,
                                "share_0", done_cb);      // on_done_cb(result, bytes) 报进度/结果
#endif
    
}

extern bool hmi_ble_central_start_scan(void);                 // 停广播、开扫描
extern uint8_t hmi_ble_central_get_dev_count(void);  // 刷新列表用
extern bool hmi_ble_central_get_dev(uint8_t idx, uint8_t bd_addr[6], uint8_t *addr_type,
                             int8_t *rssi, char *name, uint8_t name_len);  // 填列表项
extern bool hmi_ble_central_connect(uint8_t idx);                   // 连选中项
extern bool hmi_ble_central_disconnect(void);                 // 断开 → 自动回广播态

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
    gui_log("click_screen_light_icon %d\n", screen_light_idx);
    static uint8_t n = 0;
    static bool connect = 0;
    switch (screen_light_idx)
    {
    case 0:
        break;
    case 1:
        hmi_ble_central_start_scan(); 
        gui_log("hmi_ble_central_start_scan\n");
        break;
    case 2:
        n = hmi_ble_central_get_dev_count();
        gui_log("hmi_ble_central_get_dev_count %d\n", n);
        for(uint8_t i=0; i<n; i++)
        {
            uint8_t idx; 
            uint8_t bd_addr[6]; 
            uint8_t addr_type;
            int8_t rssi;
            char name[32];
            uint8_t name_len;
            hmi_ble_central_get_dev(i, bd_addr, &addr_type, &rssi, name, sizeof(name));
            gui_log("%d name %s %x:%x:%x:%x:%x:%x\n", i, name, bd_addr[5]&0xff, bd_addr[4]&0xff, bd_addr[3]&0xff,bd_addr[2]&0xff, bd_addr[1]&0xff, bd_addr[0]&0xff);
        }
        break;
    case 3:
        if(n && !connect)
        {
            connect = hmi_ble_central_connect(0);
            gui_log("hmi_ble_central_connect %d\n", connect);
        }
        break;
    case 4:
        if(connect)
        {
            connect = hmi_ble_central_disconnect() ? 0:1;
            gui_log("hmi_ble_central_disconnect %d\n", connect);
        }
        break;
    case 5:
        break;
    default:
        break;
    }

#endif
}

void click_delete_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    if (mainface_num == 0) return;

    dev_mode = MODE_DELETE;
    void *view_mainface = get_view_name_by_index(mainface_idx);
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
            case 9:
                view_left = "mainface_view_8";
                break;
            case 10:
                view_left = "mainface_view_9";
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
    PACKET_HEADER_T *header = (PACKET_HEADER_T *)info[0]; 

    
    mainface_list[mainface_num].raw = (void *)header;
    mainface_list[mainface_num].type = RES_TYPE(header);
    mainface_list[mainface_num].data = RES_DATA_X(header, 0);
    mainface_list[mainface_num].img_preview = RES_DATA_X(header, 1);
    mainface_list[mainface_num].color = RES_COLOR_BG(header);

    gui_log("raw 0x%x type %d, data 0x%x, img 0x%x, color 0x%x of0 %d of1 %d\n", mainface_list[mainface_num].raw,\
    mainface_list[mainface_num].type, mainface_list[mainface_num].data,mainface_list[mainface_num].img_preview, mainface_list[mainface_num].color,\
    RES_OFFSET_X(header, 0), RES_OFFSET_X(header, 1));

    mainface_num++;

    mainface_idx = mainface_num - 1;
    void *view = get_view_name_by_index(mainface_idx);

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
    gui_view_switch_on_event(gui_view_get_current(), "view_mainface_list", SWITCH_INIT_STATE, SWITCH_IN_FROM_BOTTOM_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_UP);
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
    uint8_t reserved = 3;
    if (data_list == NULL || !n) return idx;
    


    while (data_list[idx] != NULL && ((idx < MAINFACE_NUM_MAX) && (idx < n)))
    {
        void *addr = data_list[2*idx];
        uint8_t list_idx = idx + reserved;
        

        mainface_list[list_idx].data = addr;
        mainface_list[list_idx].type = SRC_IMG;
        
#ifdef _HONEYGUI_SIMULATOR_
#else
        // uint32_t size = (uint32_t)data_list[2*idx + 1];
        if((uint32_t)addr >= USER_RESOURCE_ADDR )
        {
            PACKET_HEADER_T *header = (PACKET_HEADER_T *)addr; 

            mainface_list[list_idx].raw = (void *)header;
            mainface_list[list_idx].type = RES_TYPE(header);
            mainface_list[list_idx].data = RES_DATA_X(header, 0);
            mainface_list[list_idx].img_preview = RES_DATA_X(header, 1);
            mainface_list[list_idx].color = RES_COLOR_BG(header);
        }
        else
        {
            if (*(uint8_t *)addr == 0x52)
            {
                mainface_list[list_idx].type = SRC_VIDEO;
            }
        }



#endif
        gui_log("list init %d, 0x%x %d", idx, mainface_list[list_idx].data, mainface_list[list_idx].type);
        idx++;
    }
    mainface_num = idx + reserved;
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

void switch_in_mainface_0(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 0);
}

void switch_in_mainface_1(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 1);
}

void switch_in_mainface_2(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 2);
}

void switch_in_mainface_3(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 3);
}

void switch_in_mainface_4(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 4);
}

void switch_in_mainface_5(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 5);
}

void switch_in_mainface_6(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 6);
}

void switch_in_mainface_7(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 7);
}

void switch_in_mainface_8(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 8);
}

void switch_in_mainface_9(gui_view_t *view)
{
    GUI_UNUSED(view);
    switch_mainface(GUI_BASE(view), 9);
}

typedef struct
{
    float r;
    float g;
    float b;
} color_rgb_f_t;

static float color_mix(float a, float b, float t)
{
    return a + (b - a) * t;
}

static uint8_t color_clamp_byte(float value)
{
    if (value <= 0.0f)
    {
        return 0;
    }
    if (value >= 255.0f)
    {
        return 255;
    }
    return (uint8_t)(value + 0.5f);
}

#if COLOR_TRANSITION_METHOD == COLOR_TRANSITION_GAMMA_LINEAR || \
    COLOR_TRANSITION_METHOD == COLOR_TRANSITION_CIELAB || \
    COLOR_TRANSITION_METHOD == COLOR_TRANSITION_OKLAB
static float color_srgb_to_linear(float value)
{
    value /= 255.0f;
    return value <= 0.04045f ? value / 12.92f : powf((value + 0.055f) / 1.055f, 2.4f);
}

static float color_linear_to_srgb(float value)
{
    value = value <= 0.0031308f ? value * 12.92f : 1.055f * powf(value, 1.0f / 2.4f) - 0.055f;
    return color_clamp_byte(value * 255.0f);
}
#endif

#if COLOR_TRANSITION_METHOD == COLOR_TRANSITION_HSL
typedef struct
{
    float h;
    float s;
    float l;
} color_hsl_f_t;

static color_hsl_f_t color_rgb_to_hsl(color_rgb_f_t rgb)
{
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    float max = fmaxf(r, fmaxf(g, b));
    float min = fminf(r, fminf(g, b));
    float delta = max - min;
    color_hsl_f_t hsl = {0.0f, 0.0f, (max + min) / 2.0f};

    if (delta != 0.0f)
    {
        if (max == r)
        {
            hsl.h = fmodf((g - b) / delta, 6.0f);
        }
        else if (max == g)
        {
            hsl.h = (b - r) / delta + 2.0f;
        }
        else
        {
            hsl.h = (r - g) / delta + 4.0f;
        }
        hsl.h *= 60.0f;
        if (hsl.h < 0.0f)
        {
            hsl.h += 360.0f;
        }
        hsl.s = delta / (1.0f - fabsf(2.0f * hsl.l - 1.0f));
    }
    return hsl;
}

static color_rgb_f_t color_hsl_to_rgb(color_hsl_f_t hsl)
{
    float c = (1.0f - fabsf(2.0f * hsl.l - 1.0f)) * hsl.s;
    float x = c * (1.0f - fabsf(fmodf(hsl.h / 60.0f, 2.0f) - 1.0f));
    float m = hsl.l - c / 2.0f;
    color_rgb_f_t rgb = {0.0f, 0.0f, 0.0f};

    if (hsl.h < 60.0f)
    {
        rgb.r = c; rgb.g = x;
    }
    else if (hsl.h < 120.0f)
    {
        rgb.r = x; rgb.g = c;
    }
    else if (hsl.h < 180.0f)
    {
        rgb.g = c; rgb.b = x;
    }
    else if (hsl.h < 240.0f)
    {
        rgb.g = x; rgb.b = c;
    }
    else if (hsl.h < 300.0f)
    {
        rgb.r = x; rgb.b = c;
    }
    else
    {
        rgb.r = c; rgb.b = x;
    }
    rgb.r = (rgb.r + m) * 255.0f;
    rgb.g = (rgb.g + m) * 255.0f;
    rgb.b = (rgb.b + m) * 255.0f;
    return rgb;
}
#endif

#if COLOR_TRANSITION_METHOD == COLOR_TRANSITION_CIELAB || \
    COLOR_TRANSITION_METHOD == COLOR_TRANSITION_OKLAB
typedef struct
{
    float l;
    float a;
    float b;
} color_lab_f_t;
#endif

#if COLOR_TRANSITION_METHOD == COLOR_TRANSITION_CIELAB
static float color_lab_f(float value)
{
    return value > 0.008856f ? cbrtf(value) : 7.787f * value + 16.0f / 116.0f;
}

static color_lab_f_t color_rgb_to_lab(color_rgb_f_t rgb)
{
    float r = color_srgb_to_linear(rgb.r);
    float g = color_srgb_to_linear(rgb.g);
    float b = color_srgb_to_linear(rgb.b);
    float fx = color_lab_f((r * 0.4124564f + g * 0.3575761f + b * 0.1804375f) / 0.95047f);
    float fy = color_lab_f(r * 0.2126729f + g * 0.7151522f + b * 0.0721750f);
    float fz = color_lab_f((r * 0.0193339f + g * 0.1191920f + b * 0.9503041f) / 1.08883f);
    color_lab_f_t lab = {116.0f * fy - 16.0f, 500.0f * (fx - fy), 200.0f * (fy - fz)};
    return lab;
}

static float color_lab_inverse_f(float value)
{
    float cube = value * value * value;
    return cube > 0.008856f ? cube : (value - 16.0f / 116.0f) / 7.787f;
}

static color_rgb_f_t color_lab_to_rgb(color_lab_f_t lab)
{
    float fy = (lab.l + 16.0f) / 116.0f;
    float x = color_lab_inverse_f(fy + lab.a / 500.0f) * 0.95047f;
    float y = color_lab_inverse_f(fy);
    float z = color_lab_inverse_f(fy - lab.b / 200.0f) * 1.08883f;
    color_rgb_f_t rgb = {
        color_linear_to_srgb(x * 3.2404542f - y * 1.5371385f - z * 0.4985314f),
        color_linear_to_srgb(-x * 0.9692660f + y * 1.8760108f + z * 0.0415560f),
        color_linear_to_srgb(x * 0.0556434f - y * 0.2040259f + z * 1.0572252f)
    };
    return rgb;
}
#endif

#if COLOR_TRANSITION_METHOD == COLOR_TRANSITION_OKLAB
static color_lab_f_t color_rgb_to_oklab(color_rgb_f_t rgb)
{
    float r = color_srgb_to_linear(rgb.r);
    float g = color_srgb_to_linear(rgb.g);
    float b = color_srgb_to_linear(rgb.b);
    float l = cbrtf(0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b);
    float m = cbrtf(0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b);
    float s = cbrtf(0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b);
    color_lab_f_t lab = {
        0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s,
        1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s,
        0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s
    };
    return lab;
}

static color_rgb_f_t color_oklab_to_rgb(color_lab_f_t lab)
{
    float l = lab.l + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
    float m = lab.l - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
    float s = lab.l - 0.0894841775f * lab.a - 1.2914855480f * lab.b;
    l = l * l * l;
    m = m * m * m;
    s = s * s * s;
    color_rgb_f_t rgb = {
        color_linear_to_srgb(4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s),
        color_linear_to_srgb(-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s),
        color_linear_to_srgb(-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s)
    };
    return rgb;
}
#endif

static float color_apply_easing(float t)
{
#if COLOR_TRANSITION_EASING == COLOR_EASING_LINEAR
    return t;
#elif COLOR_TRANSITION_EASING == COLOR_EASING_SMOOTHSTEP
    return t * t * (3.0f - 2.0f * t);
#elif COLOR_TRANSITION_EASING == COLOR_EASING_EASE_IN_OUT_CUBIC
    if (t < 0.5f)
    {
        return 4.0f * t * t * t;
    }
    t = -2.0f * t + 2.0f;
    return 1.0f - t * t * t / 2.0f;
#elif COLOR_TRANSITION_EASING == COLOR_EASING_EASE_OUT_CUBIC
    t = 1.0f - t;
    return 1.0f - t * t * t;
#else
#error "Unsupported COLOR_TRANSITION_EASING"
#endif
}

static gui_color_t color_interpolate(gui_color_t origin, gui_color_t target, float t)
{
    color_rgb_f_t a = {origin.color.rgba.r, origin.color.rgba.g, origin.color.rgba.b};
    color_rgb_f_t b = {target.color.rgba.r, target.color.rgba.g, target.color.rgba.b};
    color_rgb_f_t result;

#if COLOR_TRANSITION_METHOD == COLOR_TRANSITION_SRGB
    result.r = color_mix(a.r, b.r, t);
    result.g = color_mix(a.g, b.g, t);
    result.b = color_mix(a.b, b.b, t);
#elif COLOR_TRANSITION_METHOD == COLOR_TRANSITION_GAMMA_LINEAR
    result.r = color_linear_to_srgb(color_mix(color_srgb_to_linear(a.r), color_srgb_to_linear(b.r), t));
    result.g = color_linear_to_srgb(color_mix(color_srgb_to_linear(a.g), color_srgb_to_linear(b.g), t));
    result.b = color_linear_to_srgb(color_mix(color_srgb_to_linear(a.b), color_srgb_to_linear(b.b), t));
#elif COLOR_TRANSITION_METHOD == COLOR_TRANSITION_HSL
    color_hsl_f_t hsl_a = color_rgb_to_hsl(a);
    color_hsl_f_t hsl_b = color_rgb_to_hsl(b);
    float delta_h = hsl_b.h - hsl_a.h;
    if (delta_h > 180.0f) delta_h -= 360.0f;
    if (delta_h < -180.0f) delta_h += 360.0f;
    color_hsl_f_t hsl = {
        fmodf(hsl_a.h + delta_h * t + 360.0f, 360.0f),
        color_mix(hsl_a.s, hsl_b.s, t),
        color_mix(hsl_a.l, hsl_b.l, t)
    };
    result = color_hsl_to_rgb(hsl);
#elif COLOR_TRANSITION_METHOD == COLOR_TRANSITION_CIELAB
    color_lab_f_t lab_a = color_rgb_to_lab(a);
    color_lab_f_t lab_b = color_rgb_to_lab(b);
    color_lab_f_t lab = {
        color_mix(lab_a.l, lab_b.l, t),
        color_mix(lab_a.a, lab_b.a, t),
        color_mix(lab_a.b, lab_b.b, t)
    };
    result = color_lab_to_rgb(lab);
#elif COLOR_TRANSITION_METHOD == COLOR_TRANSITION_OKLAB
    color_lab_f_t lab_a = color_rgb_to_oklab(a);
    color_lab_f_t lab_b = color_rgb_to_oklab(b);
    color_lab_f_t lab = {
        color_mix(lab_a.l, lab_b.l, t),
        color_mix(lab_a.a, lab_b.a, t),
        color_mix(lab_a.b, lab_b.b, t)
    };
    result = color_oklab_to_rgb(lab);
#else
#error "Unsupported COLOR_TRANSITION_METHOD"
#endif

    gui_color_t color;
    color.color.rgba.a = color_clamp_byte(color_mix(origin.color.rgba.a, target.color.rgba.a, t));
    color.color.rgba.r = color_clamp_byte(result.r);
    color.color.rgba.g = color_clamp_byte(result.g);
    color.color.rgba.b = color_clamp_byte(result.b);
    return color;
}

static void lst_mainface_note_design(gui_obj_t *obj, void *param)
{
    GUI_UNUSED(param);
    
    // Cast obj to gui_list_note_t * type
    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index % mainface_num;
    index += mainface_num;
    index %= mainface_num;
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    uint16_t pic_size = 160;
    uint16_t img_y = (screen_size - pic_size) / 2 * 3/4;

#ifdef _HONEYGUI_SIMULATOR_
    gui_img_t *img = gui_img_create_from_fs(obj, 0, mainface_list[index].img_preview, 0, img_y, 0, 0);
#else
    gui_img_t *img = NULL;
    if (((uint32_t)mainface_list[index].data) >= USER_RESOURCE_ADDR && ((uint32_t)mainface_list[index].data) < (USER_RESOURCE_ADDR_END))
    {
        img = gui_img_create_from_mem(obj, 0, mainface_list[index].img_preview, 0, img_y, 0, 0);
    }
    else
    {
        img = gui_img_create_from_fs(obj, 0, mainface_list[index].img_preview, 0, img_y, 0, 0);
    }
#endif
    gui_img_set_mode(img, IMG_SRC_OVER_MODE);
    img->need_clip = false;
}

static void note_timer_cb(void *obj)
{
    static uint8_t cnt = 0;
    const uint8_t cnt_max = 20;
    gui_list_note_t *note = (gui_list_note_t *)obj;
    gui_list_t *list = (gui_list_t *)note->base.parent;

    cnt++;
    static gui_color_t color_origin;
    gui_color_t color_target;
    gui_color_t color_bg;
    color_target.color.argb_full = mainface_list[list_index].color;
    if (cnt == 1)
    {
        color_origin = gui_view_get_current()->bg_color;
    }

    float progress = color_apply_easing((float)cnt / (float)cnt_max);
    color_bg = color_interpolate(color_origin, color_target, progress);

    gui_view_set_bg_color(gui_view_get_current(), color_bg);


    gui_fb_change();

    if (cnt >= cnt_max)
    {
        cnt = 0;
        gui_obj_stop_timer((void *)note);
        gui_list_enable_scroll(list, true);
    }
    
}

static void list_timer_cb(void *obj)
{
    gui_list_t *list = (gui_list_t *)obj;
    int16_t offset = list->offset;
    static bool moved = false;
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;

    if (offset % (screen_size / 2) == 0)
    {
        if (moved)
        {
            moved = false;
            gui_list_note_t *note_right = (void *)gui_list_entry(list->base.child_list.next, gui_obj_t, brother_list);
            gui_list_note_t *note_center = (void *)gui_list_entry(note_right->base.brother_list.next, gui_obj_t, brother_list);
            int16_t index = note_center->index % mainface_num;
            index += mainface_num;
            index %= mainface_num;
            if (index != list_index)
            {
                list_index = index;
                gui_obj_create_timer((void *)note_center, 10, true, note_timer_cb);
                gui_obj_start_timer((void *)note_center);
                gui_list_enable_scroll(list, false);
            }
        }
    }
    else
    {
        moved = true;
    }
}

static void click_2_mainface_view(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    mainface_idx = list_index;
    gui_view_switch_direct(gui_view_get_current(), get_view_name_by_index(mainface_idx), SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
}

static void click_button_2_share(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

#ifndef _HONEYGUI_SIMULATOR_
    uint32_t addr = mainface_list[mainface_idx].raw;
    uint32_t len = RES_SIZE(addr);
    hmi_ble_central_send_file(HMI_L2_XFER_TYPE_IMAGE,
                                (const uint8_t *)addr, len,
                                "share_0", done_cb);      // on_done_cb(result, bytes) 报进度/结果
#endif
}

static void click_button_2_connect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    dev_mode = MODE_SHARE;
    gui_view_switch_direct(gui_view_get_current(), "ShareConnView", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
}

static void click_button_2_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

#ifndef _HONEYGUI_SIMULATOR_
    if (hmi_ble_central_disconnect())
    {
        gui_log("disconnect bd_dev success!");
    }
    else
    {
        gui_log("disconnect bd_dev failed!");
    }
#endif
    if (gui_view_get_next() == NULL)
    {
        dev_mode = MODE_DEFAULT;
        gui_view_t *view_current = gui_view_get_current();
        gui_obj_t *dev_send = gui_list_entry(view_current->base.child_list.prev, gui_obj_t, brother_list);
        gui_obj_t *dev_disconn = gui_list_entry(dev_send->brother_list.prev, gui_obj_t, brother_list);

        gui_obj_tree_free(dev_send);
        gui_dispdev_t *dc = gui_get_dc();
        uint16_t screen_size = dc->screen_width;
        uint16_t pic_size = 100;
        gui_obj_move(dev_disconn, (screen_size - pic_size) / 2, screen_size * 2/3);
        gui_img_set_src((void *)dev_disconn, "/image/dev_conn_icon.bin", IMG_SRC_FILESYS);
        dev_disconn->event_dsc[0].event_cb = click_button_2_connect;
        gui_obj_add_event_cb(view_current, (gui_event_cb_t)click_2_mainface_view, GUI_EVENT_TOUCH_CLICKED, NULL);
        gui_view_switch_on_event(view_current, get_view_name_by_index(mainface_idx), SWITCH_OUT_TO_BOTTOM_USE_TRANSLATION, SWITCH_INIT_STATE, GUI_EVENT_TOUCH_MOVE_DOWN);
    }
}

void switch_in_mainface_list(gui_view_t *view)
{
    list_index = mainface_idx;
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    uint16_t pic_size = 160;
    gui_list_t *lst_mainface = gui_list_create((gui_obj_t *)view, "lst_mainface", -pic_size / 2, 0, screen_size + pic_size, screen_size, 
                                pic_size, screen_size / 2 - pic_size, HORIZONTAL, lst_mainface_note_design, NULL, false);
    gui_list_set_style(lst_mainface, LIST_CLASSIC);
    gui_list_set_note_num(lst_mainface, mainface_num);
    gui_list_set_auto_align(lst_mainface, true);
    gui_list_enable_loop(lst_mainface, true);
    gui_list_set_inertia(lst_mainface, false);
    gui_list_set_offset(lst_mainface, (screen_size / 2) * (1 - list_index));

    gui_obj_create_timer((void *)lst_mainface, 10, true, list_timer_cb);
    gui_obj_start_timer((void *)lst_mainface);

    /* Device key */
    pic_size = 100;
    if (dev_mode == MODE_SHARE)
    {
        gui_img_t *dev_disconn = gui_img_create_from_fs(view, 0, "/image/dev_disconn_icon.bin", screen_size / 6, screen_size * 2/3, 0, 0);
        gui_img_set_mode(dev_disconn, IMG_SRC_OVER_MODE);
        gui_img_t *dev_send = gui_img_create_from_fs(view, 0, "/image/dev_send_icon.bin", screen_size * 5 /6  - pic_size, screen_size * 2/3, 0, 0);
        gui_img_set_mode(dev_send, IMG_SRC_OVER_MODE);
        gui_obj_add_event_cb(dev_disconn, (gui_event_cb_t)click_button_2_disconnect, GUI_EVENT_TOUCH_CLICKED, NULL);
        gui_obj_add_event_cb(dev_send, (gui_event_cb_t)click_button_2_share, GUI_EVENT_TOUCH_CLICKED, NULL);
    }
    else
    {
        gui_img_t *dev_conn = gui_img_create_from_fs(view, 0, "/image/dev_send_icon.bin", (screen_size - pic_size) / 2, screen_size * 2/3, 0, 0);
        gui_img_set_mode(dev_conn, IMG_SRC_OVER_MODE);
        gui_obj_add_event_cb(dev_conn, (gui_event_cb_t)click_button_2_connect, GUI_EVENT_TOUCH_CLICKED, NULL);

        gui_obj_add_event_cb(view, (gui_event_cb_t)click_2_mainface_view, GUI_EVENT_TOUCH_CLICKED, NULL);
        gui_view_switch_on_event(view, get_view_name_by_index(mainface_idx), SWITCH_OUT_TO_BOTTOM_USE_TRANSLATION, SWITCH_INIT_STATE, GUI_EVENT_TOUCH_MOVE_DOWN);
    }

    gui_color_t bg_color;
    bg_color.color.argb_full = mainface_list[list_index].color;
    gui_view_set_bg_color(view, bg_color);
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
