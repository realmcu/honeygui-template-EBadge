#include "bt_callbacks.h"
#include "../ui/bt_ui.h"
#include "../user/bt_user.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Timer animation counters
uint16_t img_2_timer_cnt = 0;

// Event callback function implementations

// Preset timer callback functions

/**
 * 动画 1
 * Component: img_2
 * Mode: Preset actions (multi-segment animation)
 * Segments: 2
 */
void img_2_timer_0_cb(void *obj)
{
    gui_obj_t *target = (gui_obj_t *)obj;
    const uint16_t total_cnt_max = 100;
    
    const uint16_t seg0_start = 0;
    const uint16_t seg0_end = 50;
    const uint16_t seg1_start = 50;
    const uint16_t seg1_end = 100;
    
    img_2_timer_cnt++;
    
    // Segment 1: 500ms, 1 action(s)
    if (img_2_timer_cnt > seg0_start && img_2_timer_cnt <= seg0_end) {
        uint16_t seg_cnt = img_2_timer_cnt - seg0_start;
        const uint16_t seg_cnt_max = seg0_end - seg0_start;
        
            // Adjust scale: (1, 1) -> (0.5, 0.5)
            const float zoom_x_origin = 1;
            const float zoom_x_target = 0.5;
            const float zoom_y_origin = 1;
            const float zoom_y_target = 0.5;
            float zoom_x_cur = zoom_x_origin + (zoom_x_target - zoom_x_origin) * seg_cnt / seg_cnt_max;
            float zoom_y_cur = zoom_y_origin + (zoom_y_target - zoom_y_origin) * seg_cnt / seg_cnt_max;
            gui_img_scale((gui_img_t *)target, zoom_x_cur, zoom_y_cur);
            
    }
    // Segment 2: 500ms, 1 action(s)
    else if (img_2_timer_cnt > seg1_start && img_2_timer_cnt <= seg1_end) {
        uint16_t seg_cnt = img_2_timer_cnt - seg1_start;
        const uint16_t seg_cnt_max = seg1_end - seg1_start;
        
            // Adjust scale: (0.5, 0.5) -> (1, 1)
            const float zoom_x_origin = 0.5;
            const float zoom_x_target = 1;
            const float zoom_y_origin = 0.5;
            const float zoom_y_target = 1;
            float zoom_x_cur = zoom_x_origin + (zoom_x_target - zoom_x_origin) * seg_cnt / seg_cnt_max;
            float zoom_y_cur = zoom_y_origin + (zoom_y_target - zoom_y_origin) * seg_cnt / seg_cnt_max;
            gui_img_scale((gui_img_t *)target, zoom_x_cur, zoom_y_cur);
            
    }
    
    if (img_2_timer_cnt >= total_cnt_max) {
        img_2_timer_cnt = 0; // Reset counter, continue loop
    }
}


/* @protected start custom_functions */
// Custom functions
/* @protected end custom_functions */
