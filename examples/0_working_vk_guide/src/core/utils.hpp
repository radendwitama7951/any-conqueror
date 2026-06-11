#pragma once

#if defined(__ANDROID__)

#define ANDROID_ONLY(call) \
  do {                     \
    call;                  \
  } while (0)
#define DESKTOP_ONLY(call) \
  do {                     \
  } while (0)
#define EMSCRIPTEN_ONLY(call) \
  do {                        \
  } while (0)

#elif defined(__EMSCRIPTEN__)

#define ANDROID_ONLY(call) \
  do {                     \
  } while (0)
#define DESKTOP_ONLY(call) \
  do {                     \
  } while (0)
#define EMSCRIPTEN_ONLY(call) \
  do {                        \
    call;                     \
  } while (0)

#else  // Desktop (Windows, macOS, Linux, etc.)

#define ANDROID_ONLY(call) \
  do {                     \
  } while (0)
#define DESKTOP_ONLY(call) \
  do {                     \
    call;                  \
  } while (0)
#define EMSCRIPTEN_ONLY(call) \
  do {                        \
  } while (0)

#endif
