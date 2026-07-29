/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: MIT
 */

/*============================================================================*
 *                          Shake-Lot (摇签) Demo
 *
 * 摇一摇求签小应用。三态状态机：
 *   IDLE    等待摇晃（动画停、签图隐藏）
 *   SHAKING 识别到摇晃 -> 循环播放 lite-video 动画表示"摇签中"
 *   RESULT  停止摇晃   -> 按概率抽一支签，停动画、显示签面图
 * RESULT 态再次摇晃即重抽（清签 -> 重播动画 -> 停下再出新签）。
 *
 * 姿态输入 —— 3 轴 g-sensor（gsensor_sc7a20_read_xyz，单位 mg）：
 *   与 spatial_wallpaper（测姿态倾角）不同，本应用设备竖握、双手合十夹于掌心，
 *   设备 z 轴指向水平方向，来回摇动发生在"垂直于 z 轴"的 sensor x-y 平面内。
 *   先低通估出重力基线、raw 减重力得用户线性加速度，再取 x-y 平面主导轴的带
 *   符号分量。
 *
 * 识别用"漏桶"计分——只在主导轴线性加速度**方向翻转（过零）**时才 +1（强翻转
 * +2），故计分 ≈ 半周期往复次数：一次性单向运动（拿起 / 调姿 / 走路颠簸）不
 * 累积、不误触发，真正来回摇动快速累积。累计到 START_SCORE 判"开始摇"，连续
 * STOP_QUIET 个静止拍判"停止摇"。整体放宽阈值 / 计分以提高识别率。
 *
 * 资源为临时占位（当前 assets 仅有 1 个视频 / 数张图），集中在下方两个数组，
 * 后续整批替换即可。
 *============================================================================*/

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "guidef.h"
#include "gui_api.h"
#include "gui_obj.h"
#include "gui_img.h"
#include "gui_fb.h"
#include "gui_lite_video.h"
#include "shake_lot.h"

/*============================================================================*
 *                       Resources（临时占位，后续替换）
 *============================================================================*/

 #define SL_IMG_START_PATH   "/image/shake_lot/lot_start.bin"
/* 摇签动画 */
#define SL_VIDEO_PATH   "/image/shake_lot/sitck.avi"

#define SL_LOT_COUNT    7   /* 签的种类数 */

/* 7 种签的签面图。当前 assets 图片不足 7 张，先用现有图循环占位。 */
static const char *const k_lot_img[SL_LOT_COUNT] =
{
    "/image/shake_lot/lot_dj.bin",
    "/image/shake_lot/lot_j.bin",
    "/image/shake_lot/lot_zj.bin",
    "/image/shake_lot/lot_xj.bin",
    "/image/shake_lot/lot_mj.bin",
    "/image/shake_lot/lot_x.bin",
    "/image/shake_lot/lot_dx.bin",
};

/* 7 种签的默认概率权重（任意正整数，按总和归一化）。留待后续修改。 */
static uint16_t k_lot_weight[SL_LOT_COUNT] =
{
    20, 22, 25, 18, 10, 4, 1,   /* 和 = 100 */
};

/*============================================================================*
 *                       Shake-detection tunables（可调）
 *
 * 单位：加速度阈值为 mg（千分之一 g，1g≈1000）；计数单位为采样拍
 * （1 拍 = SL_TICK_MS）。mg 标度与已验证的 win_timer_gsensor_cb（同颗
 * SC7A20，阈值 300/400/80）一致。
 *
 * "摇 = 往复"：只在 x-y 平面线性加速度的**主导轴方向翻转（过零）**时才计分，
 * 因此计分值 ≈ 半周期往复次数。这样"拿起设备 / 调整握姿 / 走路颠簸"等一次性
 * 单向运动不累积、不误触发，而真正来回摇动会快速累积（放宽识别、提高识别率）。
 *============================================================================*/
