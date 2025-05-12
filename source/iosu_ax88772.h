#pragma once

#include <wafel/types.h>

typedef int rw_func(void *context, void *param_2, int cmd, int index, int value, u32 size, void *buf);

#define ax8817xReadCommand  ((rw_func*)0x123b9ab4)
#define ax8817xWriteCommand ((rw_func*)0x123b98ac)