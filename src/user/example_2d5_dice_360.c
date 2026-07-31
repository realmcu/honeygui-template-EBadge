/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#include "guidef.h"
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "gui_obj.h"
#include "gui_img.h"
#include "draw_img.h"
#include "acc_api.h"
#include "gui_matrix.h"
#include "gui_fb.h"
#include "gui_server.h"
#include "gui_components_init.h"
#include "tp_algo.h"
#include "gui_obj_type.h"
#include "gui_vfs.h"

// #include "dice_litegfx/face_1.txt"
// #include "dice_litegfx/face_2.txt"
// #include "dice_litegfx/face_3.txt"
// #include "dice_litegfx/face_4.txt"
// #include "dice_litegfx/face_5.txt"
// #include "dice_litegfx/face_6.txt"
// #include "dice_litegfx/side.txt"
// #include "dice_litegfx/angle.txt"
// #include "dice_litegfx/floor_bg_360.txt"

/* ---------------- 物理参数(与 example_3d_dice.c 对齐) ---------------- */
#define DICE_NUM        3
#define PHYS_DT         0.016f
/* 竞技场半尺寸(世界系,单位≈骰子半径的倍数)。三者共同决定骰子落点范围。
   360x360 圆屏:缩小到下列值后,3 颗骰子(含身体+倒影)静止投影最大半径≈142
   (< 圆半径 180),四周留边、不会被圆角裁掉;见 tools/gen_floor_360.py 同款投影验证。 */
#define WALL_X          4.6f    /* 横向半宽:放宽让三颗骰子充分散开(允许贴边略出圆) */
#define WALL_Y          4.0f    /* 纵深半深:保持浅,落点垂直方向紧凑不拉长 */
#define ARENA_CZ        -0.9f   /* 竞技场纵深中心(世界深度):整体前后位置 */
#define RESTITUTION     0.60f
#define WALL_KICK       15.0f
#define COLLISION_DAMP  0.90f
#define COLLISION_ADAMP 0.92f
#define WALL_SPIN       0.15f
#define SETTLE_DAMP     0.965f   /* 落地后水平阻尼:偏紧,消除"漂"感 */
#define SETTLE_ADAMP    0.94f
#define ALIGN_VDAMP     0.85f
#define SPIN_GAIN       0.60f
#define COLLIDE_VN      0.15f
#define COLLIDE_SHRINK  0.5f
#define K_ALIGN         250.0f
#define C_ALIGN         32.0f
#define W_ALIGN         6.0f
#define ALIGN_DOT       0.99f
#define LOCK_W          4.0f
#define DIE_R           1.06f
#define MAX_SPEED       52.0f
#define THROW_VEL       16.0f
#define THROW_SPIN      20.0f
/* 重力/滞空同步调:GRAVITY 提到 180 后,起跳峰高 h = THROW_UP²/2G = 900/360 ≈ 2.5 半径
   与原参数持平(不飞出屏),但滞空时间 2v/g = 0.33s(原 0.5s),下落干脆有重量。 */
#define THROW_UP        30.0f   /* 起跳竖直速度:配合 GRAVITY 240,峰高维持 ~2.5 半径 */
#define GRAVITY         240.0f  /* 重力:大幅加重,落体"啵"的一声出得来,不再飘 */
#define Z_REST          0.28f   /* 落地回弹:只保留 ~8% 高度,一弹即停,消除多次小弹 */
#define Z_STOP          4.5f    /* 回弹停止阈值:随重力抬高,末端不再碎弹 */

/* ---------------- 加速度计驱动(倾斜滚动 + 甩动弹跳) ----------------
   每帧读三轴:低通分离"重力"(设备倾斜姿态)与"运动"(晃动分量)。
   - 重力在设备平面的两个分量 → 世界水平加速度(pos[0]左右 / pos[1]深度),骰子朝低处滚;
   - 运动分量突增(曼哈顿距离超阈值)→ 一次"甩动",按其方向给水平冲量 + 向上弹跳。
   轴序/符号取决于传感器在表上的安装方向,先在板子上翻正: */
#define ACC_FULLSCALE   1024.0f  /* SC7A20 ±2g/12bit:约 1024 计数/g,用于把原始值归一化到 g */
#define ACC_IDX_X       0        /* 用哪个轴当"世界左右"(pos[0]) */
#define ACC_IDX_Z       1        /* 用哪个轴当"世界深度"(pos[1]) */
#define ACC_SIGN_X      (1.0f)   /* 左右符号:倾斜方向反了就改 -1 */
#define ACC_SIGN_Z      (1.0f)   /* 深度符号 */
#define TILT_DEADZONE   0.07f    /* 倾斜死区(g):小于此按 0,避免静止时缓慢漂移 */
#define TILT_ACCEL      80.0f    /* 满倾(1g)对应的水平加速度(世界单位/s²):随 GRAVITY 抬升同步加大,倾斜时"推得动" */
#define ROLL_FRICTION   0.92f    /* 倾斜滚动每帧摩擦(<1):稍紧一点,倾斜归零后能更快停 */
#define SHAKE_THRESHOLD 500      /* 甩动触发阈值(运动分量三轴曼哈顿距离);越大越钝 */
#define SHAKE_SCALE     900.0f   /* 甩动强度归一:motion / 此值 得 strength(≈1 为一次有力甩) */

/* 模拟器预览(无加速度计):按住/拖拽=把"桌面"朝手指方向倾斜,点击一下=朝手指方向甩一把 */
#define SIM_DEPTH_SIGN  (1.0f)   /* 屏幕纵向拖拽映射到深度的符号(手感不对就改 -1) */

/* ---------------- 渲染参数(2.5D 投影) ---------------- */
#define DICE_IMG_WH     64          /* LiteGfx 点数面图尺寸(Face0X.png) */
#define SIDE_W          64          /* Side.png 棱条 */
#define SIDE_H          12
#define ANGLE_W         16          /* Angle.png 角贴片 */
#define ANGLE_H         16


/* 逐面光照(世界系平行光):Lambert + Blinn-Phong */
#define LIGHT_X         (-0.30f)
#define LIGHT_Y         ( 0.60f)
#define LIGHT_Z         (-0.70f)
#define LIGHT_AMBIENT   0.50f
#define LIGHT_DIFFUSE   0.50f
#define LIGHT_SPEC      0.55f
#define LIGHT_SHINE     12.0f
#define HALF_X          (-0.1556f)
#define HALF_Y          ( 0.5426f)
#define HALF_Z          (-0.8274f)
/* 棱/角面:沿用面的环境/漫反射(LIGHT_AMBIENT/LIGHT_DIFFUSE,连续着色)+ 柔化高光 → 倒角自然过渡 */
#define EDGE_SPEC       0.28f       /* 比面(0.55)弱,倒角略带光泽而不刺眼 */

/* ---------------- 针孔透视相机 ----------------
   与地板烘焙相机(tools/gen_litegfx_dice.py 的 FLOOR_PITCH/CAM_H/CAM_BACK/FOCAL/CAM_CY_FRAC)
   保持一致:骰子正好贴在木纹平面上;更俯视→透视更弱、地平线移到屏外、骰子可铺满整屏。 */
#define CAM_PITCH       0.52f    /* = FLOOR_PITCH,与地板同俯角 */
#define CAM_H           9.1f
#define CAM_BACK        18.2f
#define FOCAL           560.0f
#define CAM_CY_FRAC     0.62f    /* 主点下移:把落地骰子平面压到屏幕垂直中心(与 gen_floor_360.py 一致) */

