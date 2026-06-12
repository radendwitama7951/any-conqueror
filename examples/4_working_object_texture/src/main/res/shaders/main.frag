#version 450

#extension GL_EXT_nonuniform_qualifier : enable

precision mediump float;

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texuv;
layout(location = 2) flat in uint v_texslot;
layout(location = 3) flat in uint v_texid;

layout(location = 0) out vec4 out_color;

// SLOT { 0: 1920x1080, 1: 1024x1024 }
layout(set = 0, binding = 1) uniform sampler2DArray u_texs[];

void main() {
  vec4 tex_color = texture(u_texs[nonuniformEXT(v_texslot)], vec3(v_texuv, v_texid));

  out_color = tex_color * v_color;
}
