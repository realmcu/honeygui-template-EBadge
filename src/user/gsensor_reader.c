#include "gsensor_reader.h"

#ifndef _HONEYGUI_SIMULATOR_
#include "ioctls/posix_ioctl_gsensor.h"
#include "posix.h"
#endif

bool gsensor_sc7a20_read_xyz(int16_t *x, int16_t *y, int16_t *z)
{
#ifdef _HONEYGUI_SIMULATOR_
    (void)x;
    (void)y;
    (void)z;
    return false;
#else
    static posix_fd_t fd = POSIX_FD_NULL;

    if (fd == POSIX_FD_NULL)
    {
        fd = posix_open("/dev/gsensor0");
        if (fd == POSIX_FD_NULL)
        {
            return false;
        }
    }

    posix_gsensor_axis_t axis;
    posix_ssize_t bytes_read = posix_read(fd, &axis, sizeof(axis));
    if (bytes_read < 0)
    {
        return false;
    }

    if (x != NULL)
    {
        *x = (int16_t)axis.x;
    }
    if (y != NULL)
    {
        *y = (int16_t)axis.y;
    }
    if (z != NULL)
    {
        *z = (int16_t)axis.z;
    }

    return true;
#endif
}