/* 木纹背景尺寸(需与 floor_bg_360.txt / 圆屏一致) */
#define BG_W            360
#define BG_H            360

/* ---------------- 倒影(桌面 Y=0 镜像反射) ---------------- */
#define REFL_DIM        0.32f
#define REFL_H_FADE     0.13f

/* ---------------- 骰子状态 ---------------- */
typedef struct
{
    float pos[3];
    float vel[3];
    float quat[4];   /* x, y, z, w */
    float omega[3];
} dice_state_t;

static dice_state_t g_dice[DICE_NUM];

static int s_thrown = 0;
static int s_prev_press = 0;

/* 本帧的倾斜输入(g 分量, [-1,1]),由 dice_handle_input 每帧写入、physics_step 读取。
   g_tilt_gx→世界左右(pos[0]),g_tilt_gz→世界深度(pos[1]);落在死区内时为 0。 */
static float g_tilt_gx = 0.0f;
static float g_tilt_gz = 0.0f;

/* ---------------- 小工具 ---------------- */
static unsigned int s_rng = 0x1234abcdu;
static float frand(float lo, float hi)
{
    s_rng ^= s_rng << 13; s_rng ^= s_rng >> 17; s_rng ^= s_rng << 5;
    return lo + (hi - lo) * ((s_rng & 0xffffff) / (float)0xffffff);
}

static void quat_normalize(float q[4])
{
    float n = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (n < 1e-6f) { q[0] = q[1] = q[2] = 0.0f; q[3] = 1.0f; return; }
    float inv = 1.0f / n;
    q[0] *= inv; q[1] *= inv; q[2] *= inv; q[3] *= inv;
}

/* r = a * b  (Hamilton product, xyzw layout) */
static void quat_mul(const float a[4], const float b[4], float r[4])
{
    float ax = a[0], ay = a[1], az = a[2], aw = a[3];
    float bx = b[0], by = b[1], bz = b[2], bw = b[3];
    r[0] = aw * bx + ax * bw + ay * bz - az * by;
    r[1] = aw * by - ax * bz + ay * bw + az * bx;
    r[2] = aw * bz + ax * by - ay * bx + az * bw;
    r[3] = aw * bw - ax * bx - ay * by - az * bz;
}

static void quat_axis(int axis, float ang, float q[4])
{
    float s = sinf(ang * 0.5f), c = cosf(ang * 0.5f);
    q[0] = q[1] = q[2] = 0.0f; q[3] = c;
    q[axis] = s;
}

static void quat_integrate(float q[4], const float w[3], float dt)
{
    float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
    float wx = w[0], wy = w[1], wz = w[2];
    float dx =  wx * qw + wy * qz - wz * qy;
    float dy =  wy * qw + wz * qx - wx * qz;
    float dz =  wz * qw + wx * qy - wy * qx;
    float dw = -(wx * qx + wy * qy + wz * qz);
    q[0] += 0.5f * dx * dt;
    q[1] += 0.5f * dy * dt;
    q[2] += 0.5f * dz * dt;
    q[3] += 0.5f * dw * dt;
    quat_normalize(q);
}

static void quat_to_m3(const float q[4], float m[3][3])
{
    float x = q[0], y = q[1], z = q[2], w = q[3];
    m[0][0] = 1 - 2 * (y * y + z * z); m[0][1] = 2 * (x * y - z * w);     m[0][2] = 2 * (x * z + y * w);
    m[1][0] = 2 * (x * y + z * w);     m[1][1] = 1 - 2 * (x * x + z * z); m[1][2] = 2 * (y * z - x * w);
    m[2][0] = 2 * (x * z - y * w);     m[2][1] = 2 * (y * z + x * w);     m[2][2] = 1 - 2 * (x * x + y * y);
}

static int wall_bounce(float *p, float *v, float lo, float hi)
{
    if (*p < lo)
    {
        *p = lo;
        float b = -*v * RESTITUTION;
        if (b < WALL_KICK) { b = WALL_KICK; }
        *v = b;
        return 1;
    }
    else if (*p > hi)
    {
        *p = hi;
        float b = -*v * RESTITUTION;
        if (b > -WALL_KICK) { b = -WALL_KICK; }
        *v = b;
        return 1;
    }
    return 0;
}

/* 静止对齐:把最接近桌面法线(+Y)的本地轴贴到 ±Y,使某面平贴桌面。 */
static void quat_face_up(const float q[4], float out[4])
{
    float m[3][3];
    quat_to_m3(q, m);

    int k = 0; float best = fabsf(m[1][0]);
    if (fabsf(m[1][1]) > best) { k = 1; best = fabsf(m[1][1]); }
    if (fabsf(m[1][2]) > best) { k = 2; }

    float a[3] = { m[0][k], m[1][k], m[2][k] };
    float ty = (m[1][k] >= 0.0f) ? 1.0f : -1.0f;

    float hx = a[0], hy = a[1] + ty, hz = a[2];
    float hn = sqrtf(hx * hx + hy * hy + hz * hz);
    float dq[4];
    if (hn < 1e-6f) { dq[0] = dq[1] = dq[2] = 0.0f; dq[3] = 1.0f; }
    else
    {
        float inv = 1.0f / hn; hx *= inv; hy *= inv; hz *= inv;
        dq[0] = a[1] * hz - a[2] * hy;
        dq[1] = a[2] * hx - a[0] * hz;
        dq[2] = a[0] * hy - a[1] * hx;
        dq[3] = a[0] * hx + a[1] * hy + a[2] * hz;
    }

    float aw = dq[3], ax = dq[0], ay = dq[1], az = dq[2];
    float bw = q[3],  bx = q[0],  by = q[1],  bz2 = q[2];
    out[0] = aw * bx + ax * bw + ay * bz2 - az * by;
    out[1] = aw * by - ax * bz2 + ay * bw + az * bx;
    out[2] = aw * bz2 + ax * by - ay * bx + az * bw;
    out[3] = aw * bw - ax * bx - ay * by - az * bz2;
    quat_normalize(out);
}

static void quat_error_vec(const float q[4], const float tgt[4], float err[3])
{
    float cx = -q[0], cy = -q[1], cz = -q[2], cw = q[3];
    float tx = tgt[0], ty = tgt[1], tz = tgt[2], tw = tgt[3];
    float ex = tw * cx + tx * cw + ty * cz - tz * cy;
    float ey = tw * cy - tx * cz + ty * cw + tz * cx;
    float ez = tw * cz + tx * cy - ty * cx + tz * cw;
    float ew = tw * cw - tx * cx - ty * cy - tz * cz;
    float s = (ew >= 0.0f) ? 2.0f : -2.0f;
    err[0] = s * ex; err[1] = s * ey; err[2] = s * ez;
}

