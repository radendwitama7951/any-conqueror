#pragma once

typedef struct {
  // UNIT_DATA
  const float base_dmg;
  const float base_hp;
  const float base_def;
  const float atk_range;
  const float vision;
  const float movement;

  // RENDER
  const float texslot;
  const float texid;
  const float rot;
  const float scalex;
  const float scaley;
  const float transx;
  const float transy;

} app_res_unit_data_t;

const app_res_unit_data_t APP_RES_UNIT_DATA[] = {
    (app_res_unit_data_t){
        // UNIT_DATA
        .base_dmg  = 100,
        .base_hp   = 100,
        .base_def  = 20,
        .atk_range = 1,
        .vision    = 10,
        .movement  = 8,

        //RENDER
        .texslot = 1,
        .texid   = 2,
        .rot     = 0,
        .scalex  = 73.399,
        .scaley  = 73.399,
        .transx  = 0,
        .transy  = -19.983,

    },
    (app_res_unit_data_t){
        // UNIT_DATA
        .base_dmg  = 200,
        .base_hp   = 600,
        .base_def  = 20,
        .atk_range = 6,
        .vision    = 10,
        .movement  = 10,
        // RENDER
        .texslot = 1,
        .texid   = 3,
        .rot     = 0,
        .scalex  = 80.841,
        .scaley  = 108.350,
        .transx  = 0,
        .transy  = -6.533,

    },
};

#define APP_RES_UNIT_DATA_HEAVY_INFANTRY 0
#define APP_RES_UNIT_DATA_MEDIUM_TANK 1
