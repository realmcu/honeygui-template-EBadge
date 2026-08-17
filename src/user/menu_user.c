#include "menu_user.h"

/**
 * User-defined settings menu implementation.
 *
 * This file is generated only once and can be modified freely.
 */

/*============================================================================*
 * Public callbacks
 *============================================================================*/

uint8_t menu_idx = 0;

/* Open the confirmation view for the reset action. */
void click_menu_reset_button(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    menu_idx = 0;
    gui_view_switch_direct(gui_view_get_current(), "menuDetailView",
                           SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
}

/* Open the confirmation view for the storage format action. */
void click_menu_fs_button(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    menu_idx = 1;
    gui_view_switch_direct(gui_view_get_current(), "menuDetailView",
                           SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
}

/* Open the confirmation view for the power-off action. */
void click_menu_power_off_button(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    menu_idx = 2;
    gui_view_switch_direct(gui_view_get_current(), "menuDetailView",
                           SWITCH_INIT_STATE, SWITCH_IN_NONE_ANIMATION);
}

/* Execute the action selected before entering the confirmation view. */
void click_menu_yes_button(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    switch (menu_idx)
    {
    case 0:
        /* TODO: Reset the device. */
        gui_log("click_menu_yes_button: reset\n");
        break;
    case 1:
        /* TODO: Format user storage. */
        gui_log("click_menu_yes_button: format storage\n");
        break;
    case 2:
        /* TODO: Power off the device. */
        gui_log("click_menu_yes_button: power off\n");
        break;
    default:
        break;
    }
}
