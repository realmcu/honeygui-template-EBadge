#include "gui_api.h"
#include "gui_view.h"
#include "gui_components_init.h"
#include "gui_vfs.h"

/* @protected start entry_includes */
// Add user includes here
#include "gui_win.h"
#include "os_sched.h"
/* @protected end entry_includes */

static int app_init(void)
{
#ifdef _HONEYGUI_SIMULATOR_
    // Simulator: Mount POSIX filesystem
    gui_vfs_mount_posix("/", "./assets");
#else
    // SOC: Mount romfs from flash address
    gui_vfs_mount_romfs("/", (void *)0x0240f400, 0);
#endif

    /* @protected start app_init_pre */
    // Add user initialization code here (runs before the main view is created)
#ifndef _HONEYGUI_SIMULATOR_
    extern int flashdb_prepare(void);
    flashdb_prepare();

    extern int app_stream_transport_init(void);
    app_stream_transport_init();

    // extern bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z);
    // uint8_t i = 0;
    // while(1)
    // { 
    //     int16_t ax = 0, ay = 0, az = 0;
    //     if (gsensor_sc7a20_read_xyz(&ax, &ay, &az))
    //     {
    //         gui_log("[sc7a20] #%d xyz raw: x=%d y=%d z=%d", i, ax, ay, az);
    //     }
    //     os_delay(200);

    // }
#endif

    extern gui_win_t *win_view;
    if (win_view == NULL)
    {
        win_view = gui_win_create(gui_obj_get_root(), "win_view", 0, 0, 360, 360);
    }
    /* @protected end app_init_pre */

    gui_view_create(gui_obj_get_root(), "easy_demoMainView", 0, 0, 0, 0);

    /* @protected start app_init_post */
    // Add user initialization code here (runs after the main view is created)
    gui_obj_tree_free(GUI_BASE(gui_view_get_current()));
    gui_view_create(win_view, "easy_demoMainView", 0, 0, 0, 0);
    /* @protected end app_init_post */

    return 0;
}

// GUI_INIT_APP_EXPORT(app_init);