/* ---------------- 初始化:场地内随机撒开、静置 ---------------- */
static void dice_physics_init(void)
{
    const float rx = WALL_X - DIE_R;
    const float ry = WALL_Y - DIE_R;

    const float base = frand(0.0f, 6.2831853f);
    for (int i = 0; i < DICE_NUM; i++)
    {
        dice_state_t *d = &g_dice[i];
        float ang = base + (float)i * (6.2831853f / (float)DICE_NUM)
                    + frand(-0.35f, 0.35f);
        float rad = frand(0.35f, 0.95f);
        float x = cosf(ang) * rx * rad;
        float y = ARENA_CZ + sinf(ang) * ry * rad;

        d->pos[0] = x;
        d->pos[1] = y;
        d->pos[2] = 0.0f;
        d->vel[0] = 0.0f; d->vel[1] = 0.0f; d->vel[2] = 0.0f;

        float yaw = frand(0.35f, 0.75f);
        float ax  = frand(-0.06f, 0.06f);
        float az  = frand(-0.06f, 0.06f);
        float qflat[4], qyaw[4], qx[4], qz[4], q1[4], q2[4];
        quat_axis(0, -1.5707963f, qflat);
        quat_axis(1, yaw, qyaw);
        quat_axis(0, ax, qx);
        quat_axis(2, az, qz);
        quat_mul(qyaw, qflat, q1);
        quat_mul(qx, qz, q2);
        quat_mul(q1, q2, d->quat);
        quat_normalize(d->quat);

        d->omega[0] = d->omega[1] = d->omega[2] = 0.0f;
    }
}

/* ---------------- 每帧一步物理 ---------------- */
static void physics_step(dice_state_t *dice, int n)
{
    const float dt = PHYS_DT;

    for (int i = 0; i < n; i++)
    {
        dice_state_t *d = &dice[i];

        d->pos[0] += d->vel[0] * dt;
        d->pos[1] += d->vel[1] * dt;

        d->vel[2] -= GRAVITY * dt;
        d->pos[2] += d->vel[2] * dt;
        if (d->pos[2] < 0.0f)
        {
            d->pos[2] = 0.0f;
            if (d->vel[2] < 0.0f)
            {
                d->vel[2] = -d->vel[2] * Z_REST;
                if (d->vel[2] < Z_STOP) { d->vel[2] = 0.0f; }
                for (int k = 0; k < 3; k++) { d->omega[k] *= COLLISION_ADAMP; }
            }
        }
        int on_ground = (d->pos[2] <= 0.0001f && d->vel[2] <= Z_STOP);

        quat_integrate(d->quat, d->omega, dt);

        int bx = wall_bounce(&d->pos[0], &d->vel[0], -WALL_X + DIE_R, WALL_X - DIE_R);
        int by = wall_bounce(&d->pos[1], &d->vel[1],
                             ARENA_CZ - WALL_Y + DIE_R, ARENA_CZ + WALL_Y - DIE_R);
        if (bx)
        {
            d->omega[2] += -d->vel[1] * WALL_SPIN;
            d->vel[1]   *= COLLISION_DAMP;
            for (int k = 0; k < 3; k++) { d->omega[k] *= COLLISION_ADAMP; }
        }
        if (by)
        {
            d->omega[2] +=  d->vel[0] * WALL_SPIN;
            d->vel[0]   *= COLLISION_DAMP;
            for (int k = 0; k < 3; k++) { d->omega[k] *= COLLISION_ADAMP; }
        }

        float sp2 = d->vel[0] * d->vel[0] + d->vel[1] * d->vel[1];
        if (sp2 > MAX_SPEED * MAX_SPEED)
        {
            float f = MAX_SPEED / sqrtf(sp2);
            d->vel[0] *= f; d->vel[1] *= f;
        }

        float tilt_mag2 = g_tilt_gx * g_tilt_gx + g_tilt_gz * g_tilt_gz;
        if (on_ground && tilt_mag2 > 0.0f)
        {
            /* 倾斜驱动:桌面朝低处倾 → 水平加速度;摩擦给终端速度并可停住。 */
            d->vel[0] += g_tilt_gx * TILT_ACCEL * dt;
            d->vel[1] += g_tilt_gz * TILT_ACCEL * dt;
            d->vel[0] *= ROLL_FRICTION;
            d->vel[1] *= ROLL_FRICTION;
            /* 滚而非滑:角速度由水平速度决定,绕桌面内水平轴翻滚(ω = up × v / R)。 */
            d->omega[0] =  d->vel[1] / DIE_R;
            d->omega[1] *= SETTLE_ADAMP;
            d->omega[2] = -d->vel[0] / DIE_R;
            s_thrown = 1;   /* 倾斜停下后由下面的沉降/对齐逻辑接管,把骰子摆正 */
        }
        else if (s_thrown && on_ground)
        {
            d->vel[0] *= SETTLE_DAMP; d->vel[1] *= SETTLE_DAMP;
            for (int k = 0; k < 3; k++) { d->omega[k] *= SETTLE_ADAMP; }

            float wmag2 = d->omega[0] * d->omega[0] + d->omega[1] * d->omega[1]
                          + d->omega[2] * d->omega[2];
            if (wmag2 < W_ALIGN * W_ALIGN)
            {
                d->vel[0] *= ALIGN_VDAMP; d->vel[1] *= ALIGN_VDAMP;

                float qt[4];
                quat_face_up(d->quat, qt);
                float err[3];
                quat_error_vec(d->quat, qt, err);
                for (int k = 0; k < 3; k++)
                {
                    d->omega[k] += (K_ALIGN * err[k] - C_ALIGN * d->omega[k]) * dt;
                }

                float dot = d->quat[0] * qt[0] + d->quat[1] * qt[1]
                            + d->quat[2] * qt[2] + d->quat[3] * qt[3];
                float ww = d->omega[0] * d->omega[0] + d->omega[1] * d->omega[1]
                           + d->omega[2] * d->omega[2];
                if (fabsf(dot) > ALIGN_DOT && ww < LOCK_W * LOCK_W)
                {
                    d->quat[0] = qt[0]; d->quat[1] = qt[1];
                    d->quat[2] = qt[2]; d->quat[3] = qt[3];
                    d->vel[0] = d->vel[1] = 0.0f;
                    d->omega[0] = d->omega[1] = d->omega[2] = 0.0f;
                }
            }
        }
    }

    /* 骰子两两碰撞(XY 平面,随姿态膨胀的方块包围盒) */
    float hx[DICE_NUM], hy[DICE_NUM];
    for (int i = 0; i < n; i++)
    {
        float m[3][3];
        quat_to_m3(dice[i].quat, m);
        float sx = fabsf(m[0][0]) + fabsf(m[0][1]) + fabsf(m[0][2]);
        float sy = fabsf(m[1][0]) + fabsf(m[1][1]) + fabsf(m[1][2]);
        hx[i] = DIE_R * (1.0f + COLLIDE_SHRINK * (sx - 1.0f));
        hy[i] = DIE_R * (1.0f + COLLIDE_SHRINK * (sy - 1.0f));
    }
    for (int a = 0; a < n; a++)
    {
        for (int b = a + 1; b < n; b++)
        {
            dice_state_t *A = &dice[a], *B = &dice[b];
            float dx = B->pos[0] - A->pos[0];
            float dy = B->pos[1] - A->pos[1];
            float ox = (hx[a] + hx[b]) - fabsf(dx);
            float oy = (hy[a] + hy[b]) - fabsf(dy);
            if (ox > 0.0f && oy > 0.0f)
            {
                float nx = 0.0f, ny = 0.0f, pen;
                if (ox < oy) { nx = (dx >= 0.0f) ? 1.0f : -1.0f; pen = ox; }
                else         { ny = (dy >= 0.0f) ? 1.0f : -1.0f; pen = oy; }
                float half = pen * 0.5f;
                A->pos[0] -= nx * half; A->pos[1] -= ny * half;
                B->pos[0] += nx * half; B->pos[1] += ny * half;

                float rvx = B->vel[0] - A->vel[0];
                float rvy = B->vel[1] - A->vel[1];
                float vn = rvx * nx + rvy * ny;
                if (vn < -COLLIDE_VN)
                {
                    float j = -(1.0f + RESTITUTION) * vn * 0.5f;
                    A->vel[0] -= j * nx; A->vel[1] -= j * ny;
                    B->vel[0] += j * nx; B->vel[1] += j * ny;

                    float sp = fabsf(vn) * SPIN_GAIN;
                    for (int k = 0; k < 3; k++)
                    {
                        A->omega[k] += frand(-sp, sp);
                        B->omega[k] += frand(-sp, sp);
                    }

                    A->vel[0] *= COLLISION_DAMP; A->vel[1] *= COLLISION_DAMP;
                    B->vel[0] *= COLLISION_DAMP; B->vel[1] *= COLLISION_DAMP;
                    for (int k = 0; k < 3; k++)
                    {
                        A->omega[k] *= COLLISION_ADAMP;
                        B->omega[k] *= COLLISION_ADAMP;
                    }
                }
            }
        }
    }
}

