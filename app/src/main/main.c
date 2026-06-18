#define SDL_MAIN_USE_CALLBACKS 1  // MUST be before headers
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <log.h>

#ifdef APP_USE_VK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#include "vkgui.h"
#endif  // APP_USE_VK

#include "global.h"
#include "systems.h"

SDL_AppResult SDL_AppInit(void** pp_as, int argc, char* argv[]) {
  app_init_logger();

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
  // DESKTOP_ONLY({ wdowflags |= SDL_WINDOW_RESIZABLE; })

#ifdef APP_USE_VK
  app_vkinit_device();
  wdowflags |= SDL_WINDOW_VULKAN;
#endif  // APP_USE_VK

  ANDROID_ONLY({
    wdowflags |= SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_FULLSCREEN;

    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
  })

  g_pmainwdow = SDL_CreateWindow(APP_NAME, APP_MAIN_WINDOW_INIT_WIDTH, APP_MAIN_WINDOW_INIT_HEIGHT,
                                 wdowflags);
  if (g_pmainwdow == NULL) {
    log_error("[app] SDL_CreateWindow: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

#ifdef APP_USE_VK
  app_chk(app_vkinit_allocator(), "app_vkinit_allocator");
  app_chk(app_vkinit_surface(), "app_vkinit_surface");
  app_chk(app_vkinit_swapchain(), "app_vkinit_swapchain");
  app_chk(app_vkinit_depthimg(), "app_vkinit_depthimg");
  app_chk(app_vkinit_syncobj(), "app_vkinit_syncobj");
  app_chk(app_vkinit_cmdpool(), "app_vkinit_cmdpool");

  app_chk(app_vkinit_descriptor_layout(), "app_vkinit_descriptor_pool");

  app_chk(app_vkinit_shader(), "app_vkinit_shader");
  app_chk(app_vkinit_pipeline(), "app_vkinit_pipeline");

  app_chk(app_vkinit_static_render_data(), "app_vkinit_render_data");
  app_chk(app_vkinit_mesh_assets(), "app_vkinit_mesh_assets");
  app_chk(app_vkinit_tex_assets(), "app_vkinit_tex_assets");
  app_chk(app_vkgui_init(), "app_vkinitgui");
#endif  //APP_USE_VK

  g_appctx = (app_guistate_t){
      .clrscr = {APP_UTIL_COLOR_SRGB(0.12156862745098039215f, 0.12156862745098039215f,
                                     0.12156862745098039215f)},
  };

  app_chk(app_guisetup(&g_appctx), "app_guisetup");
  app_chk(app_gameinit(), "app_gameinit");

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

    case SDL_EVENT_WINDOW_RESIZED: {

#ifdef APP_USE_VK
      g_vkupdate_sc = true;
#endif  // APP_USE_VK

      break;
    }

      // case SDL_EVENT_WINDOW_MINIMIZED:
      // case SDL_EVENT_WINDOW_OCCLUDED:
      // case SDL_EVENT_WINDOW_HIDDEN:
      //   // g_app_renderable = false;
      //   break;
      //
      // case SDL_EVENT_WINDOW_EXPOSED:
      // case SDL_EVENT_WINDOW_SHOWN:
      // case SDL_EVENT_WINDOW_RESTORED:
      //   // g_app_renderable = true;
      //   break;

    default:
      break;
  }

#ifdef APP_USE_VK

  if (g_vkupdate_sc)
    app_chk(app_vkupdate_swapchain(), "app_vkupdate_swapchain");

  if (!app_vkhandle_event(p_ev)) {
    log_error("[app] app_vkhandle_event");
    return SDL_APP_FAILURE;
  }
#endif  //APP_USE_VK

  if (!app_guiwant_capture_mouse()) {
    app_gameev_camera_control(p_ev);
    app_gameev_unit_selection(p_ev);
    app_gameev_unit_action(p_ev);
  } else {
    log_debug("[app] %s: hover imgui", __FUNCTION__);
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* p_as) {
  // if (!g_app_renderable) {
  //   SDL_Delay(16);
  //   return SDL_APP_CONTINUE;
  // }

#ifdef APP_USE_VK
  app_vkinit_frame_render_data();
  app_vkbegin_render();
#endif  //APP_USE_VK

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* p_as, SDL_AppResult result) {
  log_debug("[app] SDL_AppQuit: %d", (int)result);

#ifdef APP_USE_VK
  app_chk(app_vkdestroy(), "app_vkdestroy");
#endif  //APP_USE_VK

  if (g_pmainwdow) {
    SDL_DestroyWindow(g_pmainwdow);
  }
  SDL_Quit();
}
