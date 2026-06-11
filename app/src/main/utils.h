#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "log.h"

#ifdef __ANDROID__
#define ANDROID_ONLY(call) \
  do {                     \
    call;                  \
  } while (0);

#define DESKTOP_ONLY(call) \
  do {                     \
  } while (0);
#else
#define ANDROID_ONLY(call) \
  do {                     \
  } while (0);

#define DESKTOP_ONLY(call) \
  do {                     \
    call;                  \
  } while (0);
#endif /* ifdef __ANDROID__ */

#ifdef __ANDROID__
#include <android/log.h>

static void android_log_callback(log_Event* ev) {
  int android_pri;

  switch (ev->level) {
    case LOG_TRACE:
      android_pri = ANDROID_LOG_VERBOSE;
      break;
    case LOG_DEBUG:
      android_pri = ANDROID_LOG_DEBUG;
      break;
    case LOG_INFO:
      android_pri = ANDROID_LOG_INFO;
      break;
    case LOG_WARN:
      android_pri = ANDROID_LOG_WARN;
      break;
    case LOG_ERROR:
      android_pri = ANDROID_LOG_ERROR;
      break;
    case LOG_FATAL:
      android_pri = ANDROID_LOG_FATAL;
      break;
    default:
      android_pri = ANDROID_LOG_DEFAULT;
      break;
  }

  __android_log_vprint(android_pri, APP_NAME, ev->fmt, ev->ap);
}
#endif /* ifdef __ANDROID__ */

#define APP_GET_ARRAY_SIZE(arr) ((sizeof(arr) / sizeof((arr)[0])))

static inline void app_init_logger() {
#ifdef NDEBUG
  static const uint8_t LOG_LV = LOG_INFO;
#else
  static const uint8_t LOG_LV = LOG_DEBUG;
#endif /* ifdef NDEBUG */

  log_debug("Hello world from %s!", APP_NAME);
  log_set_level(LOG_LV);
  ANDROID_ONLY(log_add_callback(android_log_callback, NULL, LOG_LV));
}

#if !defined(APP_CHK_FATAL)
#define APP_CHK_FATAL 1
#endif  // !APP_CHK_FATAL

#define app_chk(cond, desc)                   \
  do {                                        \
    if (!(cond)) {                            \
      log_error("[app] app_chk: %s", (desc)); \
      if (APP_CHK_FATAL)                      \
        exit(1);                              \
    }                                         \
  } while (0)

#define app_vkchk(cond, desc)                                        \
  do {                                                               \
    if ((cond) != VK_SUCCESS) {                                      \
      log_error("[app] app_vkchk %d: %s", (uint32_t)(cond), (desc)); \
      if (APP_CHK_FATAL)                                             \
        exit(1);                                                     \
    }                                                                \
  } while (0)

#define app_vkchk_sc(cond, update_flag)                                         \
  do {                                                                          \
    /* This will fail to compile if update_flag is not a pointer */             \
    _Generic((update_flag), bool*: (void)0, int*: (void)0, default: ((void)0)); \
                                                                                \
    VkResult res = (cond);                                                      \
    if (res < VK_SUCCESS) {                                                     \
      if (res == VK_ERROR_OUT_OF_DATE_KHR) {                                    \
        *(update_flag) = true; /* Dereference to update */                      \
        break;                                                                  \
      }                                                                         \
      log_error("[app] app_vkchk_sc: %d", (int)res);                            \
      if (APP_CHK_FATAL)                                                        \
        exit(1);                                                                \
    }                                                                           \
  } while (0)

#define APP_UTIL_COLOR_SRGBA(r, g, b, a) pow((r), 2.2f), pow((g), 2.2f), pow((b), 2.2f), a

#define APP_UTIL_CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

#define APP_UTIL_ARRAY_LEN(arr) (sizeof((arr)) / sizeof((arr)[0]))
