
#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdbool.h>
#include "app_ctx.h"

/*
 * gui_entry
 * */
bool app_guisetup(app_ctx_t* p_ctx);
bool app_guimain(app_ctx_t* p_ctx);

/*
 * IMGUI WRAPPER
 *
 * */

bool app_guiwant_capture_mouse();

#ifdef __cplusplus
}
#endif  // __cplusplus