/* ====================================================================== *
 *                         2.5D 立方体贴图渲染                             *
 * ====================================================================== */

static const gui_vertex_t s_cube_v[8] =
{
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
};

static const int   s_face_idx[6][4] =
{
    {0, 3, 2, 1},   /* FRONT  法线(0,0,-1) → 1 点 */
    {4, 5, 6, 7},   /* BACK   法线(0,0, 1) → 6 点 */
    {5, 1, 2, 6},   /* UP     法线(1,0, 0) → 2 点 */
    {0, 4, 7, 3},   /* DOWN   法线(-1,0,0) → 5 点 */
    {7, 6, 2, 3},   /* LEFT   法线(0,1, 0) → 3 点 */
    {0, 1, 5, 4},   /* RIGHT  法线(0,-1,0) → 4 点 */
};
static const gui_vertex_t s_face_n[6] =
{
    { 0,  0, -1}, { 0,  0,  1}, { 1,  0,  0},
    {-1,  0,  0}, { 0,  1,  0}, { 0, -1,  0},
};

#define N_FACE        6
#define N_EDGE        12
#define N_CORNER      8
#define N_QUAD        (N_FACE + N_EDGE + N_CORNER)   /* 26 */
/* 倒角带宽:面边 = 12:64(与 LiteGfx 的 Side 64x12 贴图比例一致),
   反解 2(1-BEVEL) = 5.33*sqrt(2)*BEVEL → BEVEL≈0.20,Side 贴上去不被压扁。 */
#define BEVEL         0.20f
/* grow 全归 1:面/棱/角顶点严格共享同一套倒角立方体顶点 → 天然贴合、无错位。 */
#define FACE_GROW        1.0f
#define EDGE_GROW        1.0f
#define EDGE_GROW_ALONG  1.0f
#define CORNER_GROW      1.0f
/* 点数面/倒角 side 贴图外沿有约 1~2px 抗锯齿渐变(不透明区内缩了)。不动几何四边形,
   而是把它当作纹理内缩矩形 [inset,w-inset] 的映射目标(见 face_set_matrix_inset):
   不透明区顶到几何边、外圈渐变溢到边外盖缝,相邻块不透明体"相接"而非"重叠"。
   单位=纹理像素,按 img_w/img_h 逐轴换算(薄条 side y 向内缩比例更大)。可微调:
   太大→轮廓胖/边虚,太小→缝没盖住。 */
#define EDGE_INSET_PX    1.5f

static gui_vertex_t s_qv[N_QUAD][4];
static gui_vertex_t s_qn[N_QUAD];

static void grow_quad(gui_vertex_t v[4], float g)
{
    float cx = (v[0].x + v[1].x + v[2].x + v[3].x) * 0.25f;
    float cy = (v[0].y + v[1].y + v[2].y + v[3].y) * 0.25f;
    float cz = (v[0].z + v[1].z + v[2].z + v[3].z) * 0.25f;
    for (int k = 0; k < 4; k++)
    {
        v[k].x = cx + (v[k].x - cx) * g;
        v[k].y = cy + (v[k].y - cy) * g;
        v[k].z = cz + (v[k].z - cz) * g;
    }
}


static void grow_edge(gui_vertex_t v[4], float gp, float ga, int axis)
{
    float c[3] = {
        (v[0].x + v[1].x + v[2].x + v[3].x) * 0.25f,
        (v[0].y + v[1].y + v[2].y + v[3].y) * 0.25f,
        (v[0].z + v[1].z + v[2].z + v[3].z) * 0.25f
    };
    for (int k = 0; k < 4; k++)
    {
        float p[3] = { v[k].x, v[k].y, v[k].z };
        for (int a = 0; a < 3; a++)
        {
            float g = (a == axis) ? ga : gp;
            p[a] = c[a] + (p[a] - c[a]) * g;
        }
        v[k].x = p[0]; v[k].y = p[1]; v[k].z = p[2];
    }
}

static void geometry_build(void)
{
    const float t = 1.0f - BEVEL;
    int qi = 0;

    for (int f = 0; f < N_FACE; f++)
    {
        int axis = (s_face_n[f].x != 0.0f) ? 0 : (s_face_n[f].y != 0.0f) ? 1 : 2;
        for (int k = 0; k < 4; k++)
        {
            gui_vertex_t v = s_cube_v[s_face_idx[f][k]];
            float c[3] = { v.x, v.y, v.z };
            for (int a = 0; a < 3; a++) { if (a != axis) { c[a] *= t; } }
            s_qv[qi][k].x = c[0]; s_qv[qi][k].y = c[1]; s_qv[qi][k].z = c[2];
        }
        s_qn[qi] = s_face_n[f];
        grow_quad(s_qv[qi], FACE_GROW);
        qi++;
    }

    for (int axis = 0; axis < 3; axis++)
    {
        int p = (axis + 1) % 3, q = (axis + 2) % 3;
        for (int sp = -1; sp <= 1; sp += 2)
        {
            for (int sq = -1; sq <= 1; sq += 2)
            {
                float corners[4][3];
                corners[0][axis] = -t; corners[0][p] = (float)sp;     corners[0][q] = sq * t;
                corners[1][axis] =  t; corners[1][p] = (float)sp;     corners[1][q] = sq * t;
                corners[2][axis] =  t; corners[2][p] = sp * t;        corners[2][q] = (float)sq;
                corners[3][axis] = -t; corners[3][p] = sp * t;        corners[3][q] = (float)sq;
                for (int k = 0; k < 4; k++)
                {
                    s_qv[qi][k].x = corners[k][0];
                    s_qv[qi][k].y = corners[k][1];
                    s_qv[qi][k].z = corners[k][2];
                }
                float n[3] = { 0.0f, 0.0f, 0.0f };
                n[p] = (float)sp; n[q] = (float)sq;
                s_qn[qi].x = n[0] * 0.70710678f;
                s_qn[qi].y = n[1] * 0.70710678f;
                s_qn[qi].z = n[2] * 0.70710678f;
                grow_edge(s_qv[qi], EDGE_GROW, EDGE_GROW_ALONG, axis);
                qi++;
            }
        }
    }

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sy = -1; sy <= 1; sy += 2)
        {
            for (int sz = -1; sz <= 1; sz += 2)
            {
                float px[3] = { (float)sx, sy * t,     sz * t     };
                float py[3] = { sx * t,    (float)sy,   sz * t     };
                float pz[3] = { sx * t,    sy * t,      (float)sz  };
                float *pts[4] = { px, py, pz, px };
                for (int k = 0; k < 4; k++)
                {
                    s_qv[qi][k].x = pts[k][0];
                    s_qv[qi][k].y = pts[k][1];
                    s_qv[qi][k].z = pts[k][2];
                }
                s_qn[qi].x = sx * 0.57735027f;
                s_qn[qi].y = sy * 0.57735027f;
                s_qn[qi].z = sz * 0.57735027f;
                grow_quad(s_qv[qi], CORNER_GROW);
                qi++;
            }
        }
    }

}

