#include "easy_demoMain_user.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "gui_arc.h"
#include "gui_list.h"
#include "gui_lite3d.h"
#include "gui_lite_video.h"
#include "gui_message.h"
#include "gui_vfs.h"
#include "stream_transport.h"
#include "tp_algo.h"

#ifndef _HONEYGUI_SIMULATOR_
#include "flashdb.h"
#include "hmi_ble_central.h"
#include "hmi_l2.h"
#include "hmi_l2_cmd_remote.h"

#define USER_RESOURCE_ADDR     (RTK_NOR_FLASH_XIP_BASE + FDB_BF_DATA_PART_OFFSET)
#define USER_RESOURCE_ADDR_END (USER_RESOURCE_ADDR + FDB_BF_DATA_SIZE)
#endif

/*============================================================================*
 * Mainface state and resources
 *============================================================================*/

gui_win_t *win_view = NULL;

uint8_t mainface_idx = 0;
uint8_t mainface_num = 3;
mainface_src_t mainface_list[MAINFACE_NUM_MAX] =
{
    {"/image/565/wallpaper_danmu.bin",      SRC_DANMU,       NULL, "/user/hello_0040F8.bin", 0xff0040F8},
    {"/user/gltf_desc_Fox.bin",             SRC_3D,          NULL, "/user/fox_40A840.bin",   0xff40A840},
    {"/image/565/wallpaper_static_img.bin", SRC_IMG,         NULL, "/user/pig_F8C8C8.bin",  0xffF8C8C8},
};

static uint8_t list_index = 0;
static bool enable_switch_mainface = true;
static bool list_moved = false;

bool is_auto_sleep_mode = false;
bool is_bt_connect = false;
bool is_dev_connect = false;
bool is_displaying_mainface = false;
MODE_TYPE dev_mode = MODE_DEFAULT;

uint8_t soc_val = 100;
uint8_t screen_light_idx = 5; /* 0~5 */
int8_t fl_color_idx = 0;      /* white, red, orange, yellow, green, blue, indigo, violet */

static SHARE_FILE_TYPE share_file_status = SHARE_DEFAULT;
static bool is_link_error = false;
static bool has_released = true;
static bool has_created_win_del = false;

/* Reuse three view slots for seamless left/right mainface switching. */
static const char *view_rec = "easy_demoMainView";
static const char *const view_array[3] =
{
    "easy_demoMainView",
    "mainface_view_1",
    "mainface_view_2",
};
static int8_t view_idx_curr = 0;
static int8_t view_idx_next = 0;

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

/* State for the 3D fox mainface. */
static float rot_angle = 45.0f;
static l3_model_base_t *fox_3d = NULL;
static uint32_t cur_anim = 1;

typedef struct
{
    float r;
    float g;
    float b;
} color_rgb_f_t;

/*============================================================================*
 * File-local functions
 *============================================================================*/

#ifndef _HONEYGUI_SIMULATOR_
extern bool hmi_ble_central_send_file(uint8_t type, const uint8_t *src,
                                      uint32_t total, const char *fname,
                                      xfer_client_done_cb_t done_cb);
extern bool hmi_ble_central_get_send_progress(uint32_t *bytes_sent,
                                               uint32_t *total,
                                               T_XFER_CLIENT_PHASE *phase);
extern bool hmi_ble_central_is_ready(void);
extern bool hmi_ble_central_disconnect(void);
#endif

static void mainface_list_delete(void *obj);
static void img_zoom_timer_cb(void *param);
#ifndef _HONEYGUI_SIMULATOR_
static void done_cb(T_XFER_CLIENT_RESULT result, uint32_t bytes_sent);
#endif
static void note_timer_cb(void *obj);
static void list_timer_cb(void *obj);
static void prog_arc_timer(void *param);
static void click_2_mainface_view(void *obj, gui_event_t *e);
static void click_button_2_share(void *obj, gui_event_t *e);
static void click_button_2_connect(void *obj, gui_event_t *e);
static void click_button_2_disconnect(void *obj, gui_event_t *e);
static void mainface_list_view_timer_cb(void *obj);
static void on_remote_state_changed(void);
static void on_remote_shot_ready(uint16_t shot_id);
static void on_remote_ctrl_result(uint8_t key, uint8_t code);
void ui_remote_change(uint32_t payload);

/*============================================================================*
 * Common view and 3D helpers
 *============================================================================*/

/*
 * Rotate the 3D model from horizontal drag deltas.
 * The initial press only establishes the baseline position.
 */
static void fox_rotate_animation(void *param)
{
    static bool was_pressing = false;
    static int16_t last_delta_x = 0;

    GUI_UNUSED(param);

    if (!enable_switch_mainface)
    {
        return;
    }

    touch_info_t *tp = tp_get_info();
    if (tp->pressing)
    {
        if (!was_pressing)
        {
            last_delta_x = tp->deltaX;
            was_pressing = true;
        }
        else
        {
            rot_angle += (tp->deltaX - last_delta_x) / 2.0f;
            last_delta_x = tp->deltaX;
        }
    }
    else
    {
        was_pressing = false;
    }
}

