#include "easy_demoMain_user.h"
#include "tp_algo.h"
#include "gui_vfs.h"
#include "gui_lite_video.h"
#include "gui_img.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <time.h>

static bool coin_status = true; // true: positive, false: negative
static bool coin_flipping = false;
static uint32_t xorshift_state = 0x2545F491u;

static uint32_t xorshift32(void)
{
    uint32_t x = xorshift_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xorshift_state = x;
    return x;
}

static void img_cb(void *obj)
{
    gui_obj_t *img = obj;
    gui_lite_video_t *vid = (void *)gui_list_entry(img->brother_list.next, gui_obj_t, brother_list);
    if (vid->state == GUI_VIDEO_STATE_STOP)
    {
        coin_flipping = false;
        gui_obj_tree_free_async(vid);
        gui_obj_stop_timer(obj);
    }
}

static void click_flip_coin(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    if (coin_flipping)
    {
        return;
    }
    coin_flipping = true;
    coin_status = (xorshift32() >> 31) & 1;

    void *vid_addr = "/coin_flip_2_yes.avi";
    void *img_addr = "/coin_flip_yes.bin";
    if (!coin_status)
    {
        vid_addr = "/coin_flip_2_no.avi";
        img_addr = "/coin_flip_no.bin";
    }
    gui_lite_video_t *vid = gui_lite_video_create_from_fs(GUI_BASE(obj)->parent, 0, vid_addr, 0, 0, 0, 0);
    gui_lite_video_set_frame_rate((gui_lite_video_t *)vid, 30.f);
    gui_lite_video_set_repeat_count((gui_lite_video_t *)vid, 0); // 0 = play once
    gui_lite_video_set_state((gui_lite_video_t *)vid, GUI_VIDEO_STATE_PLAYING);
    gui_img_set_src(obj, img_addr, IMG_SRC_FILESYS);

    gui_obj_create_timer(obj, 100, true, img_cb);
    gui_obj_start_timer(obj);
}

static void view_cb(void *obj)
{
    GUI_UNUSED(obj);
#ifndef _HONEYGUI_SIMULATOR_
    extern bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z);
    static bool initialized = false;
    static bool shake_locked = false;
    static uint8_t quiet_count = 0;
    static int32_t gravity_x = 0;
    static int32_t gravity_y = 0;
    static int32_t gravity_z = 0;
    int16_t gx, gy, gz;

    if (!gsensor_sc7a20_read_xyz(&gx, &gy, &gz))
    {
        return;
    }

    if (!initialized)
    {
        gravity_x = gx;
        gravity_y = gy;
        gravity_z = gz;
        initialized = true;
        return;
    }

    gravity_x += ((int32_t)gx - gravity_x) / 8;
    gravity_y += ((int32_t)gy - gravity_y) / 8;
    gravity_z += ((int32_t)gz - gravity_z) / 8;

    int32_t motion_x = (int32_t)gx - gravity_x;
    int32_t motion_y = (int32_t)gy - gravity_y;
    int32_t motion_z = (int32_t)gz - gravity_z;
    int32_t motion = (motion_x < 0 ? -motion_x : motion_x) +
                     (motion_y < 0 ? -motion_y : motion_y) +
                     (motion_z < 0 ? -motion_z : motion_z);

    if (shake_locked)
    {
        if (motion < 100)
        {
            if (++quiet_count >= 5)
            {
                shake_locked = false;
                quiet_count = 0;
            }
        }
        else
        {
            quiet_count = 0;
        }
        return;
    }

    if (motion >= 500)
    {
        shake_locked = true;
        quiet_count = 0;
        click_flip_coin(obj, NULL);
    }
#endif
}

void flip_coin_init(gui_obj_t *parent)
{
#ifndef _HONEYGUI_SIMULATOR_
    int16_t gx, gy, gz;
    extern bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z);
    if (gsensor_sc7a20_read_xyz(&gx, &gy, &gz))
    {
        xorshift_state ^= ((uint32_t)(uint16_t)gx * 31u)
                        ^ ((uint32_t)(uint16_t)gy * 17u)
                        ^ (uint32_t)(uint16_t)gz
                        ^ (xorshift_state << 1);
    }
#endif
    coin_flipping = false;
    void *img_addr = "/coin_flip_yes.bin";
    if (!coin_status)
    {
        img_addr = "/coin_flip_no.bin";
    }
    gui_img_t *img = gui_img_create_from_fs(parent, 0, img_addr, 0, 0, 0, 0);
    gui_img_set_mode(img, IMG_BYPASS_MODE);
    gui_obj_create_timer(img, 10, true, view_cb);
    gui_obj_start_timer(img);

    gui_obj_add_event_cb(img, (gui_event_cb_t)click_flip_coin, GUI_EVENT_TOUCH_CLICKED, NULL);
}
