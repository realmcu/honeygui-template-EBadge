/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GSENSOR_READER_H
#define GSENSOR_READER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read one raw three-axis sample from the SC7A20 accelerometer.
 *
 * @param x Optional output for the X-axis sample.
 * @param y Optional output for the Y-axis sample.
 * @param z Optional output for the Z-axis sample.
 * @return true when a sample was read successfully; otherwise false.
 */
bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z);

#ifdef __cplusplus
}
#endif

#endif /* GSENSOR_READER_H */
