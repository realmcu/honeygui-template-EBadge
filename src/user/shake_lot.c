/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * Shake-lot demo.
 *
 * The application uses a three-state workflow:
 * - IDLE waits for a shake with the animation stopped and result hidden.
 * - SHAKING loops the lite-video animation while movement continues.
 * - RESULT selects a weighted result and displays its image after movement stops.
 *
 * The device is held vertically, so the back-and-forth motion occurs in the
 * sensor X-Y plane. A low-pass filter estimates gravity, and the remaining
 * linear acceleration is projected onto the dominant in-plane axis.
 *
 * A leaky score counts direction reversals instead of one-way movement. This
 * avoids triggers from picking up or repositioning the device while allowing
 * repeated shaking to reach the start threshold quickly.
 */

#include "shake_lot.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "gsensor_reader.h"
#include "gui_api.h"
#include "gui_fb.h"
#include "gui_img.h"
#include "gui_lite_video.h"

/*============================================================================*
 * Resources
 *============================================================================*/

#define SL_IMG_START_PATH "/image/shake_lot/lot_start.bin"
#define SL_VIDEO_PATH     "/image/shake_lot/stick.avi"
#define SL_LOT_COUNT      7

static const char *const lot_image_paths[SL_LOT_COUNT] =
{
    "/image/shake_lot/lot_dj.bin",
    "/image/shake_lot/lot_j.bin",
    "/image/shake_lot/lot_zj.bin",
    "/image/shake_lot/lot_xj.bin",
    "/image/shake_lot/lot_mj.bin",
    "/image/shake_lot/lot_x.bin",
    "/image/shake_lot/lot_dx.bin",
};

/* Relative probability weights; the values are normalized by their sum. */
static const uint16_t lot_weights[SL_LOT_COUNT] =
{
    20, 22, 25, 18, 10, 4, 1,
};

/*============================================================================*
 * Shake-detection configuration
 *
 * Acceleration thresholds use mg. Counters use sampling ticks, where one tick
 * is SL_TICK_MS. A reversal on the dominant X-Y axis adds one point, or two
 * points for a strong reversal.
 *============================================================================*/

#define SL_TICK_MS      30  /* Sampling rate: approximately 33 Hz. */
#define SL_SHAKE_TH     180 /* Minimum acceleration for direction tracking. */
#define SL_STRONG_TH    350 /* A strong reversal adds two points. */
#define SL_QUIET_TH     90  /* Motion below this threshold is quiet. */
#define SL_START_SCORE  3   /* Reversal score required to start shaking. */
#define SL_MAX_SCORE    8   /* Maximum accumulated reversal score. */
#define SL_STOP_QUIET   8   /* Quiet ticks required to show a result. */
#define SL_WARMUP_TICKS 8   /* Filter stabilization period. */

/*============================================================================*
 * Types and state
 *============================================================================*/

typedef enum
{
    SL_IDLE = 0,
    SL_SHAKING,
    SL_RESULT,
} sl_phase_t;

typedef struct
{
    gui_img_t *start;
    gui_lite_video_t *video;
    gui_img_t *result;
    gui_obj_t *ctrl;

    sl_phase_t phase;

    bool filter_valid;
    uint8_t warmup;
    int32_t gravity_x;
    int32_t gravity_y;
    int32_t gravity_z;
    int32_t score;
    int8_t last_direction;
    uint8_t quiet_count;

    uint32_t random_state;
} sl_context_t;

/* Only one shake-lot view is active at a time. */
static sl_context_t shake_lot_context;

/*============================================================================*
 * File-local functions
 *============================================================================*/

static uint32_t random_next(sl_context_t *context)
{
    uint32_t value = context->random_state != 0u ?
                     context->random_state : 0x2545F491u;

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    context->random_state = value;

    return value;
}

static int pick_lot(sl_context_t *context)
{
    uint32_t total = 0;

    for (int i = 0; i < SL_LOT_COUNT; i++)
    {
        total += lot_weights[i];
    }
    if (total == 0u)
    {
        return 0;
    }

    uint32_t selected = random_next(context) % total;
    uint32_t accumulated = 0;

    for (int i = 0; i < SL_LOT_COUNT; i++)
    {
        accumulated += lot_weights[i];
        if (selected < accumulated)
        {
            return i;
        }
    }

    return SL_LOT_COUNT - 1;
}

static void start_shake_animation(sl_context_t *context)
{
    if (context->result != NULL)
    {
        gui_obj_hidden(GUI_BASE(context->result), true);
    }
    if (context->video != NULL)
    {
        gui_obj_hidden(GUI_BASE(context->video), false);
        gui_lite_video_set_repeat_count(context->video, GUI_VIDEO_REPEAT_INFINITE);
        gui_lite_video_set_state(context->video, GUI_VIDEO_STATE_PLAYING);
    }
    if (context->start != NULL)
    {
        gui_obj_hidden(GUI_BASE(context->start), true);
    }

    gui_fb_change();
}

static void show_lot(sl_context_t *context, int index)
{
    if (context->video != NULL)
    {
        gui_lite_video_set_state(context->video, GUI_VIDEO_STATE_STOP);
        gui_obj_hidden(GUI_BASE(context->video), true);
    }
    if (context->result != NULL)
    {
        gui_img_set_src(context->result, (const uint8_t *)lot_image_paths[index],
                        IMG_SRC_FILESYS);
        gui_img_refresh_size(context->result);
        gui_obj_hidden(GUI_BASE(context->result), false);
    }

    gui_fb_change();
}

/**
 * Update the filter and reversal score from one accelerometer sample.
 *
 * @return true when the score indicates active back-and-forth shaking.
 */
