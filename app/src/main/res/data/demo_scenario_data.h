#pragma once

#include <stddef.h>
#include <stdint.h>

#include "main/res/data/unit_data.h"

typedef struct {
  size_t col_count;
  size_t row_count;

  struct unit_info_t {
    uint64_t id;
    uint8_t col;
    uint8_t row;

  } initial_units[2];

  struct {
    uint8_t col;
    uint8_t row;

    float movement_cost;
    float vision_cost;

  } tiles[32 * 16];

} app_res_demo_scenario_data_t;

const app_res_demo_scenario_data_t g_app_res_demo_scenario_data = {
    .col_count = 32,
    .row_count = 16,

    .initial_units =
        {

            {
                .id  = APP_RES_UNIT_DATA_HEAVY_INFANTRY,
                .col = 16,
                .row = 8,

            },
            {
                .id  = APP_RES_UNIT_DATA_MEDIUM_TANK,
                .col = 25,
                .row = 8,

            },
        },
    .tiles = {{0}},

};
