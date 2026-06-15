#pragma once

#include <SDL3/SDL.h>

#include <volk.h>
//
#include <vk_mem_alloc.h>

#include "app_ctx.h"

#define APP_MAIN_WINDOW_SCALE 1.f
#define APP_MAIN_WINDOW_WIDTH (1920 * APP_MAIN_WINDOW_SCALE)
#define APP_MAIN_WINDOW_HEIGHT (1080 * APP_MAIN_WINDOW_SCALE)

SDL_Window* g_pmainwdow;

app_ctx_t g_appctx;

/*
 * GAME
 *
 * */
#define APP_GAME_GRID_MAX_COL 32
#define APP_GAME_GRID_MAX_ROW 16
#define APP_GAME_GRID_TILE_COUNT (APP_GAME_GRID_MAX_COL * APP_GAME_GRID_MAX_ROW)

#define APP_GAME_GRID_DIMENSION_X (1920 * APP_MAIN_WINDOW_SCALE)
#define APP_GAME_GRID_DIMENSION_Y (1080 * APP_MAIN_WINDOW_SCALE)

#define APP_GAME_TEXTURE_ARRAY_COUNT 2
const char* APP_GAME_TEXTURE_ARRAY_SOURCE_PATH[] = {
    APP_ASSETS_SOURCE_DIR "drawables/textures1920x1080.ktx2",
    APP_ASSETS_SOURCE_DIR "drawables/textures1024x1024.ktx2",
};

/*
 *
 * */
typedef struct {
  // 16 byte align
  float coordx;
  float coordy;
  float _pad0[2];

  // 16 byte align
  float colorr;
  float colorg;
  float colorb;
  float colora;

  // 16 byte align
  float scalex;
  float scaley;
  float rot;
  uint32_t _pad1[1];

  // 16 byte align
  float transx;
  float transy;
  uint32_t texslot;
  uint32_t texid;

  // uint32_t _pad2[1];
} app_game_render_data_t;

#define APP_GAME_RENDER_DATA_SZ sizeof(app_game_render_data_t)

/*
 * CAMERA
 * !world space == view space
 * !anchor top-left
 *
 * */
typedef struct {
  // 16 byte align
  float posx;
  float posy;

  float zoom;
  float _pad0[1];

  // 16 byte align
  float resow;
  float resoh;
  // float _pad1[2]; // ignored
} app_game_camera_data_t;
#define APP_GAME_CAMERA_DATA_SZ sizeof(app_game_camera_data_t)

static app_game_camera_data_t g_game_cam = {
    .posx  = -1920 / 2.f,
    .posy  = -1080 / 2.f,
    .zoom  = 1.,
    .resow = 1920,
    .resoh = 1080,
};

#define APP_GAME_TILE_RADIUS_SCALE (1.0 / 2.f)
#define APP_GAME_TILE_INRADIUS_SCALE (0.866 / 2.f)

#define APP_GAME_TILE_SCALE_X (77.942f * APP_MAIN_WINDOW_SCALE)
#define APP_GAME_TILE_SCALE_Y (77.942f * APP_MAIN_WINDOW_SCALE)

#define APP_GAME_TILE_RADIUS (APP_GAME_TILE_RADIUS_SCALE * APP_GAME_TILE_SCALE_X)
#define APP_GAME_TILE_INRADIUS (APP_GAME_TILE_INRADIUS_SCALE * APP_GAME_TILE_SCALE_Y)
#define APP_GAME_TILE_SPACE_HORIZ (3.f / 2.f * APP_GAME_TILE_RADIUS)
#define APP_GAME_TILE_SPACE_VERTI (2.f * APP_GAME_TILE_INRADIUS)

/*
 *
 * VK
 *
 * inst = instance
 * dev = physical/logical device
 * sc = swapchain
 * fnc = fence 
 * sem = semaphore
 * q = queue
 *
 * */

