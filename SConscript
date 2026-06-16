#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
#

Import('SDK_ROOT')
from building import *

# get current directory
cwd = GetCurrentDir()

src = []

CPPDEFINES = []
LINKFLAGS = []

path = ['src/callbacks',
        'src/ui',
        'src/user' ]


src += ['src/easy_demoEntry.c']

src += Glob('src/callbacks/*.c')
src += Glob('src/user/*.c')
src += Glob('src/ui/*.c')


#SrcRemove(src,SDK_ROOT + '/src/mcu/driver/imdc/src/rtl_common/rtl_lcdc_dphy.c')


group = DefineGroup('ui', src, depend = [''], CPPPATH = path, CPPDEFINES = CPPDEFINES, LINKFLAGS = LINKFLAGS)


Return('group')