static float s_cam_c = 1.0f, s_cam_s = 0.0f;
static float s_cx0 = 0.0f,  s_cy0 = 0.0f;
static void view_build(void)
{
    s_cam_c = cosf(CAM_PITCH);
    s_cam_s = sinf(CAM_PITCH);
}

static float cam_project(const float pw[3], float *sx, float *sy)
{
    float dx = pw[0];
    float dy = pw[1] - CAM_H;
    float dz = pw[2] + CAM_BACK;
    float py =  s_cam_c * dy + s_cam_s * dz;
    float pz = -s_cam_s * dy + s_cam_c * dz;
    if (pz < 0.1f) { pz = 0.1f; }
    *sx = s_cx0 + FOCAL * dx / pz;
    *sy = s_cy0 - FOCAL * py / pz;
    return pz;
}

typedef struct gui_dice
{
    gui_obj_t      base;
    draw_img_t     draw_img[DICE_NUM][N_QUAD];
    float          nz[DICE_NUM][N_QUAD];
    draw_img_t     draw_img_refl[DICE_NUM][N_QUAD];
    float          nz_refl[DICE_NUM][N_QUAD];
} gui_dice_t;

static void dice_blit_matrix(float w, float h,
                             gui_vertex_t *v0, gui_vertex_t *v1,
                             gui_vertex_t *v2, gui_vertex_t *v3,
                             gui_matrix_t *matrix)
{
    float x0 = v0->x, y0 = v0->y;
    float x1 = v1->x, y1 = v1->y;
    float x2 = v2->x, y2 = v2->y;
    float x3 = v3->x, y3 = v3->y;

    float dx1 = x1 - x2, dx2 = x3 - x2, dx3 = x0 - x1 + x2 - x3;
    float dy1 = y1 - y2, dy2 = y3 - y2, dy3 = y0 - y1 + y2 - y3;

    float a, b, c, d, e, f, g, hc;
    if (fabsf(dx3) < 1e-6f && fabsf(dy3) < 1e-6f)
    {
        a = x1 - x0; b = x3 - x0; c = x0;
        d = y1 - y0; e = y3 - y0; f = y0;
        g = 0.0f; hc = 0.0f;
    }
    else
    {
        float den = dx1 * dy2 - dx2 * dy1;
        if (fabsf(den) < 1e-6f)
        {
            matrix->m[0][0] = 1.0f; matrix->m[0][1] = 0.0f; matrix->m[0][2] = 0.0f;
            matrix->m[1][0] = 0.0f; matrix->m[1][1] = 1.0f; matrix->m[1][2] = 0.0f;
            matrix->m[2][0] = 0.0f; matrix->m[2][1] = 0.0f; matrix->m[2][2] = 1.0f;
            return;
        }
        g  = (dx3 * dy2 - dx2 * dy3) / den;
        hc = (dx1 * dy3 - dx3 * dy1) / den;
        a = x1 - x0 + g * x1;  b = x3 - x0 + hc * x3;  c = x0;
        d = y1 - y0 + g * y1;  e = y3 - y0 + hc * y3;  f = y0;
    }

    matrix->m[0][0] = a / w;  matrix->m[0][1] = b / h;   matrix->m[0][2] = c;
    matrix->m[1][0] = d / w;  matrix->m[1][1] = e / h;   matrix->m[1][2] = f;
    matrix->m[2][0] = g / w;  matrix->m[2][1] = hc / h;  matrix->m[2][2] = 1.0f;
}

static float face_brightness(const float n[3])
{
    float ndl = n[0] * LIGHT_X + n[1] * LIGHT_Y + n[2] * LIGHT_Z;
    if (ndl < 0.0f) { ndl = 0.0f; }
    float ndh = n[0] * HALF_X + n[1] * HALF_Y + n[2] * HALF_Z;
    if (ndh < 0.0f) { ndh = 0.0f; }
    float spec = ndh;
    for (int e = 1; e < (int)LIGHT_SHINE; e++) { spec *= ndh; }
    float b = LIGHT_AMBIENT + LIGHT_DIFFUSE * ndl + LIGHT_SPEC * spec;
    if (b > 1.0f) { b = 1.0f; }
    return b;
}

static float edge_brightness(const float n[3])
{
    /* 与 face_brightness 同一套环境/漫反射(连续),外加柔化高光:
       倒角法线介于两相邻面之间 → 亮度自然过渡,不再是包边暗框。 */
    float ndl = n[0] * LIGHT_X + n[1] * LIGHT_Y + n[2] * LIGHT_Z;
    if (ndl < 0.0f) { ndl = 0.0f; }
    float ndh = n[0] * HALF_X + n[1] * HALF_Y + n[2] * HALF_Z;
    if (ndh < 0.0f) { ndh = 0.0f; }
    float spec = ndh;
    for (int e = 1; e < (int)LIGHT_SHINE; e++) { spec *= ndh; }
    float b = LIGHT_AMBIENT + LIGHT_DIFFUSE * ndl + EDGE_SPEC * spec;
    if (b > 1.0f) { b = 1.0f; }
    return b;
}

static float quad_area(const gui_vertex_t rv[4])
{
    return 0.5f * fabsf(
               rv[0].x * rv[1].y - rv[1].x * rv[0].y +
               rv[1].x * rv[2].y - rv[2].x * rv[1].y +
               rv[2].x * rv[3].y - rv[3].x * rv[2].y +
               rv[3].x * rv[0].y - rv[0].x * rv[3].y);
}

static void face_set_matrix(gui_obj_t *obj, draw_img_t *img, gui_vertex_t rv[4])
{
    gui_matrix_t blit;
    dice_blit_matrix((float)img->img_w, (float)img->img_h,
                     &rv[0], &rv[1], &rv[2], &rv[3], &blit);
    gui_matrix_t tmp;
    memcpy(&tmp, obj->matrix, sizeof(gui_matrix_t));
    matrix_multiply(&tmp, &blit);
    memcpy(&img->matrix, &tmp, sizeof(gui_matrix_t));
    memcpy(&img->inverse, &tmp, sizeof(gui_matrix_t));
    matrix_inverse(&img->inverse);
    draw_img_new_area(img, NULL);
}

/* 同 face_set_matrix,但把几何四边形 rv 当作纹理内缩矩形 [inset,w-inset]×[inset,h-inset]
   的映射目标:用双线性把 rv(参数 (0,0)(1,0)(1,1)(0,1))外推出整张图 4 角 E,再照常建矩阵。
   于是纹理不透明区顶到几何边,外沿抗锯齿渐变落到 rv 之外(盖缝),不透明体相邻"相接"不"重叠"。
   在纹理空间偏移——除以常量、不求交,近侧视薄条也不会算爆;各向异性由 w/h 天然处理。 */