/* Cycle through the available glTF animations when the model is clicked. */
static void fox_animation_update_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    uint32_t anim_count = l3_gltf_get_animation_count(fox_3d);
    if (anim_count > 0)
    {
        cur_anim = (cur_anim + 1) % anim_count;
        l3_gltf_set_active_animation(fox_3d, cur_anim);
    }
}

/* Apply the camera parameters and current model rotation each frame. */
static void fox_global_cb(l3_model_base_t *model)
{
    l3_camera_UVN_initialize(&model->camera, l3_4d_point(0, 0, 0),
                             l3_4d_point(0, 0, 1), 1, 32767, 90,
                             model->viewPortWidth, model->viewPortHeight);
    l3_world_initialize(&model->world, 0, 10, 25, 0, rot_angle, 0, 5);
}

/* Destroy and recreate the current view on the GUI thread. */
static void msg_2_regenerate_view(void *msg)
{
    GUI_UNUSED(msg);
    void *view_rec = (void *)gui_view_get_current()->base.name;
    gui_obj_child_free(GUI_BASE(win_view));
    gui_view_create(GUI_BASE(win_view), view_rec, 0, 0, 0, 0);
}

/* Create the delete-mode overlay and its delete/back action buttons. */
static void create_win_del(void)
{
    if (has_created_win_del)
    {
        return;
    }
    has_created_win_del = true;

    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    gui_win_t *win_del = gui_win_create(gui_obj_get_root(), "win_del", 0, 0,
                                        screen_size, screen_size);

    gui_img_t *img = gui_img_create_from_fs(win_del, 0, "/image/A8/circle_360_bg.bin",
                                            0, screen_size / 2,
                                            screen_size, screen_size);
    gui_img_set_mode(img, IMG_SRC_OVER_MODE);
    gui_img_set_opacity(img, 122);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/delete_icon.bin",
                                 screen_size / 6, screen_size * 2 / 3, 0, 0);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_delete_icon_detail,
                         GUI_EVENT_TOUCH_CLICKED, NULL);

    img = gui_img_create_from_fs(win_del, 0, "/image/A8/back_icon.bin",
                                 screen_size * 5 / 6 - img->base.w,
                                 screen_size * 2 / 3, 0, 0);
    gui_obj_add_event_cb(img, (gui_event_cb_t)click_back_icon,
                         GUI_EVENT_TOUCH_CLICKED, NULL);

    gui_obj_focus_set(GUI_BASE(win_del));
}

/*
 * Toggle the mainface lock when the long-press progress completes.
 * Cancel the operation if the touch is released early.
 */
static void lock_icon_timer_cb(void *obj)
{
    static uint8_t count = 0;

    touch_info_t *tp = tp_get_info();
    gui_obj_t *lock = obj;
    gui_obj_t *progress_arc = gui_list_entry(lock->brother_list.next,
                                             gui_obj_t, brother_list);
    gui_obj_hidden(lock, false);
    gui_obj_hidden(progress_arc, false);

    if (enable_switch_mainface)
    {
        gui_img_set_src((gui_img_t *)lock, (const uint8_t *)"/image/lock_icon.bin",
                        IMG_SRC_FILESYS);
        gui_img_set_src((gui_img_t *)progress_arc, prog_arc_array[count], IMG_SRC_FILESYS);
    }
    else
    {
        gui_img_set_src((gui_img_t *)lock, (const uint8_t *)"/image/unlock_icon.bin",
                        IMG_SRC_FILESYS);
        gui_img_set_src((gui_img_t *)progress_arc, prog_arc_array[19 - count], IMG_SRC_FILESYS);
    }

    count++;
    if (count < 19)
    {
        if (!tp->pressing)
        {
            gui_obj_hidden(lock, true);
            gui_obj_hidden(progress_arc, true);
            count = 0;
            gui_obj_delete_timer(lock);
        }
    }
    else if (count >= 19)
    {
        enable_switch_mainface = !enable_switch_mainface;
        gui_msg_t msg =
        {
            .event = GUI_EVENT_USER_DEFINE,
            .cb = msg_2_regenerate_view,
        };
        gui_send_msg_to_server(&msg);
        count = 0;
    }
}

/*
 * Resolve the mainface index from the target view slot, then create the
 * content associated with that mainface type.
 */