static bool update_shake(sl_context_t *context, int16_t gx, int16_t gy, int16_t gz)
{
    if (!context->filter_valid)
    {
        context->gravity_x = gx;
        context->gravity_y = gy;
        context->gravity_z = gz;
        context->filter_valid = true;
        return false;
    }

    /*
     * Estimate gravity with a one-pole low-pass filter. Subtracting that
     * baseline leaves the user's linear acceleration.
     */
    context->gravity_x += ((int32_t)gx - context->gravity_x) / 8;
    context->gravity_y += ((int32_t)gy - context->gravity_y) / 8;
    context->gravity_z += ((int32_t)gz - context->gravity_z) / 8;

    int32_t linear_x = (int32_t)gx - context->gravity_x;
    int32_t linear_y = (int32_t)gy - context->gravity_y;
    int32_t abs_x = linear_x < 0 ? -linear_x : linear_x;
    int32_t abs_y = linear_y < 0 ? -linear_y : linear_y;

    /*
     * The Manhattan amplitude determines activity, while the signed dominant
     * axis detects reversals through zero.
     */
    int32_t amplitude = abs_x + abs_y;
    int32_t projection = abs_x >= abs_y ? linear_x : linear_y;

    context->random_state ^= ((uint32_t)(uint16_t)gx * 31u) ^
                             ((uint32_t)(uint16_t)gy * 17u) ^
                             (uint32_t)(uint16_t)gz ^
                             (context->random_state << 1);

    if (context->warmup < SL_WARMUP_TICKS)
    {
        context->warmup++;
        return false;
    }

    if (amplitude < SL_QUIET_TH)
    {
        if (context->quiet_count < UINT8_MAX)
        {
            context->quiet_count++;
        }
    }
    else
    {
        context->quiet_count = 0;
    }

    /*
     * Count only direction reversals. The hysteresis band between the quiet
     * and active thresholds neither changes the score nor clears direction.
     */
    if (amplitude > SL_SHAKE_TH)
    {
        int8_t direction = projection >= 0 ? 1 : -1;
        if (context->last_direction != 0 && direction != context->last_direction)
        {
            context->score += amplitude > SL_STRONG_TH ? 2 : 1;
            if (context->score > SL_MAX_SCORE)
            {
                context->score = SL_MAX_SCORE;
            }
        }
        context->last_direction = direction;
    }
    else if (amplitude < SL_QUIET_TH)
    {
        if (context->score > 0)
        {
            context->score--;
        }
        context->last_direction = 0;
    }

    return context->score >= SL_START_SCORE;
}

static void shake_lot_tick(void *obj)
{
    GUI_UNUSED(obj);

#ifdef _HONEYGUI_SIMULATOR_
    /* The simulator has no accelerometer source. */
    return;
#else
    sl_context_t *context = &shake_lot_context;
    int16_t gx;
    int16_t gy;
    int16_t gz;

    if (!gsensor_sc7a20_read_xyz(&gx, &gy, &gz))
    {
        return;
    }

    bool shaking = update_shake(context, gx, gy, gz);

    switch (context->phase)
    {
    case SL_IDLE:
        if (shaking)
        {
            context->quiet_count = 0;
            start_shake_animation(context);
            context->phase = SL_SHAKING;
        }
        break;

    case SL_SHAKING:
        if (context->quiet_count >= SL_STOP_QUIET)
        {
            show_lot(context, pick_lot(context));
            context->score = 0;
            context->last_direction = 0;
            context->phase = SL_RESULT;
        }
        break;

    case SL_RESULT:
        if (shaking)
        {
            context->quiet_count = 0;
            start_shake_animation(context);
            context->phase = SL_SHAKING;
        }
        break;

    default:
        break;
    }
#endif
}

/*============================================================================*
 * Public API
 *============================================================================*/

int shake_lot(gui_obj_t *parent)
{
    sl_context_t *context = &shake_lot_context;

    memset(context, 0, sizeof(*context));
    context->phase = SL_IDLE;
    context->random_state = 0x2545F491u;

    uint32_t screen_width = gui_get_screen_width();

    context->start = gui_img_create_from_fs(parent, "sl_start",
                                            (void *)SL_IMG_START_PATH,
                                            0, 0, 0, 0);
    if (context->start != NULL)
    {
        gui_img_set_mode(context->start, IMG_BYPASS_MODE);
        gui_obj_hidden(GUI_BASE(context->start), false);
    }

    context->video = gui_lite_video_create_from_fs(parent, "sl_video",
                                                   (void *)SL_VIDEO_PATH,
                                                   0, 0,
                                                   (int16_t)screen_width,
                                                   (int16_t)screen_width);
    if (context->video != NULL)
    {
        gui_lite_video_set_repeat_count(context->video, GUI_VIDEO_REPEAT_INFINITE);
        gui_lite_video_set_state(context->video, GUI_VIDEO_STATE_STOP);
        gui_obj_hidden(GUI_BASE(context->video), true);
    }

    context->result = gui_img_create_from_fs(parent, "sl_result",
                                             (void *)lot_image_paths[0],
                                             0, 0, 0, 0);
    if (context->result != NULL)
    {
        gui_img_set_mode(context->result, IMG_BYPASS_MODE);
        gui_obj_hidden(GUI_BASE(context->result), true);
    }

    context->ctrl = gui_obj_create(parent, "sl_ctrl", 0, 0, 1, 1);
    gui_obj_create_timer(context->ctrl, SL_TICK_MS, true, shake_lot_tick);
    gui_obj_start_timer(context->ctrl);

    return 0;
}