#define SL_TICK_MS       30    /* 采样 / 驱动周期（~33 Hz，可靠捕捉 3~6 Hz 往复）*/
#define SL_SHAKE_TH      180   /* 活跃门槛（超过才计入方向；壁纸切页用 300，放宽）*/
#define SL_STRONG_TH     350   /* 强活跃门槛（一次翻转计 2 分）          */
#define SL_QUIET_TH      90    /* 静止门槛（低于才算"静"，两阈值间为迟滞区）*/
#define SL_START_SCORE   3     /* 累计到即判"开始摇"（≈3 次往复翻转，~0.5 s）*/
#define SL_MAX_SCORE     8     /* 计分上限，限制惯性余量                 */
#define SL_STOP_QUIET    8     /* 连续静止拍数判"停止摇"（~240 ms）      */
#define SL_WARMUP_TICKS  8     /* 重力滤波稳定期，期间不识别            */

/*============================================================================*
 *                              State
 *============================================================================*/
typedef enum
{
    SL_IDLE = 0,   /* 等待摇晃           */
    SL_SHAKING,    /* 摇晃中，播动画     */
    SL_RESULT,     /* 已出签，显示签图   */
} sl_phase_t;

typedef struct
{
    gui_obj_t        *root;      /* 承载所有子控件的容器           */
    gui_img_t        *start;    /* 开始图片    */
    gui_lite_video_t *video;     /* 摇签动画                       */
    gui_img_t        *result;    /* 出签图片                       */
    gui_obj_t        *ctrl;      /* 承载驱动 timer 的隐藏节点      */

    sl_phase_t phase;

    bool     filter_valid;            /* 重力基线是否已建立         */
    uint8_t  warmup;                  /* 稳定期计数                 */
    int32_t  grav_x, grav_y, grav_z;  /* 低通估计的重力分量         */
    int32_t  score;                   /* 漏桶计分                   */
    int8_t   last_dir;                /* 主导轴线性加速度上次方向（过零检测）*/
    uint8_t  quiet_cnt;               /* 连续静止拍数               */

    uint32_t rng;                     /* xorshift32 随机数状态      */
} sl_ctx_t;

/* 单例：摇签表盘同时只存在一个。 */
static sl_ctx_t g_sl;

/*============================================================================*
 *                        Random / probability
 *============================================================================*/

/* xorshift32：种子在摇晃过程中持续混入 g-sensor 噪声，保证每次结果不同。 */
static uint32_t sl_rng_next(sl_ctx_t *c)
{
    uint32_t x = c->rng ? c->rng : 0x2545F491u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    c->rng = x;
    return x;
}

/* 按权重的累积分布抽一支签，返回 [0, SL_LOT_COUNT)。 */
static int sl_pick_lot(sl_ctx_t *c)
{
    uint32_t total = 0;
    for (int i = 0; i < SL_LOT_COUNT; i++)
    {
        total += k_lot_weight[i];
    }
    if (total == 0)
    {
        return 0;
    }

    uint32_t r   = sl_rng_next(c) % total;
    uint32_t acc = 0;
    for (int i = 0; i < SL_LOT_COUNT; i++)
    {
        acc += k_lot_weight[i];
        if (r < acc)
        {
            return i;
        }
    }
    return SL_LOT_COUNT - 1;
}

/*============================================================================*
 *                        View transitions
 *============================================================================*/

/* 进入 / 回到"摇签中"：显示并从头循环播放动画，隐藏签图。 */
static void sl_start_anim(sl_ctx_t *c)
{
    if (c->result != NULL)
    {
        gui_obj_hidden((gui_obj_t *)c->result, true);
    }
    if (c->video != NULL)
    {
        gui_obj_hidden((gui_obj_t *)c->video, false);
        gui_lite_video_set_repeat_count(c->video, GUI_VIDEO_REPEAT_INFINITE);
        gui_lite_video_set_state(c->video, GUI_VIDEO_STATE_PLAYING); /* STOP->PLAY 从头播 */

        gui_obj_hidden((gui_obj_t *)g_sl.start, true);
    }
    gui_fb_change();
}

