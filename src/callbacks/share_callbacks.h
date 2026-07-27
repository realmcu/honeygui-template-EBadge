#ifndef SHARE_CALLBACKS_H
#define SHARE_CALLBACKS_H

#include "gui_api.h"
#include "gui_text.h"
#include "gui_obj_focus.h"

// Timer animation counters (defined in callbacks.c)
extern uint16_t circle_anime_timer_cnt;
extern uint16_t lbl_share_timer_cnt;
extern uint16_t img_8_timer_cnt;

// Event callback function declarations
void SelectDevView_key_0_cb(void *obj, gui_event_t *e);
void ShareConnView_key_0_cb(void *obj, gui_event_t *e);
void img_4_clicked_cb(void *obj, gui_event_t *e);
void img_5_clicked_cb(void *obj, gui_event_t *e);
void shareMainView_key_0_cb(void *obj, gui_event_t *e);

// User-configured timer callback function declarations
void circle_anime_timer_0_cb(void *obj);
void lbl_share_timer_0_cb(void *obj);
void img_8_timer_0_cb(void *obj);

// Custom function declarations (auto-extracted from callbacks.c protected area)
void lbl_share_timer_0_cb(void *obj);
void win_share_timer_0_cb(void *obj);
uint8_t hmi_ble_central_get_dev_count(void);
bool hmi_ble_central_get_dev(uint8_t idx, uint8_t bd_addr[6], uint8_t *addr_type,
                                             int8_t *rssi, char *name, uint8_t name_len);
bool hmi_ble_central_get_dev(uint8_t idx, uint8_t bd_addr[6], uint8_t *addr_type,
                             int8_t *rssi, char *name, uint8_t name_len);

#endif // SHARE_CALLBACKS_H
