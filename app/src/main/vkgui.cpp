
#include <stdbool.h>

#include <SDL3/SDL.h>

#include <volk.h>

#include <imgui.h>
//
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include "vkgui.h"

#include "utils.h"

#define APP_VK_MAX_FIFO 2
#define APP_VK_MAX_PDEVS 4

extern SDL_Window* g_pmainwdow;

extern VkPhysicalDevice g_vkphydevs[APP_VK_MAX_PDEVS];
extern uint32_t g_vkphydevs_cnt;
extern uint32_t g_vkphydev_idx;

extern VkDevice g_vkdev;
extern VkFormat g_vkscimgfmt;
extern uint32_t g_vkimg_cnt;

extern VkInstance g_vkinst;
extern VkQueue g_vkqueue;
extern uint32_t g_vkq_fam;

static VkDescriptorPool g_vkguidsetpool;

bool app_vkgui_init() {

  VkDescriptorPoolSize dpoolszs[] = {
      (VkDescriptorPoolSize){
          .type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
          .descriptorCount = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE,
      },
      (VkDescriptorPoolSize){
          .type            = VK_DESCRIPTOR_TYPE_SAMPLER,
          .descriptorCount = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE,

      },
  };
  VkDescriptorPoolCreateInfo dpool_ci = {
      .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets       = 0,
      .poolSizeCount = SDL_arraysize(dpoolszs),
      .pPoolSizes    = dpoolszs,
  };

  for (size_t i = 0; i < SDL_arraysize(dpoolszs); ++i) {
    dpool_ci.maxSets += dpoolszs[i].descriptorCount;
  }

  app_vkchk(vkCreateDescriptorPool(g_vkdev, &dpool_ci, VK_NULL_HANDLE, &g_vkguidsetpool),
            "vkCreateDescriptorPool");

  // VkFormat img_fmt = VK_FORMAT_B8G8R8A8_UNORM;
  VkPipelineRenderingCreateInfoKHR plrender_ci = {
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .pNext                   = VK_NULL_HANDLE,
      .colorAttachmentCount    = 1,
      .pColorAttachmentFormats = &g_vkscimgfmt,
      .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
      .stencilAttachmentFormat = VK_FORMAT_UNDEFINED};

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  // (void)io;

  io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(1);
  style.FontScaleDpi = 1;

  SDL_assert(g_vkimg_cnt >= 2);

  ImGui_ImplSDL3_InitForVulkan(g_pmainwdow);
  ImGui_ImplVulkan_InitInfo implvkinit_i = {
      .Instance       = g_vkinst,
      .PhysicalDevice = g_vkphydevs[g_vkphydev_idx],
      .Device         = g_vkdev,
      .QueueFamily    = g_vkq_fam,
      .Queue          = g_vkqueue,
      .DescriptorPool = g_vkguidsetpool,
      .MinImageCount  = APP_VK_MAX_FIFO,
      .ImageCount     = g_vkimg_cnt,
      .PipelineInfoMain =
          {
              .PipelineRenderingCreateInfo = plrender_ci,
          },
      .Allocator           = VK_NULL_HANDLE,
      .PipelineCache       = VK_NULL_HANDLE,
      .UseDynamicRendering = true,
      .CheckVkResultFn     = VK_NULL_HANDLE,
  };

  app_chk(ImGui_ImplVulkan_Init(&implvkinit_i), "ImGui_ImplVulkan_Init");

  return true;
}
bool app_vkgui_event(SDL_Event* p_ev) {
  ImGui_ImplSDL3_ProcessEvent(p_ev);
  return true;
}
bool app_vkgui_begin() {

  // Start the Dear ImGui frame
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  return true;
}

bool app_vkgui_end(VkCommandBuffer* p_cb) {
  // Rendering
  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();

  ImGui_ImplVulkan_RenderDrawData(draw_data, *p_cb);

  return true;
}
bool app_vkgui_destroy() {
  app_vkchk(vkDeviceWaitIdle(g_vkdev), "vkDeviceWaitIdle");

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  vkDestroyDescriptorPool(g_vkdev, g_vkguidsetpool, VK_NULL_HANDLE);

  return true;
}