/* 出签：停动画并隐藏，换上抽中的签面图并显示。 */
static void sl_show_lot(sl_ctx_t *c, int idx)
{
    if (c->video != NULL)
    {
        gui_lite_video_set_state(c->video, GUI_VIDEO_STATE_STOP);
        gui_obj_hidden((gui_obj_t *)c->video, true);
    }
    if (c->result != NULL)
    {
        gui_img_set_src(c->result, (const uint8_t *)k_lot_img[idx], IMG_SRC_FILESYS);
        gui_img_refresh_size(c->result);   /* 换图后按新文件头更新宽高，避免沿用旧尺寸 */
        gui_obj_hidden((gui_obj_t *)c->result, false);
    }
    gui_fb_change();
}

/*============================================================================*
 *                        Shake detection
 *============================================================================*/

/* 送入一帧原始 g-sensor 样本，更新滤波 / 计分，返回当前是否"活跃摇动"。 */
static bool sl_update_shake(sl_ctx_t *c, int16_t gx, int16_t gy, int16_t gz)
{
    /* 第一帧建立重力基线 */
    if (!c->filter_valid)
    {
        c->grav_x = gx;
        c->grav_y = gy;
        c->grav_z = gz;
        c->filter_valid = true;
        return false;
    }

    /* 低通估重力（/8，与已验证的 win_timer_gsensor_cb 一致，避免右移对负数
     * 向下取整的偏差）；高通（raw - 重力）= 剔除姿态后的用户线性加速度。 */
    c->grav_x += ((int32_t)gx - c->grav_x) / 8;
    c->grav_y += ((int32_t)gy - c->grav_y) / 8;
    c->grav_z += ((int32_t)gz - c->grav_z) / 8;

    int32_t lx = (int32_t)gx - c->grav_x;
    int32_t ly = (int32_t)gy - c->grav_y;
    int32_t ax = lx < 0 ? -lx : lx;
    int32_t ay = ly < 0 ? -ly : ly;

    /* 设备 z 轴水平，来回摇动落在垂直 z 轴的 x-y 平面：
     *   amp  = 平面幅度（曼哈顿），用于活跃 / 静止判定；
     *   proj = 主导轴（当前帧幅度较大者）的带符号分量，用于方向翻转检测。 */
    int32_t amp  = ax + ay;
    int32_t proj = (ax >= ay) ? lx : ly;

    /* 用样本噪声持续搅动随机种子 */
    c->rng ^= ((uint32_t)(uint16_t)gx * 31u)
            ^ ((uint32_t)(uint16_t)gy * 17u)
            ^ (uint32_t)(uint16_t)gz
            ^ (c->rng << 1);

    /* 等重力滤波稳定后再识别 */
    if (c->warmup < SL_WARMUP_TICKS)
    {
        c->warmup++;
        return false;
    }

    /* 静止拍计数（低于静止门槛才算"静"，用于判停止摇） */
    if (amp < SL_QUIET_TH)
    {
        if (c->quiet_cnt < 255)
        {
            c->quiet_cnt++;
        }
    }
    else
    {
        c->quiet_cnt = 0;
    }

    /* "摇 = 往复"：只在主导轴线性加速度方向翻转（过零）时才计分，故计分值
     * ≈ 半周期往复次数。这样"拿起设备 / 调整握姿 / 走路颠簸"等一次性单向运动
     * 不累积、不误触发，而真正来回摇动会快速累积（放宽识别、提高识别率）。
     * 迟滞区 [QUIET_TH, SHAKE_TH] 内既不计分也不减分也不清方向，抑制过零抖动。 */
    if (amp > SL_SHAKE_TH)
    {
        int8_t dir = (proj >= 0) ? 1 : -1;
        if (c->last_dir != 0 && dir != c->last_dir)
        {
            c->score += (amp > SL_STRONG_TH) ? 2 : 1;
            if (c->score > SL_MAX_SCORE)
            {
                c->score = SL_MAX_SCORE;
            }
        }
        c->last_dir = dir;
    }
    else if (amp < SL_QUIET_TH)
    {
        /* 真正静止：漏桶缓慢泄放并清方向 */
        if (c->score > 0)
        {
            c->score--;
        }
        c->last_dir = 0;
    }

    return c->score >= SL_START_SCORE;
}

