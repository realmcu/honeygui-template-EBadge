#include "easy_demoMain_user.h"
#include "tp_algo.h"
#include "gui_message.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* ----------------------------------------------------*/
#ifdef _HONEYGUI_SIMULATOR_

#else
#include "flashdb.h"
#define USER_RESOURCE_ADDR     (0x0240f400u +   0x700000u)
#define USER_RESOURCE_ADDR_END (0x0240f400u + 0x00998000) // 0x00998000  //9824K Bytes

#endif

gui_win_t *win_view = NULL;

uint8_t mainface_idx = 0;
uint8_t mainface_num = 6;
mainface_src_t mainface_list[MAINFACE_NUM_MAX] =
{
    {"/image/565/wallpaper_6.bin", SRC_IMG},
    {"/wallpaper_1.avi", SRC_VIDEO},
    {"/image/565/wallpaper_4.bin", SRC_IMG},
    {"/wallpaper_2.avi", SRC_VIDEO},
    {"/image/565/wallpaper_5.bin", SRC_IMG},
    {"/wallpaper_3.avi", SRC_VIDEO},
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

void *danmu_bg[MAINFACE_NUM_MAX] = {0};

/* ---------------FUNC---------------- */
typedef enum
{
    PURE_PF_RGB565    = 0,  /* 2B: bit[15:11]R bit[10:5]G bit[4:0]B, little-endian      */
    PURE_PF_ARGB8565  = 1,  /* 3B: RGB565(2B, little-endian) + Alpha(1B)                */
    PURE_PF_RGB888    = 3,  /* 3B: byte order B,G,R                                     */
    PURE_PF_ARGB8888  = 4,  /* 4B: byte order B,G,R,A                                  */
    PURE_PF_XRGB8888  = 5,  /* 4B: byte order B,G,R,X(fill 0)                          */
    PURE_PF_A8        = 9,  /* 1B: Alpha only (upper 8 bits of color)                   */
} pure_pixel_format_t;

/* RLE compression algorithm constants (compress/rle.ts: COMPRESS_RLE = 0) */
#define PURE_RLE_ALGORITHM   0
#define PURE_RLE_FEATURE_1   1   /* RLECompression default runLength1 = 1 */
#define PURE_RLE_FEATURE_2   0   /* default runLength2 = 0 */
#define PURE_RLE_MAX_RUN     255 /* max repeat count per RLE node */

/* gui_rgb_data_head_t flags byte: set compress bit only (bit4) */
#define PURE_RLE_HEAD_FLAGS  0x10

static void pure_wr_u16le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static void pure_wr_u32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

/* Bytes per pixel (length of the pixel part in one RLE node). Returns 0 for unsupported formats. */
static int pure_rle_bpp(uint8_t fmt)
{
    switch (fmt)
    {
    case PURE_PF_RGB565:   return 2;
    case PURE_PF_ARGB8565: return 3;
    case PURE_PF_RGB888:   return 3;
    case PURE_PF_ARGB8888: return 4;
    default:               return 0;
    }
}

/* imdc_file_header_t pixel_bytes field encoding (PixelBytes enum):
 *   BYTES_2=0, BYTES_3=1, BYTES_4=2, BYTES_1=3 */
static uint8_t pure_rle_pixel_bytes_code(uint8_t fmt)
{
    switch (pure_rle_bpp(fmt))
    {
    case 2:  return 0; /* BYTES_2 */
    case 3:  return 1; /* BYTES_3 */
    case 4:  return 2; /* BYTES_4 */
    case 1:  return 3; /* BYTES_1 */
    default: return 0;
    }
}

/* Pack 0xAARRGGBB color into out using the target format; returns bytes written (=bpp), or 0 if unsupported. */
static int pure_rle_pack_pixel(uint32_t argb, uint8_t fmt, uint8_t *out)
{
    switch (fmt)
    {
    case PURE_PF_RGB565:
        pure_wr_u16le(out, argb);
        return 2;

    case PURE_PF_ARGB8565:
        pure_wr_u16le(out, argb);
        out[2] = argb >> 16;
        return 3;

    case PURE_PF_RGB888:        /* BGR */
    {
        uint8_t r = (uint8_t)((argb >> 16) & 0xFF);
        uint8_t g = (uint8_t)((argb >> 8) & 0xFF);
        uint8_t b = (uint8_t)(argb & 0xFF);
        out[0] = b; out[1] = g; out[2] = r;
        return 3;
    }
        
    case PURE_PF_ARGB8888:      /* BGRA */
    {
        uint8_t a = (uint8_t)((argb >> 24) & 0xFF);
        uint8_t r = (uint8_t)((argb >> 16) & 0xFF);
        uint8_t g = (uint8_t)((argb >> 8) & 0xFF);
        uint8_t b = (uint8_t)(argb & 0xFF);
        out[0] = b; out[1] = g; out[2] = r; out[3] = a;
        return 4;
    }

    default:
        return 0;
    }
}

/*
 * Calculate the total byte size needed for a solid-color RLE image (for caller pre-allocation).
 * Returns 0 if parameters are invalid (unsupported format / zero width or height).
 */
static size_t gui_pure_rle_size(uint8_t fmt, uint16_t width, uint16_t height)
{
    int bpp = pure_rle_bpp(fmt);
    if (bpp == 0 || width == 0 || height == 0)
    {
        return 0;
    }

    /* width identical pixels per row -> ceil(width/255) RLE nodes */
    size_t nodes_per_line = ((size_t)width + (PURE_RLE_MAX_RUN - 1)) / PURE_RLE_MAX_RUN;
    size_t bytes_per_line = nodes_per_line * (size_t)(1 + bpp);

    size_t header_bytes = 8                                   /* gui_rgb_data_head_t */
                          + 12                                /* imdc_file_header_t  */
                          + (size_t)(height + 1) * 4;         /* row offset table    */

    return header_bytes + bytes_per_line * (size_t)height;
}

/*
 * Generate binary data for a solid-color RLE image.
 *
 *   argb      Input color in 0xAARRGGBB format (A ignored for non-alpha formats; A8 uses A only)
 *   fmt       Image pixel format, see pure_pixel_format_t
 *   width     Image width in pixels
 *   height    Image height in pixels
 *   out_buf   Output buffer pointer (“image pointer”)
 *   buf_size  Output buffer capacity in bytes, used for bounds protection
 *
 * Returns the number of bytes actually written; returns 0 if parameters are invalid or buffer is too small.
 */
static void *gui_pure_rle_create(uint32_t argb, uint8_t fmt,
                           uint16_t width, uint16_t height)
{
    void *buffer = NULL;
    int bpp = pure_rle_bpp(fmt);
    if (bpp == 0 || width == 0 || height == 0)
    {
        return buffer;
    }

    size_t total = gui_pure_rle_size(fmt, width, height);
    if (total == 0)
    {
        return buffer;
    }
    buffer = gui_malloc(total);
    if (buffer == NULL)
    {
        return buffer;
    }
    uint8_t *p = buffer;

    /* ---- 1. gui_rgb_data_head_t (8B) ---- */
    p[0] = PURE_RLE_HEAD_FLAGS;     /* flags: compress=1 */
    p[1] = fmt;                     /* type               */
    pure_wr_u16le(p + 2, width);    /* width  (int16 LE)  */
    pure_wr_u16le(p + 4, height);   /* height (int16 LE)  */
    p[6] = 0;                       /* version            */
    p[7] = 0;                       /* rsvd2              */
    p += 8;

    /* ---- 2. imdc_file_header_t (12B) ---- */
    p[0] = (uint8_t)((PURE_RLE_ALGORITHM & 0x03)
                     | ((PURE_RLE_FEATURE_1 & 0x03) << 2)
                     | ((PURE_RLE_FEATURE_2 & 0x03) << 4)
                     | ((pure_rle_pixel_bytes_code(fmt) & 0x03) << 6));
    p[1] = 0;                       /* reserved[0] */
    p[2] = 0;                       /* reserved[1] */
    p[3] = 0;                       /* reserved[2] */
    pure_wr_u32le(p + 4, width);    /* raw_pic_width  */
    pure_wr_u32le(p + 8, height);   /* raw_pic_height */
    p += 12;

    /* ---- 3. Row offset table (height+1) * 4B ----
     * offset base = start of imdc header (file offset 8), imdcOffset = 12 + (height+1)*4 */
    size_t nodes_per_line = ((size_t)width + (PURE_RLE_MAX_RUN - 1)) / PURE_RLE_MAX_RUN;
    size_t bytes_per_line = nodes_per_line * (size_t)(1 + bpp);
    uint32_t imdc_offset = (uint32_t)(12 + (size_t)(height + 1) * 4);

    for (uint16_t line = 0; line < height; line++)
    {
        /* All rows have the same byte count: lineOffset = line * bytes_per_line */
        pure_wr_u32le(p, imdc_offset + (uint32_t)((size_t)line * bytes_per_line));
        p += 4;
    }
    /* Last entry = imdc_offset + total compressed data length (end of file) */
    pure_wr_u32le(p, imdc_offset + (uint32_t)(bytes_per_line * (size_t)height));
    p += 4;

    /* ---- 4. RLE compressed data ---- */
    uint8_t pix[4];
    pure_rle_pack_pixel(argb, fmt, pix);     /* Pre-pack one pixel, reused for the entire image */

    for (uint16_t line = 0; line < height; line++)
    {
        uint16_t remain = width;
        while (remain > 0)
        {
            uint8_t run = (remain > PURE_RLE_MAX_RUN)
                          ? (uint8_t)PURE_RLE_MAX_RUN
                          : (uint8_t)remain;
            *p++ = run;                      /* [len] */
            memcpy(p, pix, (size_t)bpp);     /* [pixel] */
            p += bpp;
            remain = (uint16_t)(remain - run);
        }
    }

    return buffer;
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
            uint32_t color = (uint16_t)(((uint16_t *)img_0->src_data)[4]);
            gui_log("bg color = 0x%x\n", color);
            void *img_bg_data = gui_pure_rle_create(color, ((uint8_t *)img_0->src_data)[1], SCREEN_SIZE, SCREEN_SIZE);

            if (img_bg_data != NULL)
            {
                gui_img_t *img_bg = gui_img_create_from_mem(parent, 0, img_bg_data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
                gui_img_set_mode(img_bg, IMG_BYPASS_MODE);
                gui_list_insert(&GUI_BASE(img_bg)->brother_list, &GUI_BASE(win)->brother_list);
                danmu_bg[idx] = img_bg_data;
            }
        }
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
        gui_img_set_mode(img_0, IMG_BYPASS_MODE);
        if (img_0->base.w > SCREEN_SIZE)
        {
            int16_t img_y = (SCREEN_SIZE - img_0->base.h) / 2;
            gui_obj_move((void *)img_0, SCREEN_SIZE, img_y);
            gui_list_remove(&GUI_BASE(win)->brother_list);
            uint32_t color = (uint16_t)(((uint16_t *)img_0->src_data)[4]);
            gui_log("bg color = 0x%x\n", color);
            void *img_bg_data = gui_pure_rle_create(color, ((uint8_t *)img_0->src_data)[1], SCREEN_SIZE, SCREEN_SIZE);

            if (img_bg_data != NULL)
            {
                gui_img_t *img_bg = gui_img_create_from_mem(parent, 0, img_bg_data, 0, 0, SCREEN_SIZE, SCREEN_SIZE);
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
    gui_log("Add resource 0x%x\n", data);
    if(!data)
    {
        gui_log("New passed resource  is NULL!!!!!!!!!!!!!!!!!\n");
        return ;
    }

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
        
        mainface_list[idx].data = data_list[idx];
        mainface_list[idx].type = SRC_IMG;
        if (*(uint8_t *)data_list[idx] == 0x52)
        {
            mainface_list[idx].type = SRC_VIDEO;
        }
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
        if (view_cur != NULL && strcmp(view_cur->base.name, "view_cast") != 0)
        {
            view_rec = view_cur->base.name;
        }
        gui_obj_child_free(GUI_BASE(win_view));
        gui_view_create(GUI_BASE(win_view), "view_cast", 0, 0, 0, 0);
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

void switch_out_mainface_0(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (danmu_bg[0] != NULL)
    {
        gui_free(danmu_bg[0]);
        danmu_bg[0] = NULL;
    }
}

void switch_out_mainface_1(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (danmu_bg[1] != NULL)
    {
        gui_free(danmu_bg[1]);
        danmu_bg[1] = NULL;
    }
}

void switch_out_mainface_2(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (danmu_bg[2] != NULL)
    {
        gui_free(danmu_bg[2]);
        danmu_bg[2] = NULL;
    }
}

void switch_out_mainface_3(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (danmu_bg[3] != NULL)
    {
        gui_free(danmu_bg[3]);
        danmu_bg[3] = NULL;
    }
}

void switch_out_mainface_4(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (danmu_bg[4] != NULL)
    {
        gui_free(danmu_bg[4]);
        danmu_bg[4] = NULL;
    }
}

void switch_out_mainface_5(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (danmu_bg[5] != NULL)
    {
        gui_free(danmu_bg[5]);
        danmu_bg[5] = NULL;
    }
}

void switch_out_mainface_6(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (danmu_bg[6] != NULL)
    {
        gui_free(danmu_bg[6]);
        danmu_bg[6] = NULL;
    }
}

void switch_out_mainface_7(gui_view_t *view)
{
    GUI_UNUSED(view);
    if (danmu_bg[7] != NULL)
    {
        gui_free(danmu_bg[7]);
        danmu_bg[7] = NULL;
    }
}