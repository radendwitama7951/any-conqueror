#define SDL_MAIN_USE_CALLBACKS 1  // MUST be before headers
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <log.h>

#ifdef APP_USE_VK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif  // APP_USE_VK

#include "global.h"
#include "systems.h"

SDL_AppResult SDL_AppInit(void** pp_as, int argc, char* argv[]) {
  log_debug("[app] Hello world from %s!", APP_NAME);

  if (!SDL_SetAppMetadata("Project Any Conqueror", "1.0", "id.my.radengan.app")) {
    log_error("[app] SDL_SetAppMetadata: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    log_error("[app] SDL_Init: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_WindowFlags wdowflags = 0;

#ifdef APP_USE_VK
  app_vkinit_device();
  wdowflags |= SDL_WINDOW_VULKAN;
#endif  // APP_USE_VK

  g_pmainwdow =
      SDL_CreateWindow(APP_NAME, APP_MAIN_WINDOW_WIDTH, APP_MAIN_WINDOW_HEIGHT, wdowflags);
  if (g_pmainwdow == NULL) {
    log_error("[app] SDL_CreateWindow: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

#ifdef APP_USE_VK
  app_vkinit_surface();
  app_vkinit_swapchain();
  app_vkinit_depthimg();
  app_vkinit_mesh_assets();
  app_vkinit_syncobj();
  app_vkinit_cmdpool();
  app_vkinit_tex_assets();

  app_vkinit_shader();

  app_vkinit_pipeline();
#endif  //APP_USE_VK

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* p_as, SDL_Event* p_ev) {
  switch (p_ev->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN:
      if (p_ev->key.scancode == SDL_SCANCODE_ESCAPE ||  //
          p_ev->key.scancode == SDL_SCANCODE_AC_BACK) {
        return SDL_APP_SUCCESS;
      }

    default:
      break;
  }

#ifdef APP_USE_VK
  if (!app_vkhandle_event(p_ev)) {
    log_error("[app] app_vkhandle_event");
    return SDL_APP_FAILURE;
  }
#endif  //APP_USE_VK

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* p_as) {

#ifdef APP_USE_VK
  app_vkbegin_render();
#endif  //APP_USE_VK

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* p_as, SDL_AppResult result) {
  log_debug("[app] SDL_AppQuit: %d", (int)result);

#ifdef APP_USE_VK
  app_vkdestroy();
#endif  //APP_USE_VK

  if (g_pmainwdow) {
    SDL_DestroyWindow(g_pmainwdow);
  }
  SDL_Quit();
}