/*============================================================================*
 *                        Driver tick / state machine
 *============================================================================*/
static void sl_tick(void *obj)
{
    GUI_UNUSED(obj);
    sl_ctx_t *c = &g_sl;

    int16_t gx = 0, gy = 0, gz = 0;
#ifdef _HONEYGUI_SIMULATOR_
    /* PC 无传感器：此处可接入仿真数据。暂不识别。 */
    return;
#else
    extern bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z);
    if (!gsensor_sc7a20_read_xyz(&gx, &gy, &gz))
    {
        return;   /* 读失败：保持当前状态 */
    }
#endif

    bool shaking = sl_update_shake(c, gx, gy, gz);

    switch (c->phase)
    {
    case SL_IDLE:
        if (shaking)
        {
            c->quiet_cnt = 0;   /* 显式复位，勿依赖隐式状态 */
            sl_start_anim(c);
            c->phase = SL_SHAKING;
        }
        break;

    case SL_SHAKING:
        if (c->quiet_cnt >= SL_STOP_QUIET)
        {
            sl_show_lot(c, sl_pick_lot(c));
            c->score    = 0;
            c->last_dir = 0;
            c->phase    = SL_RESULT;
        }
        break;

    case SL_RESULT:
        /* 再次摇晃 -> 重抽 */
        if (shaking)
        {
            c->quiet_cnt = 0;
            sl_start_anim(c);
            c->phase = SL_SHAKING;
        }
        break;

    default:
        break;
    }
}

/*============================================================================*
 *                              Entry
 *============================================================================*/
int shake_lot(gui_obj_t *parent)
{
    memset(&g_sl, 0, sizeof(g_sl));
    g_sl.root  = parent;
    g_sl.phase = SL_IDLE;
    g_sl.rng   = 0x2545F491u;   /* 初始种子（会被 g-sensor 噪声持续搅动） */

    uint32_t scr = gui_get_screen_width();

    /* 出签图片控件：初始隐藏（占位用第 0 张，出签时再换 src） */
    g_sl.start = gui_img_create_from_fs(parent, "sl_start",
                                         (void *)SL_IMG_START_PATH, 0, 0, 0, 0);
    if (g_sl.start != NULL)
    {
        gui_img_set_mode(g_sl.start, IMG_BYPASS_MODE);
        gui_obj_hidden((gui_obj_t *)g_sl.start, false);
    }

    /* 动画控件：先建好，置停止 + 隐藏（进入 SHAKING 才播） */
    g_sl.video = gui_lite_video_create_from_fs(parent, "sl_video",
                                               (void *)SL_VIDEO_PATH,
                                               0, 0, (int16_t)scr, (int16_t)scr);
    if (g_sl.video != NULL)
    {
        gui_lite_video_set_repeat_count(g_sl.video, GUI_VIDEO_REPEAT_INFINITE);
        gui_lite_video_set_state(g_sl.video, GUI_VIDEO_STATE_STOP);
        gui_obj_hidden((gui_obj_t *)g_sl.video, true);
    }

    /* 出签图片控件：初始隐藏（占位用第 0 张，出签时再换 src） */
    g_sl.result = gui_img_create_from_fs(parent, "sl_result",
                                         (void *)k_lot_img[0], 0, 0, 0, 0);
    if (g_sl.result != NULL)
    {
        gui_img_set_mode(g_sl.result, IMG_BYPASS_MODE);
        gui_obj_hidden((gui_obj_t *)g_sl.result, true);
    }

    /* 驱动 timer：采样 g-sensor + 跑状态机 */
    g_sl.ctrl = gui_obj_create(parent, "sl_ctrl", 0, 0, 1, 1);
    gui_obj_create_timer(g_sl.ctrl, SL_TICK_MS, true, sl_tick);
    gui_obj_start_timer(g_sl.ctrl);

    return 0;
}
