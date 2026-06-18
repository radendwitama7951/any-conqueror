
#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdbool.h>

typedef struct {
  float clrscr[3];
} app_guistate_t;

/*
 * gui_entry
 * */
bool app_guisetup(app_guistate_t* p_ctx);
bool app_guimain(app_guistate_t* p_ctx);

/*
 * IMGUI WRAPPER
 *
 * */

bool app_guiwant_capture_mouse();

#ifdef __cplusplus
}
#endif  // __cplusplus
