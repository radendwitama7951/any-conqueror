#pragma once

#include <stddef.h>
#include <stdint.h>

#define APP_GAME_MAX_ENTITY 512

#define APP_GAME_ENTITY_DIRTY_FLAG (1 << 0)
#define APP_GAME_ENTITY_UNITS_OFFSET (0)

size_t g_game_static_enitites_count  = {0};
size_t g_game_tiles_entities_count   = {0};
size_t g_game_units_entities_count   = {0};

uint16_t g_game_static_entities_flag = {0};
uint16_t g_game_tiles_entities_flag  = {0};
uint16_t g_game_units_entities_flag  = {0};

size_t g_game_object_count           = {0};

#define APP_GAME_COORD_COMPONENT_BIT (1 << 0)
#define APP_GAME_RENDER_COMPONENT_BIT (1 << 1)
#define APP_GAME_TRANSFORM_COMPONENT_BIT (1 << 2)

typedef struct {
  uint64_t bitset;
} app_game_entity_t;
app_game_entity_t g_game_ents[APP_GAME_MAX_ENTITY] = {0};

typedef struct {
  uint8_t col;
  uint8_t row;
} app_game_coord_comp_t;
app_game_coord_comp_t g_game_coord_comp[APP_GAME_MAX_ENTITY] = {0};

typedef struct {
  uint8_t texslot;
  uint8_t texid;
} app_game_render_comp_t;
app_game_render_comp_t g_game_render_comp[APP_GAME_MAX_ENTITY] = {0};

typedef struct {
  float rot;
  float scalex;
  float scaley;
  float transx;
  float transy;
} app_game_transform_comp_t;
app_game_transform_comp_t g_game_transform_comp[APP_GAME_MAX_ENTITY] = {0};