static void switch_mainface(gui_obj_t *parent)
{
    int8_t diff = view_idx_next - view_idx_curr;
    uint8_t idx = mainface_idx;
    if (diff != 0)
    {
        int8_t temp = mainface_idx;
        switch (diff)
        {
        case -1:
        case 2:
            temp--;
            break;
        case 1:
            temp++;
            break;
        case -2:
            temp++;
            break;
        default:
            break;
        }
        temp += mainface_num;
        temp %= mainface_num;
        idx = temp;
    }

    gui_win_t *win = gui_win_create(parent, 0, 0, 0, 0, 0);
    win->base.user_data = &mainface_list[idx];
    gui_obj_create_timer((void *)win, 20, true, win_timer_0_cb);
    gui_obj_start_timer((void *)win);

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
        // gui_view_set_bg_color((gui_view_t *)parent, gui_rgb(0x41, 0xAD, 0x41));
        fox_3d = l3_create_model_fs((void *)mainface_list[idx].data, L3_DRAW_FRONT_AND_SORT, 0, 0, screen_size, screen_size);

        l3_set_global_transform(fox_3d, (l3_global_transform_cb)fox_global_cb);
        l3_gltf_set_active_animation(fox_3d, cur_anim);

        gui_lite3d_t *lite3d_fox = gui_lite3d_create(win, "lite3d-widget",
                                                    fox_3d, 0, 0, 0, 0);

        gui_obj_create_timer(GUI_BASE(lite3d_fox), 20, true, fox_rotate_animation);

        gui_lite3d_on_click(lite3d_fox, fox_animation_update_cb, NULL);
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
        img_0->need_clip = false;
        // {
        //     const void *src_data = gui_img_get_image_data(img_0);
        //     uint32_t src_color = get_img_color((uint8_t *)src_data);
        //     gui_log("bg color = 0x%x\n", src_color);
        //     gui_color_t bg_color;
        //     bg_color.color.rgba.a = UINT8_MAX;
        //     if (((gui_rgb_data_head_t *)src_data)->type == RGB565)
        //     {
        //         bg_color.color.rgba.r = src_color >> 8 & 0xFF;
        //         bg_color.color.rgba.g = src_color >> 3 & 0xFF;
        //         bg_color.color.rgba.b = src_color << 3 & 0xFF;
        //     }
        //     else
        //     {
        //         bg_color.color.rgba.r = src_color >> 16 & 0xFF;
        //         bg_color.color.rgba.g = src_color >> 8 & 0xFF;
        //         bg_color.color.rgba.b = src_color & 0xFF;
        //     }
        //     gui_view_set_bg_color((gui_view_t *)parent, bg_color);
        // }
        break;
    }

    default:
        break;
    }
    gui_color_t bg_color;
    bg_color.color.argb_full = mainface_list[idx].color;
    gui_view_set_bg_color((gui_view_t *)parent, bg_color);

    /* lock icon & prog_arc */
    {
        int16_t img_size = 180;
        int16_t pos = (screen_size - img_size) / 2;
        gui_img_t *img = gui_img_create_from_fs(win, 0, "/image/lock_icon.bin", pos, pos, 0, 0);
        gui_obj_hidden((gui_obj_t *)img, true);
        img = gui_img_create_from_fs(win, 0, prog_arc_array[0], pos, pos, 0, 0);
        gui_obj_hidden((gui_obj_t *)img, true);
    }

    if (dev_mode != MODE_DELETE && !enable_switch_mainface) return;
    if (dev_mode == MODE_DELETE)
    {
        create_win_del();
    }
    else
    {
        gui_view_switch_on_event((void *)parent, "top_view", SWITCH_INIT_STATE, SWITCH_IN_FROM_TOP_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_DOWN);
        if (mainface_num > 0)
        {
            gui_view_switch_on_event((void *)parent, "view_mainface_list", SWITCH_INIT_STATE, SWITCH_IN_FROM_BOTTOM_USE_TRANSLATION, GUI_EVENT_TOUCH_MOVE_UP);
        }
    }
    gui_view_switch_on_event((void *)parent, view_array[(view_idx_next + 1) % 3], SWITCH_OUT_TO_LEFT_USE_TRANSLATION, SWITCH_IN_FROM_RIGHT_USE_TRANSLATION, event_code_l);
    gui_view_switch_on_event((void *)parent, view_array[(view_idx_next - 1 + 3) % 3], SWITCH_OUT_TO_RIGHT_USE_TRANSLATION, SWITCH_IN_FROM_LEFT_USE_TRANSLATION, event_code_r);
}

#ifndef _HONEYGUI_SIMULATOR_
/* Record the final state of a BLE resource transfer. */
static void done_cb(T_XFER_CLIENT_RESULT result, uint32_t bytes_sent)
{
    printf("done_cb %d, sent %d\n", result, bytes_sent);
    if (result == XFER_CLIENT_OK)
    {
        share_file_status = SHARE_DONE;
    }
    else if (result == XFER_CLIENT_ERR_LINK)
    {
        share_file_status = SHARE_FAIL;
        is_link_error = true;
    }
    else
    {
        share_file_status = SHARE_FAIL;
    }
}
#endif

