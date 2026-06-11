#pragma once

#include <SDL3/SDL.h>

#include <volk.h>

#include <vk_mem_alloc.h>

#define APP_MAIN_WINDOW_SCALE 0.5
#define APP_MAIN_WINDOW_WIDTH (1920 * APP_MAIN_WINDOW_SCALE)
#define APP_MAIN_WINDOW_HEIGHT (1080 * APP_MAIN_WINDOW_SCALE)

SDL_Window* g_pmainwdow;

/*
 * GAME
 *
 * */
#define APP_GAME_GRID_MAX_COL 32
#define APP_GAME_GRID_MAX_ROW 16
#define APP_GAME_GRID_TILE_COUNT (APP_GAME_GRID_MAX_COL * APP_GAME_GRID_MAX_ROW)

/*
 * // SHADERS SSBO STRUCT
 *
 * struct app_game_tile_t {
 *  vec2 coord;
 *  vec4 color;
 *  vec2 scale;
 *  float rot;
 *  vec2 translate
 *
 *  uint texid
 * };
 *
 * */
typedef struct {
  // 32 byte pad
  float coordx;
  float coordy;
  float _pad0[2];

  // 32 byte pad
  float colorr;
  float colorg;
  float colorb;
  float colora;

  // 32 byte pad
  float scalex;
  float scaley;
  float rot;
  uint32_t _pad1[1];

  // 32 byte pad
  float transx;
  float transy;
  uint32_t texid;
  uint32_t _pad2[1];
} app_game_tile_data_t;

#define APP_GAME_TILE_DATA_SZ sizeof(app_game_tile_data_t)

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
#define APP_VK_SHADER_MAIN_SSBO_BUFFER_SZ

VkInstance g_vkinst;
VkPhysicalDevice g_vkdevs[APP_VK_MAX_PDEVS];
uint32_t g_vkdevs_cnt;
uint32_t g_vkdev_idx;
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

// RESOURCES
/*
 * DESCRIPTOR SET (VERTEX ATTRIBUTE) // UNUSED
 * 
 * */
#define APP_VK_MAIN_ATTR_COORD_FORMAT VK_FORMAT_R32G32_SFLOAT
#define APP_VK_MAIN_ATTR_COLOR_FORMAT VK_FORMAT_R32G32B32A32_SFLOAT
#define APP_VK_MAIN_ATTR_SCALE_FORMAT VK_FORMAT_R32G32_SFLOAT
#define APP_VK_MAIN_ATTR_ROTATION_FORMAT VK_FORMAT_R32_SFLOAT
#define APP_VK_MAIN_ATTR_TRANSLATION_FORMAT VK_FORMAT_R32G32_SFLOAT
#define APP_VK_MAIN_ATTR_TEXTUREID_FORMAT VK_FORMAT_R32_UINT

#define APP_VK_MAIN_ATTR_COORD_OFFSET offsetof(app_game_tile_data_t, coordx)
#define APP_VK_MAIN_ATTR_COLOR_OFFSET offsetof(app_game_tile_data_t, colorr)
#define APP_VK_MAIN_ATTR_SCALE_OFFSET offsetof(app_game_tile_data_t, scalex)
#define APP_VK_MAIN_ATTR_ROTATION_OFFSET offsetof(app_game_tile_data_t, rot)
#define APP_VK_MAIN_ATTR_TRANSLATION_OFFSET offsetof(app_game_tile_data_t, transx)
#define APP_VK_MAIN_ATTR_TEXTUREID_OFFSET offsetof(app_game_tile_data_t, texid)

/*
 * DESCRIPTOR SET (VERTEX ATTRIBUTE) SSBOs
 * 
 * */

#define APP_VK_MAIN_GAME_TILES_BUFFER_SZ (APP_GAME_TILE_DATA_SZ * APP_GAME_GRID_TILE_COUNT)

typedef struct {
  VkBuffer ssbobuf;
  VmaAllocation ssboalloc;
  void* pmapped_data;  // <-- APP_VK_MAIN_GAME_TILES_BUFFER_SZ data
  VkDescriptorSet dset;
} app_vkmain_game_tile_t;

app_vkmain_game_tile_t g_vkmain_game_tiles[APP_VK_MAX_FIFO];

typedef struct {
  VkImage img;
  VmaAllocation alloc;
  VkImageView view;
  VkSampler sampler;
  uint32_t slotid;
} app_vkmain_game_texture_t;

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
