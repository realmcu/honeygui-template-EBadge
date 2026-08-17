/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include "gsensor_reader.h"
#include "gui_img.h"
#include "gui_lite_video.h"

#define COIN_SENSOR_POLL_MS   10
#define COIN_MOTION_THRESHOLD 500

static bool coin_heads = true;
static bool coin_flipping = false;
static uint32_t random_state = 0x2545F491u;
static gui_lite_video_t *flip_video = NULL;

/*============================================================================*
 * File-local functions
 *============================================================================*/

static void coin_gsensor_cb(void *obj);

static uint32_t coin_random_next(void)
{
    uint32_t value = random_state;

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    random_state = value;

    return value;
}

static void coin_video_poll_cb(void *obj)
{
    if (flip_video != NULL && flip_video->state == GUI_VIDEO_STATE_STOP)
    {
        coin_flipping = false;
        gui_obj_create_timer(obj, COIN_SENSOR_POLL_MS, true, coin_gsensor_cb);
        gui_obj_start_timer(obj);
        gui_obj_hidden(obj, false);
    }
}

static void flip_coin_cb(void *obj, gui_event_t *event)
{
    GUI_UNUSED(event);

    if (coin_flipping)
    {
        return;
    }

    coin_flipping = true;
    coin_heads = ((coin_random_next() >> 31) & 1u) != 0u;

    const char *video_path = "/coin_flip_2_yes.avi";
    if (!coin_heads)
    {
        video_path = "/coin_flip_2_no.avi";
    }

    if (flip_video == NULL)
    {
        flip_video = gui_lite_video_create_from_fs(GUI_BASE(obj)->parent, NULL,
                                                   (void *)video_path,
                                                   0, 0, 0, 0);
    }
    else
    {
        gui_lite_video_set_src(flip_video, (void *)video_path, IMG_SRC_FILESYS);
    }

    gui_lite_video_set_frame_rate(flip_video, 30.0f);
    gui_lite_video_set_repeat_count(flip_video, 0); /* Play once. */
    gui_lite_video_set_state(flip_video, GUI_VIDEO_STATE_PLAYING);

    gui_obj_hidden(obj, true);
    gui_obj_create_timer(obj, COIN_SENSOR_POLL_MS, true, coin_video_poll_cb);
    gui_obj_start_timer(obj);
}

static void coin_gsensor_cb(void *obj)
{
#ifndef _HONEYGUI_SIMULATOR_
    static bool initialized = false;
    static int32_t gravity_x = 0;
    static int32_t gravity_y = 0;
    static int32_t gravity_z = 0;
    int16_t gx;
    int16_t gy;
    int16_t gz;

    if (coin_flipping || !gsensor_sc7a20_read_xyz(&gx, &gy, &gz))
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

    if (motion >= COIN_MOTION_THRESHOLD)
    {
        initialized = false;
        flip_coin_cb(obj, NULL);
    }
#else
    GUI_UNUSED(obj);
#endif
}

/*============================================================================*
 * Public API
 *============================================================================*/

void flip_coin_init(gui_obj_t *parent)
{
#ifndef _HONEYGUI_SIMULATOR_
    int16_t gx;
    int16_t gy;
    int16_t gz;

    if (gsensor_sc7a20_read_xyz(&gx, &gy, &gz))
    {
        random_state ^= ((uint32_t)(uint16_t)gx * 31u) ^
                        ((uint32_t)(uint16_t)gy * 17u) ^
                        (uint32_t)(uint16_t)gz ^
                        (random_state << 1);
    }
#endif

    coin_flipping = false;
    flip_video = NULL;

    const char *image_path = coin_heads ? "/coin_flip_yes.bin" : "/coin_flip_no.bin";
    gui_img_t *coin_image = gui_img_create_from_fs(parent, "coin_image",
                                                   (void *)image_path,
                                                   0, 0, 0, 0);
    gui_img_set_mode(coin_image, IMG_BYPASS_MODE);
    gui_obj_create_timer(GUI_BASE(coin_image), COIN_SENSOR_POLL_MS, true,
                         coin_gsensor_cb);
    gui_obj_start_timer(GUI_BASE(coin_image));
    gui_obj_add_event_cb(coin_image, (gui_event_cb_t)flip_coin_cb,
                         GUI_EVENT_TOUCH_CLICKED, NULL);
}
