#ifndef SHARE_USER_H
#define SHARE_USER_H

#include "../callbacks/share_callbacks.h"
#include "../ui/share_ui.h"

/**
 * User-defined header file
 * This file is generated once only, feel free to modify
 */

// Add custom declarations here
#include "easy_demoMain_user.h"

void click_share_image_button(void *obj, gui_event_t *e);
void click_receive_image_button(void *obj, gui_event_t *e);

void switch_in_share_view(gui_view_t *view);
void switch_out_share_view(gui_view_t *view);
void switch_in_select_dev_view(gui_view_t *view);
void switch_out_select_dev_view(gui_view_t *view);





#endif // SHARE_USER_H