static void face_set_matrix_inset(gui_obj_t *obj, draw_img_t *img, gui_vertex_t rv[4], float inset)
{
    float w = (float)img->img_w, h = (float)img->img_h;
    float dw = w - 2.0f * inset, dh = h - 2.0f * inset;
    if (dw < 1.0f) { dw = 1.0f; }
    if (dh < 1.0f) { dh = 1.0f; }
    float sN = inset / dw, tN = inset / dh;     /* 纹理 x=0→s=-sN, x=w→1+sN;y 同理 */
    const float ss[4] = { -sN, 1.0f + sN, 1.0f + sN, -sN };
    const float tt[4] = { -tN, -tN, 1.0f + tN, 1.0f + tN };
    gui_vertex_t E[4];
    for (int k = 0; k < 4; k++)
    {
        float s = ss[k], t = tt[k];
        float w0 = (1.0f - s) * (1.0f - t), w1 = s * (1.0f - t);
        float w2 = s * t,                   w3 = (1.0f - s) * t;
        E[k].x = w0 * rv[0].x + w1 * rv[1].x + w2 * rv[2].x + w3 * rv[3].x;
        E[k].y = w0 * rv[0].y + w1 * rv[1].y + w2 * rv[2].y + w3 * rv[3].y;
        E[k].z = 0.0f;
    }
    gui_matrix_t blit;
    dice_blit_matrix(w, h, &E[0], &E[1], &E[2], &E[3], &blit);
    gui_matrix_t tmp;
    memcpy(&tmp, obj->matrix, sizeof(gui_matrix_t));
    matrix_multiply(&tmp, &blit);
    memcpy(&img->matrix, &tmp, sizeof(gui_matrix_t));
    memcpy(&img->inverse, &tmp, sizeof(gui_matrix_t));
    matrix_inverse(&img->inverse);
    draw_img_new_area(img, NULL);
}

/* Angle.png 里不透明等边三角(尖朝上)的 3 个顶点,单位:图像像素(原点左上)。
   量自 alpha 掩膜:尖 (7.5,0.5)、左底 (2.5,10.5)、右底 (13.5,10.5)。 */
#define ANGLE_APEX_X 7.5f
#define ANGLE_APEX_Y 0.5f
#define ANGLE_BL_X   2.5f
#define ANGLE_BL_Y   10.5f
#define ANGLE_BR_X   13.5f
#define ANGLE_BR_Y   10.5f

/* 倒角三角面用退化四边形 {px,py,pz,px} 表示。HoneyGUI 只能把图像 4 角映射到 4 顶点,
   而三角纹理的 3 个尖点在图像内部,直接映射会错位露黑。
   这里反过来:先按“三角纹理 3 顶点 → 倒角面 3 投影点(s0,s1,s2)”定一个仿射,
   再用它把图像 4 角映射出去,得到 rv[4]。这样整张图仿射贴合,三角刚好盖住角面,
   透明区落在角面之外(露出先画的棱/木纹,而非黑缝)。s0/s1/s2 对应 px/py/pz。 */
static void corner_affine_rv(const gui_vertex_t *s0, const gui_vertex_t *s1,
                             const gui_vertex_t *s2, gui_vertex_t rv[4])
{
    const float u0x = ANGLE_APEX_X, u0y = ANGLE_APEX_Y;
    const float e1x = ANGLE_BL_X - u0x, e1y = ANGLE_BL_Y - u0y;
    const float e2x = ANGLE_BR_X - u0x, e2y = ANGLE_BR_Y - u0y;
    float det = e1x * e2y - e2x * e1y;
    if (det == 0.0f) { det = 1e-6f; }
    const float cx[4] = { 0.0f, (float)ANGLE_W, (float)ANGLE_W, 0.0f };
    const float cy[4] = { 0.0f, 0.0f, (float)ANGLE_H, (float)ANGLE_H };
    for (int k = 0; k < 4; k++)
    {
        float dx = cx[k] - u0x, dy = cy[k] - u0y;
        float a = (dx * e2y - e2x * dy) / det;   /* 沿 e1(→左底)分量 */
        float b = (e1x * dy - dx * e1y) / det;   /* 沿 e2(→右底)分量 */
        rv[k].x = s0->x + a * (s1->x - s0->x) + b * (s2->x - s0->x);
        rv[k].y = s0->y + a * (s1->y - s0->y) + b * (s2->y - s0->y);
        rv[k].z = 0.0f;
    }
}

/* 一次"甩动":按 (dirx,dirz) 方向给所有骰子水平冲量 + 向上弹跳。
   dir 为世界水平面单位向量(可为 0=纯竖直甩),strength≈1 为一次有力甩动。
   与随机掷不同:方向由三轴甩动分量决定,只叠一点随机让三颗略有差异。 */
static void dice_shake_impulse(float dirx, float dirz, float strength)
{
    if (strength > 1.5f) { strength = 1.5f; }
    if (strength < 0.3f) { strength = 0.3f; }
    for (int i = 0; i < DICE_NUM; i++)
    {
        dice_state_t *d = &g_dice[i];
        d->vel[0] += dirx * THROW_VEL * strength + frand(-1.0f, 1.0f) * THROW_VEL * 0.25f;
        d->vel[1] += dirz * THROW_VEL * strength + frand(-1.0f, 1.0f) * THROW_VEL * 0.25f;
        d->vel[2]  = THROW_UP * (0.5f + 0.6f * strength) * frand(0.85f, 1.15f);
        for (int k = 0; k < 3; k++) { d->omega[k] = frand(-THROW_SPIN, THROW_SPIN) * strength; }
    }
    s_thrown = 1;
}

