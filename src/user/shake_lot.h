/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SHAKE_LOT_H__
#define __SHAKE_LOT_H__

#include "gui_obj.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create the shake-lot application under the supplied GUI object.
 *
 * The view samples the accelerometer, plays a lite-video animation while the
 * device is shaken, and selects a weighted result after movement stops.
 *
 * @param parent Parent object that owns the shake-lot controls.
 * @return 0 on success.
 */
int shake_lot(gui_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* __SHAKE_LOT_H__ */
