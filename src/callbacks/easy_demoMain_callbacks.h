#ifndef EASY_DEMOMAIN_CALLBACKS_H
#define EASY_DEMOMAIN_CALLBACKS_H

#include "gui_api.h"
#include "gui_text.h"
#include "gui_obj_focus.h"

// Timer animation counters (defined in callbacks.c)
extern uint16_t easy_demoMainView_timer_cnt;
extern uint16_t icon_bat_timer_cnt;
extern uint16_t lbl_1_timer_cnt;

// Event callback function declarations
void bg_circle_key_cb(void *obj, gui_event_t *e);
void easy_demoMainView_key_0_cb(void *obj, gui_event_t *e);
void easy_demoMainView_key_1_cb(void *obj, gui_event_t *e);
void icon_as_clicked_cb(void *obj, gui_event_t *e);
void icon_del_clicked_cb(void *obj, gui_event_t *e);
void icon_fl_clicked_cb(void *obj, gui_event_t *e);
void icon_sl_clicked_cb(void *obj, gui_event_t *e);
void img_3_key_cb(void *obj, gui_event_t *e);

// User-configured timer callback function declarations
void easy_demoMainView_timer_0_cb(void *obj);
void easy_demoMainView_timer_init_cb(void *obj);
void icon_bat_timer_0_cb(void *obj);
void lbl_1_timer_0_cb(void *obj);

// Custom function declarations (auto-extracted from callbacks.c protected area)
void easy_demoMainView_timer_init_cb(void *obj);
void easy_demoMainView_timer_0_cb(void *obj);
void lbl_1_timer_0_cb(void *obj);
void icon_bat_timer_0_cb(void *obj);

#endif // EASY_DEMOMAIN_CALLBACKS_H