static void dice_handle_input(void)
{
#ifdef _HONEYGUI_SIMULATOR_
    /* 模拟器无加速度计:用鼠标模拟。按住/拖拽 = 把桌面朝手指方向倾斜(骰子朝手指滚);
       点击的上升沿 = 朝手指方向甩一把。便于在 PC 上预览两种手感。 */
    touch_info_t *tp = tp_get_info();
    int pressing = (tp && tp->pressing) ? 1 : 0;
    if (pressing)
    {
        float nx = ((float)tp->x - BG_W * 0.5f) / (BG_W * 0.5f);
        float ny = ((float)tp->y - BG_H * 0.5f) / (BG_H * 0.5f);
        if (nx >  1.0f) { nx =  1.0f; } if (nx < -1.0f) { nx = -1.0f; }
        if (ny >  1.0f) { ny =  1.0f; } if (ny < -1.0f) { ny = -1.0f; }
        g_tilt_gx = nx;
        g_tilt_gz = SIM_DEPTH_SIGN * ny;
    }
    else
    {
        g_tilt_gx = 0.0f; g_tilt_gz = 0.0f;
    }
    if (pressing && !s_prev_press)
    {
        float dx = g_tilt_gx, dz = g_tilt_gz;
        float dn = sqrtf(dx * dx + dz * dz);
        if (dn > 1e-3f) { dx /= dn; dz /= dn; } else { dx = 0.0f; dz = 0.0f; }
        dice_shake_impulse(dx, dz, 1.0f);
    }
    s_prev_press = pressing;
#else
    /* 板子:加速度计三轴实时驱动(倾斜滚动 + 甩动弹跳),读取方式参考 EBadge flip_coin.c。
       1) 低通滤波(/8)让 gravity_* 跟踪重力方向 = 设备倾斜姿态;
       2) 重力在设备平面的两个分量归一化到 g → 世界水平倾斜(死区外才生效);
       3) 当前读数减去重力 = 运动分量,曼哈顿距离超阈值 → 按其方向甩一把。 */
    extern bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z);
    static bool    initialized = false;
    static int32_t grav[3] = { 0, 0, 0 };
    int16_t gx, gy, gz;

    if (!gsensor_sc7a20_read_xyz(&gx, &gy, &gz)) { return; }
    int32_t raw[3] = { gx, gy, gz };

    if (!initialized)
    {
        grav[0] = raw[0]; grav[1] = raw[1]; grav[2] = raw[2];
        initialized = true;
        return;
    }

    for (int k = 0; k < 3; k++) { grav[k] += (raw[k] - grav[k]) / 8; }

    /* 倾斜 → 世界水平 g 分量(死区内按 0,防静止漂移) */
    float txg = ACC_SIGN_X * (float)grav[ACC_IDX_X] / ACC_FULLSCALE;
    float tzg = ACC_SIGN_Z * (float)grav[ACC_IDX_Z] / ACC_FULLSCALE;
    g_tilt_gx = (txg > TILT_DEADZONE || txg < -TILT_DEADZONE) ? txg : 0.0f;
    g_tilt_gz = (tzg > TILT_DEADZONE || tzg < -TILT_DEADZONE) ? tzg : 0.0f;

    /* 运动分量 → 甩动检测与方向 */
    int32_t m[3];
    for (int k = 0; k < 3; k++) { m[k] = raw[k] - grav[k]; }
    int32_t motion = (m[0] < 0 ? -m[0] : m[0]) + (m[1] < 0 ? -m[1] : m[1])
                     + (m[2] < 0 ? -m[2] : m[2]);
    if (motion >= SHAKE_THRESHOLD)
    {
        float dx = ACC_SIGN_X * (float)m[ACC_IDX_X];
        float dz = ACC_SIGN_Z * (float)m[ACC_IDX_Z];
        float dn = sqrtf(dx * dx + dz * dz);
        if (dn > 1e-3f) { dx /= dn; dz /= dn; } else { dx = 0.0f; dz = 0.0f; }
        dice_shake_impulse(dx, dz, (float)motion / SHAKE_SCALE);
        initialized = false;   /* 重置重力基线,避免甩动余震连续触发 */
    }
    (void)s_prev_press;
#endif
}

static void dice2d5_prepare(gui_obj_t *obj)
{
    gui_dice_t *this = (gui_dice_t *)obj;
    gui_dispdev_t *dc = gui_get_dc();

    s_cx0 = dc->screen_width * 0.5f;
    s_cy0 = dc->screen_height * CAM_CY_FRAC;

    for (int d = 0; d < DICE_NUM; d++)
    {
        float m3[3][3];
        quat_to_m3(g_dice[d].quat, m3);

        float ctr[3] = { g_dice[d].pos[0], DIE_R + g_dice[d].pos[2], g_dice[d].pos[1] };

        for (int q = 0; q < N_QUAD; q++)
        {
            gui_vertex_t nl = s_qn[q];
            float nw[3] = {
                m3[0][0] * nl.x + m3[0][1] * nl.y + m3[0][2] * nl.z,
                m3[1][0] * nl.x + m3[1][1] * nl.y + m3[1][2] * nl.z,
                m3[2][0] * nl.x + m3[2][1] * nl.y + m3[2][2] * nl.z
            };

            float pw4[4][3];
            for (int kk = 0; kk < 4; kk++)
            {
                float vl[3] = { s_qv[q][kk].x * DIE_R, s_qv[q][kk].y * DIE_R, s_qv[q][kk].z * DIE_R };
                pw4[kk][0] = ctr[0] + m3[0][0] * vl[0] + m3[0][1] * vl[1] + m3[0][2] * vl[2];
                pw4[kk][1] = ctr[1] + m3[1][0] * vl[0] + m3[1][1] * vl[1] + m3[1][2] * vl[2];
                pw4[kk][2] = ctr[2] + m3[2][0] * vl[0] + m3[2][1] * vl[1] + m3[2][2] * vl[2];
            }
            float fc[3] = { ctr[0] + nw[0] * DIE_R, ctr[1] + nw[1] * DIE_R, ctr[2] + nw[2] * DIE_R };

            float vv[3] = { fc[0], fc[1] - CAM_H, fc[2] + CAM_BACK };
            float facing = nw[0] * vv[0] + nw[1] * vv[1] + nw[2] * vv[2];
            this->nz[d][q] = -facing;
            if (facing < 0.0f)
            {
                float br = (q < N_FACE) ? face_brightness(nw) : edge_brightness(nw);
                /* 面/棱/角统一走 SRC_MODE:opacity 作亮度缩放,逐像素 alpha 同时驱动
                   src-over(角贴图透明三角自动保留)。角用 edge_brightness,与倒角棱条同调。 */
                this->draw_img[d][q].opacity_value = (uint8_t)(br * 255.0f);

                gui_vertex_t rv[4];
                if (q >= N_FACE + N_EDGE)
                {
                    /* 角:退化四边形前 3 点 px,py,pz 是倒角三角面的 3 顶点,
                       用仿射把 Angle 三角纹理贴到这三点上。 */
                    gui_vertex_t s[3];
                    for (int kk = 0; kk < 3; kk++)
                    {
                        float sx, sy;
                        cam_project(pw4[kk], &sx, &sy);
                        s[kk].x = sx; s[kk].y = sy; s[kk].z = 0.0f;
                    }
                    corner_affine_rv(&s[0], &s[1], &s[2], rv);
                }
                else
                {
                    for (int kk = 0; kk < 4; kk++)
                    {
                        float sx, sy;
                        cam_project(pw4[kk], &sx, &sy);
                        rv[kk].x = sx; rv[kk].y = sy; rv[kk].z = 0.0f;
                    }
                }
                if (quad_area(rv) < 4.0f) { this->nz[d][q] = 0.0f; }
                else if (q >= N_FACE + N_EDGE) { face_set_matrix(obj, &this->draw_img[d][q], rv); }
                else { face_set_matrix_inset(obj, &this->draw_img[d][q], rv, EDGE_INSET_PX); }
            }

            float nr[3]  = { nw[0], -nw[1], nw[2] };
            float cr[3]  = { fc[0], -fc[1], fc[2] };
            float vvr[3] = { cr[0], cr[1] - CAM_H, cr[2] + CAM_BACK };
            float facing_r = nr[0] * vvr[0] + nr[1] * vvr[1] + nr[2] * vvr[2];
            this->nz_refl[d][q] = -facing_r;
            if (facing_r < 0.0f)
            {
                float hfade = 1.0f - g_dice[d].pos[2] * REFL_H_FADE;
                if (hfade <= 0.0f)
                {
                    this->nz_refl[d][q] = 0.0f;
                }
                else
                {
                    float brn = (q < N_FACE) ? face_brightness(nr) : edge_brightness(nr);
                    float bright = brn * REFL_DIM * hfade;
                    this->draw_img_refl[d][q].opacity_value = (uint8_t)(bright * 255.0f);

                    gui_vertex_t rv[4];
                    if (q >= N_FACE + N_EDGE)
                    {
                        gui_vertex_t s[3];
                        for (int kk = 0; kk < 3; kk++)
                        {
                            float pwr[3] = { pw4[kk][0], -pw4[kk][1], pw4[kk][2] };
                            float sx, sy;
                            cam_project(pwr, &sx, &sy);
                            s[kk].x = sx; s[kk].y = sy; s[kk].z = 0.0f;
                        }
                        corner_affine_rv(&s[0], &s[1], &s[2], rv);
                    }
                    else
                    {
                        for (int kk = 0; kk < 4; kk++)
                        {
                            float pwr[3] = { pw4[kk][0], -pw4[kk][1], pw4[kk][2] };
                            float sx, sy;
                            cam_project(pwr, &sx, &sy);
                            rv[kk].x = sx; rv[kk].y = sy; rv[kk].z = 0.0f;
                        }
                        /* 反射同样用纹理内缩:不透明体"相接"不"重叠",半透明 src-over 不再累加白线。 */
                    }
                    if (quad_area(rv) < 4.0f) { this->nz_refl[d][q] = 0.0f; }
                    else if (q >= N_FACE + N_EDGE) { face_set_matrix(obj, &this->draw_img_refl[d][q], rv); }
                    else { face_set_matrix_inset(obj, &this->draw_img_refl[d][q], rv, EDGE_INSET_PX); }
                }
            }
        }

    }
}

