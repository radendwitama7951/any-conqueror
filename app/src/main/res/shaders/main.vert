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

struct app_game_data_t {
  vec2 coord;
  vec4 color;

  vec2 scale;
  float rot; // degree
  vec2 translate;

  uint texslot; // 0: 1920x1080, 1: 1024x1024
  uint texid;
};

layout(std430, set = 0, binding = 0) readonly buffer app_game_data_buf {
  app_game_data_t instances[];
} batch;

layout(std430, push_constant) uniform app_camera_data_t {
  vec2 pos;
  float zoom;
  vec2 reso;
} u_cam;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_texuv;
layout(location = 2) flat out uint v_texslot;
layout(location = 3) flat out uint v_texid;

void main() {
  app_game_data_t a_data = batch.instances[gl_InstanceIndex];

  vec2 vert = APP_MAIN_QUAD_VERTICES[gl_VertexIndex];

  float obj_rotrads = radians(a_data.rot);
  float obj_rotcos = cos(obj_rotrads);
  float obj_rotsin = sin(obj_rotrads);

  vec2 obj_scaled = vert * a_data.scale;
  vec2 obj_rotated = vec2(
      obj_scaled.x * obj_rotcos - obj_scaled.y * obj_rotsin,
      obj_scaled.x * obj_rotsin + obj_scaled.y * obj_rotcos);

  vec2 obj_world = obj_rotated + a_data.coord + a_data.translate;

  vec2 obj_view = (obj_world - u_cam.pos) * u_cam.zoom;

  // const vec2 half_reso = vec2(0, 0); // u_cam.reso / 2.0;

  // Anchored top left (unused)
  // const vec2 anchor_view = (obj_view - half_reso) * u_cam.zoom;

  const vec2 aspect = 2.0 / u_cam.reso;

  vec2 obj_clip;
  obj_clip.x = obj_view.x * aspect.x;
  obj_clip.y = obj_view.y * aspect.y;

  gl_Position = vec4(obj_clip, 0.f, 1.f);

  v_color = a_data.color;
  v_texuv = APP_MAIN_UV_MAPPING[gl_VertexIndex];
  v_texslot = a_data.texslot;
  v_texid = a_data.texid;
}
