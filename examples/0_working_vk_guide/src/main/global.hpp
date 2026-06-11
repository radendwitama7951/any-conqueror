#pragma once

#include <SDL3/SDL.h>

struct app_state_t {
  SDL_Window* p_wdow;
  int wdow_width;
  int wdow_height;
};

static app_state_t g_appstate = {};
