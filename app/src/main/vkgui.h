#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <volk.h>

bool app_vkgui_init();
bool app_vkgui_event(SDL_Event* p_ev);
bool app_vkgui_begin();
bool app_vkgui_end(VkCommandBuffer* p_cb);
bool app_vkgui_destroy();

#ifdef __cplusplus
}
#endif  // __cplusplus
