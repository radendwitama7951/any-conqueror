
#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#define APP_USE_VK

#ifdef APP_USE_VK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#include "systems/app_vk_system.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#endif  // APP_USE_VK

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "core/core.hpp"
#include "global.hpp"

SDL_AppResult SDL_AppInit(void** _pp_as, int argc, char* argv[]) {
  LOGD("[app] Hello world from {}!", APP_NAME);

  if (!SDL_SetAppMetadata("Project Any Conqueror", "1.0", "id.my.radengan.app")) {
    LOGE("[app] SDL_SetAppMetadata: {}", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LOGE("[app] SDL_Init: {}", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_WindowFlags wdowflags = 0;

#ifdef APP_USE_VK
  app_vk::init_device(&g_appstate);
  wdowflags |= SDL_WINDOW_VULKAN;
#endif  // APP_USE_VK

  g_appstate.p_wdow = SDL_CreateWindow(APP_NAME, 1280u, 720u, wdowflags);
  if (g_appstate.p_wdow == NULL) {
    LOGE("[app] SDL_CreateWindow: {}", SDL_GetError());
    return SDL_APP_FAILURE;
  }

#ifdef APP_USE_VK
  app_vk::init_surface(&g_appstate);
  app_vk::init_swapchain(&g_appstate);
  app_vk::init_depthimg(&g_appstate);
  app_vk::init_mesh_assets(&g_appstate);
  app_vk::init_syncobj(&g_appstate);
  app_vk::init_cmdpool(&g_appstate);
  app_vk::init_tex_assets(&g_appstate);
  app_vk::init_shader(&g_appstate);
#endif  //APP_USE_VK

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* _p_as, SDL_Event* p_ev) {
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
  if (!app_vk::handle_event(&g_appstate, p_ev)) {
    LOGE("[app] app_vk::handle_event");
    return SDL_APP_FAILURE;
  }
#endif  //APP_USE_VK

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* _p_as) {

#ifdef APP_USE_VK
  app_vk::begin_render(&g_appstate);
#endif  //APP_USE_VK

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* _p_as, SDL_AppResult result) {
  LOGD("[app] SDL_AppQuit: {}", (int)result);

#ifdef APP_USE_VK
  app_vk::destroy(&g_appstate);
#endif  //APP_USE_VK
}
