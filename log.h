#ifndef ZYGOTE_HOOK_LOG_H
#define ZYGOTE_HOOK_LOG_H

#include <utils/Log.h>

#ifdef ENABLE_DEBUG
#define DEBUG(format, ...)  ALOGD("ZYGOTEHOOK:" format, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#endif

