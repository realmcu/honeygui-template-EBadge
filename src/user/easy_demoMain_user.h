#ifndef EASY_DEMOMAIN_USER_H
#define EASY_DEMOMAIN_USER_H

#include "../callbacks/easy_demoMain_callbacks.h"
#include "../ui/easy_demoMain_ui.h"

/**
 * User-defined header file
 * This file is generated once only, feel free to modify
 */

// Add custom declarations here
#define  SCREEN_H  360
#define  MAINFACE_NUM  10

uint8_t mainface_idx;
uint8_t mainface_num;
void *mainface_list[MAINFACE_NUM];

int16_t y_rec;

bool is_auto_sleep_mode;
bool is_delete_mode;

uint8_t soc_val;
uint8_t screen_light_idx;

void switch_mainface(void);
void mainface_list_delete(void);

void click_auto_sleep_icon(void *obj, gui_event_t *e);
void click_screen_light_icon(void *obj, gui_event_t *e);
void click_delete_icon(void *obj, gui_event_t *e);
void click_delete_icon_detail(void *obj, gui_event_t *e);
void click_back_icon(void *obj, gui_event_t *e);

#endif // EASY_DEMOMAIN_USER_H
