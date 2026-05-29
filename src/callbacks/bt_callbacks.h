#ifndef BT_CALLBACKS_H
#define BT_CALLBACKS_H

#include "gui_api.h"
#include "gui_text.h"
#include "gui_obj_focus.h"

// Timer animation counters (defined in callbacks.c)
extern uint16_t img_2_timer_cnt;

// Event callback function declarations

// User-configured timer callback function declarations
void img_2_timer_0_cb(void *obj);

#endif // BT_CALLBACKS_H
