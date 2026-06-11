#version 450

precision mediump float;

highp const vec2 APP_MAIN_QUAD_VERTICES[6] = vec2[](
    // First triangle
    vec2(-0.5f, -0.5f),
    vec2(0.5f, -0.5f),
    vec2(-0.5f, 0.5f),

    // Second triangle
    vec2(0.5f, -0.5f),
    vec2(0.5f, 0.5f),
    vec2(-0.5f, 0.5f)
  );

const vec2 APP_MAIN_UV_MAPPING[6] = vec2[](
    vec2(0.0f, 0.0f), // Top-Left
    vec2(1.0f, 0.0f), // Top-Right
    vec2(0.0f, 1.0f), // Bottom-Left

    vec2(1.0f, 0.0f), // Top-Right
    vec2(1.0f, 1.0f), // Bottom-Right
    vec2(0.0f, 1.0f) // Bottom-Left
  );

const vec4 APP_MAIN_QUAD_COLOR[6] = vec4[](
    vec4(1., 0., 0., 1.),
    vec4(0., 1., 0., 1.),
    vec4(0., 0., 1., 1.),
    vec4(0., 1., 0., 1.),
    vec4(0., 0., 1., 1.),
    vec4(0., 0., 1., 1.)
  );

struct app_game_tile_t {
  vec2 coord;
  vec4 color;

  vec2 scale;
  float rot;
  vec2 translate;

  uint texid;
};

layout(std430, set = 0, binding = 0) readonly buffer app_game_tile_buf {
  app_game_tile_t instances[];
} batch;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_texuv;
layout(location = 2) flat out uint v_texid;

void main() {
  app_game_tile_t tile = batch.instances[gl_InstanceIndex];

  // vec2 vert = APP_MAIN_QUAD_VERTICES[gl_InstanceIndex];
  vec2 vert = APP_MAIN_QUAD_VERTICES[gl_VertexIndex];

  vec2 obj_space = (vert * tile.scale);

  gl_Position = vec4(obj_space, 0.f, 1.f);

  v_color = tile.color; // <-- trying using the tile instance color; didn't work
  v_texuv = APP_MAIN_UV_MAPPING[gl_VertexIndex];
  v_texid = tile.texid;
}