static void dice2d5_timer_cb(void *param)
{
    (void)param;
    dice_handle_input();
    physics_step(g_dice, DICE_NUM);
    gui_fb_change();
}

static void dice2d5_draw(gui_obj_t *obj)
{
    gui_dice_t *this = (gui_dice_t *)obj;
    gui_dispdev_t *dc = gui_get_dc();

    /* 木纹背景已由独立的 gui_img 子控件(在本对象之前创建)绘制,这里只画骰子。 */

    int order[DICE_NUM];
    for (int i = 0; i < DICE_NUM; i++) { order[i] = i; }
    for (int i = 0; i < DICE_NUM - 1; i++)
        for (int j = i + 1; j < DICE_NUM; j++)
            if (g_dice[order[j]].pos[1] > g_dice[order[i]].pos[1])
            {
                int t = order[i]; order[i] = order[j]; order[j] = t;
            }

    for (int k = 0; k < DICE_NUM; k++)
    {
        int d = order[k];
        /* 倒影层:先于本体画,只画朝向镜像相机的 26 块 */
        for (int q = 0; q < N_QUAD; q++)
        {
            if (this->nz_refl[d][q] > 0.0f)
            {
                gui_acc_blit_to_dc(&this->draw_img_refl[d][q], dc, NULL);
            }
        }
    }

    for (int k = 0; k < DICE_NUM; k++)
    {
        int d = order[k];
        /* 本体层:只画背面剔除后可见(nz>0)的 26 块 */
        for (int q = 0; q < N_QUAD; q++)
        {
            if (this->nz[d][q] > 0.0f)
            {
                gui_acc_blit_to_dc(&this->draw_img[d][q], dc, NULL);
            }
        }
    }
}

static void dice2d5_cb(gui_obj_t *obj, T_OBJ_CB_TYPE cb_type)
{
    if (obj == NULL) { return; }
    switch (cb_type)
    {
    case OBJ_PREPARE: dice2d5_prepare(obj); break;
    case OBJ_DRAW:    dice2d5_draw(obj);    break;
    default: break;
    }
}

/* 点数面贴图:对面之和为 7(前1后6 / 上2下5 / 左3右4) */
static const void *s_face_img[6] =
{
    (const void *)"/image/dice/Face01.bin", (const void *)"/image/dice/Face06.bin", (const void *)"/image/dice/Face02.bin",
    (const void *)"/image/dice/Face05.bin", (const void *)"/image/dice/Face03.bin", (const void *)"/image/dice/Face04.bin",
};

static gui_dice_t *dice2d5_create(gui_obj_t *parent)
{
    gui_dice_t *this = gui_malloc(sizeof(gui_dice_t));
    GUI_ASSERT(this != NULL);
    memset(this, 0, sizeof(gui_dice_t));

    gui_obj_t *base = (gui_obj_t *)this;
    gui_obj_ctor(base, parent, "dice-2d5-litegfx", 0, 0, 0, 0);
    base->type = VG_LITE_SOCCER;   /* 复用 2.5D 贴图类型 */
    base->obj_cb = dice2d5_cb;
    base->has_prepare_cb = true;
    base->has_draw_cb = true;

    for (int d = 0; d < DICE_NUM; d++)
    {
        for (int q = 0; q < N_QUAD; q++)
        {
            /* q<6 点数主面(64x64);6..17 棱面(Side 64x12);18..25 角面(Angle 16x16) */
            const void *tex; int tw, th;
            // int is_corner = (q >= N_FACE + N_EDGE);
            if (q < N_FACE)              { tex = s_face_img[q];        tw = th = DICE_IMG_WH; }
            else if (q < N_FACE + N_EDGE) { tex = (const void *)"/image/dice/Side.bin";  tw = SIDE_W;  th = SIDE_H;  }
            else                          { tex = (const void *)"/image/dice/Angle.bin"; tw = ANGLE_W; th = ANGLE_H; }

            const void *img_data = gui_vfs_get_file_address(tex);
            if (!img_data)
            {
                /* Fallback: read file into memory */
                gui_vfs_file_t *f = gui_vfs_open(tex, GUI_VFS_READ);
                GUI_ASSERT(f != NULL);
                gui_vfs_seek(f, 0, GUI_VFS_SEEK_END);
                int size = gui_vfs_tell(f);

                if (size <= 0)
                {
                    gui_vfs_close(f);
                    return;
                }
                gui_vfs_seek(f, 0, GUI_VFS_SEEK_SET);
                img_data = gui_malloc(size);
                GUI_ASSERT(img_data != NULL);
                gui_vfs_read(f, (void *)img_data, size);
                gui_vfs_close(f);
                gui_log("dice2d5_create: loaded %s (%d bytes) into memory\n", (const char *)tex, size);

            }



            this->draw_img[d][q].data  = (void *)img_data;
            this->draw_img[d][q].img_w = tw;
            this->draw_img[d][q].img_h = th;
            this->draw_img[d][q].opacity_value = UINT8_MAX;
#ifdef _HONEYGUI_SIMULATOR_
            this->draw_img[d][q].blend_mode = IMG_SRC_OVER_MODE;
#else
            this->draw_img[d][q].blend_mode = IMG_SRC_MODE;
#endif
            this->draw_img[d][q].high_quality = true;

            this->draw_img_refl[d][q].data  = (void *)img_data;
            this->draw_img_refl[d][q].img_w = tw;
            this->draw_img_refl[d][q].img_h = th;
            this->draw_img_refl[d][q].opacity_value = 0;
            this->draw_img_refl[d][q].blend_mode = IMG_SRC_OVER_MODE;

            gui_log("data 0x%x 0x%x\n", this->draw_img[d][q].data, this->draw_img_refl[d][q].data);
        }
    }

    gui_list_init(&(base->child_list));
    if (base->parent != NULL)
    {
        gui_list_insert_before(&(base->parent->child_list), &(base->brother_list));
    }
    base->create_done = true;
    return this;
}

int dice_demo(gui_obj_t *parent)
{
    dice_physics_init();
    geometry_build();
    view_build();

    gui_obj_t *root = parent;
    gui_img_t *bg = gui_img_create_from_fs(root, "dice-bg", "/image/dice/floor_bg_360.bin",
                                            0, 0, 0, 0);
    gui_img_set_mode(bg, IMG_BYPASS_MODE);

    gui_dice_t *dice = dice2d5_create(root);

    gui_obj_create_timer(GUI_BASE(dice), 16, true, dice2d5_timer_cb);
    gui_obj_start_timer(GUI_BASE(dice));
    return 0;
}


