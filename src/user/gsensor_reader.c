#include <stdbool.h>
#include <stdint.h>

#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
#include "posix.h"
#include "ioctls/posix_ioctl_gsensor.h"

bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z)
{
    static posix_fd_t s_fd = POSIX_FD_NULL;

    if (s_fd == POSIX_FD_NULL) {
        s_fd = posix_open("/dev/gsensor0");
        if (s_fd == POSIX_FD_NULL) { return false; }
    }

    posix_gsensor_axis_t axis;
    posix_ssize_t        n = posix_read(s_fd, &axis, sizeof(axis));
    if (n < 0) { return false; }

    if (x) { *x = (int16_t)axis.x; }
    if (y) { *y = (int16_t)axis.y; }
    if (z) { *z = (int16_t)axis.z; }
    return true;
}
#endif



