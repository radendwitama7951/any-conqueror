#pragma once

typedef struct {
  const float texslot;
  const float texid;
  const float texcount;
  const float rot;
  const float scalex;
  const float scaley;
  const float transx;
  const float transy;
} app_res_anim_data_t;

const app_res_anim_data_t APP_RES_ANIM_DATA[] = {
    (app_res_anim_data_t){
        .texslot  = 1,
        .texid    = 4,
        .texcount = 3,
        .rot      = 0,
        .scalex   = 73.384,
        .scaley   = 73.384,
        .transx   = 72.817,
        .transy   = -16.445,
    },
};

#define APP_RES_ANIM_DATA_EFFECT_MEDIUM_TANK_GUNSHOT_A 0
