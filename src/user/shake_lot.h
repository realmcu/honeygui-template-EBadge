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
 * @brief 摇签(shake-lot)小应用入口。
 *
 * 在 @p parent 下构建"摇晃识别 → 播放 lite-video 动画 → 按概率出签"的控件树，
 * 参考 spatial_wallpaper()，作为一张独立表盘挂载（mainface 类型 SRC_SHAKE_LOT）。
 * 自带 g-sensor 采样 timer 与状态机，调用方无需再驱动。
 *
 * @param parent 父 GUI 对象（switch_mainface 里为 win 下新建的容器）。
 * @return 0 成功。
 */
int shake_lot(gui_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* __SHAKE_LOT_H__ */