#define APP_VK_MAX_PDEVS 4
#define APP_VK_SC_IMGS_CAP 8
#define APP_VK_MAX_FIFO 2
#define APP_VK_N_BUFFER APP_VK_MAX_FIFO
#define APP_VK_DEVICE_COUNT 1
#define APP_VK_MAX_TEXTURES 128

VkInstance g_vkinst;
VkPhysicalDevice g_vkphydevs[APP_VK_MAX_PDEVS];
uint32_t g_vkphydevs_cnt;
uint32_t g_vkphydev_idx;

VkDevice g_vkdev;

VkQueue g_vkqueue;

VmaAllocator g_vkvma_allocator;

VkSurfaceKHR g_vksurface;
VkSurfaceCapabilitiesKHR g_vksurface_capas;

uint32_t g_vkimg_idx;
uint32_t g_vkframe_idx;

VkSwapchainKHR g_vksc;
VkFormat g_vkscimgfmt = VK_FORMAT_B8G8R8A8_SRGB;
bool g_vkupdate_sc;
VkImage g_vksc_imgs[APP_VK_SC_IMGS_CAP];
VkImageView g_vksc_img_views[APP_VK_SC_IMGS_CAP];

VkFence g_vkfncs[APP_VK_MAX_FIFO];
uint32_t g_vkfncs_sz;
VkSemaphore g_vkpresent_sems[APP_VK_MAX_FIFO];
uint32_t g_vkpresent_sems_sz;
VkSemaphore g_vkrender_sems[APP_VK_SC_IMGS_CAP];
uint32_t g_vkrender_sems_sz;

// RESOURCES
VkDescriptorPool g_vkmaindsetpool;
VkDescriptorSetLayout g_vkmaindsetlayout;

VkCommandPool g_vkcp;
VkCommandBuffer g_vkcb[APP_VK_MAX_FIFO];

VkFormat g_vkimg_fmt;
VkSwapchainCreateInfoKHR g_vksc_ci;
uint32_t g_vkimg_cnt;

uint32_t g_vkq_fam;

VkDescriptorPool g_vkguidsetpool;

// RESOURCES
/*
 * DESCRIPTOR SET (VERTEX ATTRIBUTE) // UNUSED
 * 
 * */

/*
 * DESCRIPTOR SET (VERTEX ATTRIBUTE) SSBOs
 * 
 * */
#define APP_VK_MAIN_GAME_DATA_BUFFER_SZ (3 * APP_GAME_RENDER_DATA_SZ * APP_GAME_GRID_TILE_COUNT)

typedef struct {
  VkBuffer ssbobuf;
  VmaAllocation ssboalloc;
  void* pmapped_data;
  VkDescriptorSet dset;
} app_vkmain_game_data_t;

static app_vkmain_game_data_t g_vkmain_game_data[APP_VK_MAX_FIFO];

typedef struct {
  VkImage img;
  VmaAllocation alloc;
  VkImageView view;
  VkSampler sampler;
  uint32_t layer_count;
  // uint32_t slotid;
} app_vkmain_game_texture_t;

#define APP_VK_MAIN_GAME_TEXTURE_ARRAY_COUNT 2
#define APP_VK_MAIN_GAME_TEXTURE_ARRAY_MIPLEVELS 2
static app_vkmain_game_texture_t g_vkmain_game_texs[APP_VK_MAIN_GAME_TEXTURE_ARRAY_COUNT];

/*
 * SHADER 
 * vshdrm = vertex shader module
 * fshdrm = fragment shader module
 *
 * pl = pipeline
 *
 * */
#define APP_VK_VERTEX_SHADER_SOURCE_PATH APP_ASSETS_SOURCE_DIR "shaders/main.vert.spv"
#define APP_VK_FRAGMENT_SHADER_SOURCE_PATH APP_ASSETS_SOURCE_DIR "shaders/main.frag.spv"

VkShaderModule g_vkmainvshdrm;
VkShaderModule g_vkmainfshdrm;

VkPipelineLayout g_vkmainpllayout;
VkPipeline g_vkmainpl;