static void mainface_list_delete(void *obj)
{
    if (mainface_num == 0) return;

    void *addr_del = mainface_list[mainface_idx].data;
    for (uint8_t i = mainface_idx; i < mainface_num - 1; i++)
    {
        memcpy(&mainface_list[i], &mainface_list[i + 1], sizeof(mainface_src_t));
    }
    mainface_num--;
    if (mainface_num != 0)
    {
        if (mainface_idx != 0) mainface_idx--;
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
    view_idx_curr = 0;
    view_idx_next = 0;
    gui_obj_child_free(GUI_BASE(win_view));
    gui_view_create(GUI_BASE(win_view), view_array[view_idx_curr], 0, 0, 0, 0);
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
    PACKET_HEADER_T *header = (PACKET_HEADER_T *)(uintptr_t)info[0];


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
    view_idx_curr = 0;
    view_idx_next = 0;
    gui_obj_child_free(GUI_BASE(win_view));
    gui_view_create(GUI_BASE(win_view), view_array[view_idx_curr], 0, 0, 0, 0);
}

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

static float color_srgb_to_linear(float value)
{
    value /= 255.0f;
    return value <= 0.04045f ? value / 12.92f :
           powf((value + 0.055f) / 1.055f, 2.4f);
}

static float color_linear_to_srgb(float value)
{
    value = value <= 0.0031308f ? value * 12.92f :
            1.055f * powf(value, 1.0f / 2.4f) - 0.055f;
    return color_clamp_byte(value * 255.0f);
}

static float color_apply_easing(float t)
{
    if (t < 0.5f)
    {
        return 4.0f * t * t * t;
    }
    t = -2.0f * t + 2.0f;
    return 1.0f - t * t * t / 2.0f;
}

static gui_color_t color_interpolate(gui_color_t origin, gui_color_t target, float t)
{
    color_rgb_f_t a = {origin.color.rgba.r, origin.color.rgba.g, origin.color.rgba.b};
    color_rgb_f_t b = {target.color.rgba.r, target.color.rgba.g, target.color.rgba.b};
    color_rgb_f_t result;

    result.r = color_linear_to_srgb(color_mix(color_srgb_to_linear(a.r),
                                               color_srgb_to_linear(b.r), t));
    result.g = color_linear_to_srgb(color_mix(color_srgb_to_linear(a.g),
                                               color_srgb_to_linear(b.g), t));
    result.b = color_linear_to_srgb(color_mix(color_srgb_to_linear(a.b),
                                               color_srgb_to_linear(b.b), t));

    gui_color_t color;
    color.color.rgba.a = color_clamp_byte(color_mix(origin.color.rgba.a,
                                                    target.color.rgba.a, t));
    color.color.rgba.r = color_clamp_byte(result.r);
    color.color.rgba.g = color_clamp_byte(result.g);
    color.color.rgba.b = color_clamp_byte(result.b);
    return color;
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
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;

    if (offset % (screen_size / 2) == 0)
    {
        if (list_moved)
        {
            list_moved = false;
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
        list_moved = true;
    }
}

static void img_zoom_timer_cb(void *param)
{
    gui_obj_t *obj = (gui_obj_t *)param;
    static uint8_t cnt = 0;
    const uint8_t cnt_max = 15;

    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    uint16_t pic_size = 100;

    cnt++;
    uint16_t y_delt = (screen_size - pic_size) / 4 / cnt_max;
    float zoom = (float)screen_size / (float)pic_size * cnt / (cnt_max + 2);
    gui_obj_move(obj, obj->x, obj->y + y_delt);
    gui_img_scale((void *)obj, zoom, zoom);
    if (cnt >= cnt_max)
    {
        cnt = 0;
        gui_obj_stop_timer(obj);
        gui_view_switch_direct(gui_view_get_current(), view_array[view_idx_curr], SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
    }
}

static void prog_arc_timer(void *param)
{
    gui_obj_t *obj = (gui_obj_t *)param;
    gui_arc_t *arc = (gui_arc_t *)gui_list_entry(obj->child_list.next, gui_obj_t, brother_list);
#ifdef _HONEYGUI_SIMULATOR_
    static float angle = 0.f;
    angle += 40.f;
    if (angle >= 360.f)
    {
        angle = 0.f;
        share_file_status = SHARE_DONE;
    }
#else
    float angle = 0.f;
    uint32_t bytes_sent = 0, total = 0;
    T_XFER_CLIENT_PHASE phase = 0;
    hmi_ble_central_get_send_progress(&bytes_sent, &total, &phase);
    angle = (float)bytes_sent / (float)total * 360.f;
    angle = angle > 1.0f ? angle : 1.0f;
    gui_log("bytes_sent: %d, total: %d, angle: %f\n", bytes_sent, total, angle);
#endif
    switch (share_file_status)
    {
    case SHARE_ING:
        gui_arc_set_end_angle(arc, angle - 90.f);
        break;
    case SHARE_DONE:
    {
        gui_img_set_src((void *)obj, "/image/send_done_icon.bin", IMG_SRC_FILESYS);
        break;
    }
    case SHARE_FAIL:
    {
        gui_img_set_src((void *)obj, "/image/send_fail_icon.bin", IMG_SRC_FILESYS);
        break;
    }
    default:
        break;
    }
    if (share_file_status == SHARE_DONE || share_file_status == SHARE_FAIL)
    {
        gui_obj_delete_timer(obj);
        gui_obj_child_free(obj);
        gui_fb_change();
    }
}

static void click_2_mainface_view(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    if (list_moved) return;
    gui_obj_t *parent = obj;
    gui_obj_t *list= (void *)lst_mainface;
    gui_obj_t *send_icon = gui_list_entry(list->brother_list.next, gui_obj_t, brother_list);
    gui_obj_t *img = gui_list_entry(parent->child_list.prev, gui_obj_t, brother_list);
    if (mainface_num > 1)
    {
        gui_obj_t *note_first = gui_list_entry(list->child_list.next, gui_obj_t, brother_list);
        gui_obj_t *note_center = gui_list_entry(note_first->brother_list.next, gui_obj_t, brother_list);
        gui_obj_t *note_last = gui_list_entry(list->child_list.prev, gui_obj_t, brother_list);
        img = gui_list_entry(note_center->child_list.next, gui_obj_t, brother_list);
        gui_list_enable_scroll((void *)list, false);
        gui_obj_hidden(note_first, true);
        gui_obj_hidden(note_last, true);
    }
    gui_img_set_focus((void *)img, img->w / 2, img->h / 2);
    gui_img_translate((void *)img, img->w / 2, img->h / 2);
    gui_obj_create_timer(img, 10, true, img_zoom_timer_cb);
    gui_obj_hidden(send_icon, true);

    mainface_idx = list_index;
}

static void click_button_2_share(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    switch (share_file_status)
    {
    case SHARE_DEFAULT:
    {
        gui_arc_create(obj, 0, 50, 50, 42, -90.f, -89.f, 6, gui_rgb(0xff, 0xff, 0xff));
        gui_obj_create_timer(obj, 500, true, prog_arc_timer);
        share_file_status = SHARE_ING;
#ifndef _HONEYGUI_SIMULATOR_
        /* B: gate the send on the central link being READY (connected + HMI
         * service discovered + notify enabled).  hmi_ble_central_connect()
         * returning true only meant "connecting"; a send before READY is
         * rejected by hmi_ble_central_send_file() and the progress arc would
         * otherwise freeze at 0 (bytes/total = 0/0). */
        extern bool hmi_ble_central_is_ready(void);
        if (!hmi_ble_central_is_ready())
        {
            gui_log("share: central link not READY, refuse send\n");
            share_file_status = SHARE_FAIL;
            is_link_error = true;
            return;
        }
        gui_log("\nResource %d 0x%x \n", list_index, (unsigned int)(uint32_t)mainface_list[list_index].raw);
        if ((uint32_t)mainface_list[list_index].raw < USER_RESOURCE_ADDR || (uint32_t)mainface_list[list_index].raw >= USER_RESOURCE_ADDR_END)
        {
            gui_log("\nResource is not in user resource area, cannot share!\n");
            return;
        }

        uint32_t addr = (uint32_t)mainface_list[list_index].raw;
        uint32_t len = RES_SIZE(addr);

        /* The completion callback reports the final result and byte count. */
        int res = hmi_ble_central_send_file(HMI_L2_XFER_TYPE_IMAGE,
                                            (const uint8_t *)addr, len,
                                            "share_0", done_cb);
        gui_log("Sending file: %p, size: %d, result: %d\n", (void *)addr, len, res);
        if (!res)
        {
            /* Rejected at the stack (e.g. link dropped between the READY check
             * and here) -- surface FAIL instead of a progress arc frozen at 0. */
            gui_log("share: send_file rejected -> mark FAIL\n");
            share_file_status = SHARE_FAIL;
            is_link_error = true;
        }
#endif
        break;
    }
    case SHARE_ING:
        break;
    case SHARE_DONE:
    case SHARE_FAIL:
    {
        share_file_status = SHARE_DEFAULT;
        if (is_link_error)
        {
            is_link_error = false;
            dev_mode = MODE_DEFAULT;
            msg_2_regenerate_view(NULL);
        }
        else
        {
            gui_img_set_src(obj, "/image/dev_send_icon.bin", IMG_SRC_FILESYS);
            gui_fb_change();
        }
        break;
    }

    default:
        break;
    }
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

    if (share_file_status == SHARE_ING) return;

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
        share_file_status = SHARE_DEFAULT;
        gui_view_t *view_current = gui_view_get_current();
        gui_obj_t *dev_send = gui_list_entry(view_current->base.child_list.prev, gui_obj_t, brother_list);
        gui_obj_t *dev_disconn = gui_list_entry(dev_send->brother_list.prev, gui_obj_t, brother_list);

        gui_obj_tree_free(dev_send);
        gui_dispdev_t *dc = gui_get_dc();
        uint16_t screen_size = dc->screen_width;
        uint16_t pic_size = 100;
        gui_obj_move(dev_disconn, (screen_size - pic_size) / 2, screen_size * 2/3);
        gui_img_set_src((void *)dev_disconn, "/image/dev_send_icon.bin", IMG_SRC_FILESYS);
        dev_disconn->event_dsc[0].event_cb = click_button_2_connect;
        gui_obj_add_event_cb(view_current, (gui_event_cb_t)click_2_mainface_view, GUI_EVENT_TOUCH_CLICKED, NULL);
        gui_view_switch_on_event(view_current, view_array[view_idx_curr], SWITCH_OUT_TO_BOTTOM_USE_TRANSLATION, SWITCH_INIT_STATE, GUI_EVENT_TOUCH_MOVE_DOWN);
    }
}

static void mainface_list_view_timer_cb(void *obj)
{
    GUI_UNUSED(obj);
    is_displaying_mainface = false;
}

/*============================================================================*
 * CMD 0x0F remote-control callbacks (spec v2 section 2, principle 2)
 *----------------------------------------------------------------------------*
 * State parsing itself lives in app/l2_handlers/hmi_l2_cmd_remote.c
 * (parse_state_report -- the ONLY writer of the authoritative state mirror).
 * UI-side wiring is just: register a callback, pull latest via getters, redraw.
 *
 * BEWARE: these callbacks run on the BLE / L2 RX thread, NOT on the GUI thread.
 * Do NOT touch widgets directly here -- either only log (P0 today), or post a
 * message to the GUI thread when you add zoom / recording indicator controls.
 *============================================================================*/
static void on_remote_state_changed(void)
{
#ifdef _HONEYGUI_SIMULATOR_
    gui_log("on_remote_state_changed");
#else
    gui_log("remote: STATE zoom=%u.%02ux rec=%d facing=%s shot(has=%d id=%u)\n",
            (unsigned)(hmi_l2_remote_state_zoom_x100() / 100u),
            (unsigned)(hmi_l2_remote_state_zoom_x100() % 100u),
            (int)hmi_l2_remote_state_recording(),
            hmi_l2_remote_state_facing() ? "front" : "back",
            (int)hmi_l2_remote_state_has_last_shot(),
            (unsigned)hmi_l2_remote_state_last_shot_id());
    /* TODO(ui): once zoom label / rec-dot widgets exist, post a redraw msg
     * to the GUI thread from here (do NOT call widget APIs directly). */

    ui_remote_change(hmi_l2_remote_state_zoom_x100());
#endif
}

static void on_remote_shot_ready(uint16_t shot_id)
{
    gui_log("remote: LAST_SHOT_READY id=%u\n", (unsigned)shot_id);
    /* TODO(ui): pop a "new photo" hint / trigger thumbnail refresh. */
}

static void on_remote_ctrl_result(uint8_t key, uint8_t code)
{
    /* App rejected one of our requests (success is signaled implicitly through
     * STATE_REPORT -- see spec section 5.2). */
    gui_log("remote: CTRL_RESULT key=0x%02x code=0x%02x (%s)\n",
            (unsigned)key, (unsigned)code,
            code == 0x01 ? "unsupported" :
            code == 0x02 ? "busy" :
            code == 0x03 ? "out_of_range" :
            code == 0x04 ? "not_ready" : "unknown");
    /* TODO(ui): toast the error to the user. */
}

/*============================================================================*
 * Public callbacks and APIs
 *============================================================================*/

/* Update the flashlight view background from the selected color index. */
void set_flashlight_color(void *obj)
{
    static const uint8_t color_table[][3] =
    {
        {255, 255, 255},
        {255,   0,   0},
        {255, 165,   0},
        {255, 255,   0},
        {  0, 255,   0},
        {  0, 127, 255},
        {  0,   0, 255},
        {139,   0, 255},
    };
    uint8_t color_idx = (uint8_t)fl_color_idx;

    if (color_idx >= sizeof(color_table) / sizeof(color_table[0]))
    {
        color_idx = 0;
    }

    gui_color_t color = gui_rgb(color_table[color_idx][0],
                                color_table[color_idx][1],
                                color_table[color_idx][2]);
    gui_view_set_bg_color(obj, color);
    gui_fb_change();

    /* TODO: Synchronize the display backlight level. */
}



/*
 * Mainface periodic task: update the scrolling image position and process
 * the long-press lock gesture.
 */
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
            gui_obj_create_timer(lock, 50, true, lock_icon_timer_cb);
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
}

void click_delete_icon(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    if (mainface_num == 0) return;

    dev_mode = MODE_DELETE;
    gui_view_switch_direct(gui_view_get_current(), view_array[view_idx_curr], SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    // TODO
#endif

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


void click_camera_shutter(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    gui_log("click shutter\n");
#ifndef _HONEYGUI_SIMULATOR_
    /* CMD 0x0F CAPTURE (spec v2 5.1).  Do not touch local UI here -- wait for
     * app's LAST_SHOT_READY / STATE_REPORT (spec section 2, principle 1). */
    hmi_l2_remote_send_capture();
#endif
}
void click_camera_1x(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    gui_log("click 1x\n");
    void *img_src = "/image/stream/1x_hl.bin";
    gui_img_set_src(img_1x, img_src, IMG_SRC_FILESYS);
    img_src = "/image/stream/2x_df.bin";
    gui_img_set_src(img_2x, img_src, IMG_SRC_FILESYS);

#ifndef _HONEYGUI_SIMULATOR_
    hmi_l2_remote_send_set_zoom(100u);   /* 1.0x */
#endif
}
void click_camera_2x(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    gui_log("click 2x\n");
    void *img_src = "/image/stream/2x_hl.bin";
    gui_img_set_src(img_2x, img_src, IMG_SRC_FILESYS);
    img_src = "/image/stream/1x_df.bin";
    gui_img_set_src(img_1x, img_src, IMG_SRC_FILESYS);

#ifndef _HONEYGUI_SIMULATOR_
    hmi_l2_remote_send_set_zoom(200u);   /* 2.0x */
#endif
}


uint8_t mainface_list_init(void **data_list, uint32_t n)
{
    uint8_t idx = 0;
    uint8_t reserved = 6;
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
        gui_log("list init %d, 0x%x  type %d, raw 0x%x, color 0x%x", idx, mainface_list[list_idx].data, mainface_list[list_idx].type, mainface_list[list_idx].raw, mainface_list[list_idx].color);
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
    case CONNECT_DEV:
        is_dev_connect = true;
        break;
    case DISCONNECT_DEV:
    {
        is_dev_connect = false;
        dev_mode = MODE_DEFAULT;
        msg_2_regenerate_view(NULL);
        break;
    }
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
        if (!is_displaying_mainface || mainface_num <= 1) return;
        mainface_idx = (mainface_idx - 1 + mainface_num) % mainface_num;
        msg_2_regenerate_view(NULL);
        break;
    }
    case SWITCH_RIGHT_MAINFACE:
    {
        if (!is_displaying_mainface || mainface_num <= 1) return;
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
    case REMOTE_CHANGE:
    {
        gui_log("remote change zoom %d\n", msg->payload);
        uint16_t zoom_x100 = (uint16_t)(uintptr_t)msg->payload;
        if(zoom_x100 == 100)
        {
            void *img_src = "/image/stream/1x_hl.bin";
            gui_img_set_src(img_1x, img_src, IMG_SRC_FILESYS);
            img_src = "/image/stream/2x_df.bin";
            gui_img_set_src(img_2x, img_src, IMG_SRC_FILESYS);
        }
        else if(zoom_x100 == 200)
        {
            void *img_src = "/image/stream/2x_hl.bin";
            gui_img_set_src(img_2x, img_src, IMG_SRC_FILESYS);
            img_src = "/image/stream/1x_df.bin";
            gui_img_set_src(img_1x, img_src, IMG_SRC_FILESYS);
        }
        else
        {
            void *img_src = "/image/stream/1x_df.bin";
            gui_img_set_src(img_1x, img_src, IMG_SRC_FILESYS);
            img_src = "/image/stream/2x_df.bin";
            gui_img_set_src(img_2x, img_src, IMG_SRC_FILESYS);
        }

        break;
    }

    default:
        break;
    }
}


void ui_add_resource(uint32_t payload)
{
    gui_msg_t msg = {.event = GUI_EVENT_USER_DEFINE, .sub_event = ADD_MAINFACE, .cb = (gui_msg_cb)ui_process_msg, .payload = (void *)(uintptr_t)payload};
    gui_send_msg_to_server(&msg);
}
void ui_jump_streaming(void)
{
    gui_view_t *view_c = gui_view_get_current();
    if (view_c && (strcmp(view_c->base.name, "view_cam_ctl") == 0)) return;
    gui_msg_t msg = {.event = GUI_EVENT_USER_DEFINE, .sub_event = CAST_START, .cb = (gui_msg_cb)ui_process_msg};
    gui_send_msg_to_server(&msg);
}
void ui_remote_change(uint32_t payload)
{
    gui_msg_t msg = {.event = GUI_EVENT_USER_DEFINE, .sub_event = REMOTE_CHANGE, .cb = (gui_msg_cb)ui_process_msg, .payload = (void *)(uintptr_t)payload};
    gui_send_msg_to_server(&msg);
}

void switch_in_mainface_0(gui_view_t *view)
{
    GUI_UNUSED(view);
    view_idx_next = 0;
    switch_mainface(GUI_BASE(view));
}

void switch_in_mainface_1(gui_view_t *view)
{
    GUI_UNUSED(view);
    view_idx_next = 1;
    switch_mainface(GUI_BASE(view));
}

void switch_in_mainface_2(gui_view_t *view)
{
    GUI_UNUSED(view);
    view_idx_next = 2;
    switch_mainface(GUI_BASE(view));
}

void switch_out_mainface(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (strcmp(view->base.name, view_array[view_idx_curr]) == 0 &&
        gui_view_get_current() != NULL)
    {
        int8_t diff = view_idx_next - view_idx_curr;
        if (diff != 0)
        {
            int8_t temp = mainface_idx;
            switch (diff)
            {
            case -1:
            case 2:
                temp--;
                break;
            case 1:
            case -2:
                temp++;
                break;
            default:
                break;
            }
            temp += mainface_num;
            temp %= mainface_num;
            mainface_idx = temp;
            view_idx_curr = view_idx_next;
        }
    }
}

void switch_in_top_view(gui_view_t *view)
{
    gui_view_switch_on_event(view, view_array[view_idx_curr], SWITCH_OUT_TO_TOP_USE_TRANSLATION, SWITCH_INIT_STATE, GUI_EVENT_TOUCH_MOVE_UP);
}
void switch_in_view_cam_ctl(gui_view_t *view)
{
    gui_view_switch_on_event(view, view_array[view_idx_curr], SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION, GUI_EVENT_TOUCH_LEFT_SLIDE_QUICK);
    gui_view_switch_on_event(view, view_array[view_idx_curr], SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_NONE_ANIMATION, GUI_EVENT_TOUCH_RIGHT_SLIDE_QUICK);
}







void lst_mainface_note_design(gui_obj_t *obj, void *param)
{
    GUI_UNUSED(param);

    // Cast obj to gui_list_note_t * type
    gui_list_note_t *note = (gui_list_note_t *)obj;
    int16_t index = note->index % mainface_num;
    index += mainface_num;
    index %= mainface_num;
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    uint16_t pic_size = 160;
    uint16_t img_y = (screen_size - pic_size) / 4;

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









void switch_in_mainface_list(gui_view_t *view)
{
    list_index = mainface_idx;
    gui_obj_create_timer((void *)view, 10, true, mainface_list_view_timer_cb);
    gui_dispdev_t *dc = gui_get_dc();
    uint16_t screen_size = dc->screen_width;
    uint16_t pic_size = 100;
    /* Device key */
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
        gui_view_switch_on_event(view, view_array[view_idx_curr], SWITCH_OUT_TO_BOTTOM_USE_TRANSLATION, SWITCH_INIT_STATE, GUI_EVENT_TOUCH_MOVE_DOWN);
    }

    pic_size = 160;
    if (mainface_num == 1)
    {
        uint16_t pic_size = 160;
        uint16_t img_x = (screen_size - pic_size) / 2;
        uint16_t img_y = (screen_size - pic_size) / 4;

    #ifdef _HONEYGUI_SIMULATOR_
        gui_img_t *img = gui_img_create_from_fs(view, 0, mainface_list[mainface_idx].img_preview, img_x, img_y, 0, 0);
    #else
        gui_img_t *img = NULL;
        if (((uint32_t)mainface_list[mainface_idx].data) >= USER_RESOURCE_ADDR && ((uint32_t)mainface_list[mainface_idx].data) < (USER_RESOURCE_ADDR_END))
        {
            img = gui_img_create_from_mem(view, 0, mainface_list[mainface_idx].img_preview, img_x, img_y, 0, 0);
        }
        else
        {
            img = gui_img_create_from_fs(view, 0, mainface_list[mainface_idx].img_preview, img_x, img_y, 0, 0);
        }
    #endif
        gui_img_set_mode(img, IMG_SRC_OVER_MODE);
        gui_list_set_note_num(lst_mainface, 0);
    }
    else
    {
        lst_mainface->base.w = 520;
        gui_list_set_note_num(lst_mainface, mainface_num * 2);
        gui_list_set_offset(lst_mainface, (screen_size / 2) * (1 - list_index));

        gui_obj_create_timer((void *)lst_mainface, 10, true, list_timer_cb);
        gui_obj_start_timer((void *)lst_mainface);
    }

    gui_color_t bg_color;
    bg_color.color.argb_full = mainface_list[list_index].color;
    gui_view_set_bg_color(view, bg_color);
}

/*============================================================================*
 * Live-video stream
 *
 * A single transport is shared by the BLE producer and GUI stream consumer.
 * MSV1 frames are consumed oldest-first and are never dropped.
 *============================================================================*/
#define APP_STREAM_MAX_FRAME   (35u * 1024u)
#define APP_STREAM_BUF_COUNT   32u

static const stp_class_cfg_t s_stream_classes[] =
{
    { .buf_size = APP_STREAM_MAX_FRAME, .buf_count = APP_STREAM_BUF_COUNT },
};
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
    cfg.drop_mode          = STP_DROP_UNCONDITIONAL;   /* MSV1: oldest-first, never drop */
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




int app_remote_ctrl_init(void)
{
#ifdef _HONEYGUI_SIMULATOR_
    gui_log("app_remote_ctrl_init\n");
#else
    hmi_l2_remote_set_state_cb(on_remote_state_changed);
    hmi_l2_remote_set_shot_cb(on_remote_shot_ready);
    hmi_l2_remote_set_ctrl_result_cb(on_remote_ctrl_result);
#endif
    gui_log("app_remote: callbacks registered (CMD 0x0F)\n");
    return 0;
}
