#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <assert.h>
#include <stdint.h>

#define VK_NO_PROTOTYPES
#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "vk_mem_alloc.h"

#include "log.h"
#include "utils.h"

#include "cglm/cglm.h"
#include "cglm/struct.h"

#define APP_VK_MAX_PDEVS 4
#define APP_VK_SC_IMGS_CAP 8
#define APP_VK_MAX_FIFO 2

typedef struct {
  VkInstance inst;
  VkPhysicalDevice devs[APP_VK_MAX_PDEVS];
  uint32_t devs_cnt;
  uint32_t dev_idx;
  VkDevice dev;

  VkQueue queue;

  VmaAllocator vma;

  VkSurfaceKHR surface;
  VkSurfaceCapabilitiesKHR surface_capas;

  uint32_t img_idx;
  uint32_t frame_idx;

  VkSwapchainKHR sc;
  bool update_sc;
  VkImage sc_imgs[APP_VK_SC_IMGS_CAP];
  VkImageView sc_img_views[APP_VK_SC_IMGS_CAP];

  VkFence fncs[APP_VK_MAX_FIFO];
  uint32_t fncs_sz;
  VkSemaphore present_sems[APP_VK_MAX_FIFO];
  uint32_t present_sems_sz;
  VkSemaphore render_sems[APP_VK_SC_IMGS_CAP];
  uint32_t render_sems_sz;

  VkCommandPool cp;
  VkCommandBuffer cb[APP_VK_MAX_FIFO];

  struct {
    VkFormat img_fmt;
    VkSwapchainCreateInfoKHR sc_ci;
    uint32_t img_cnt;
  } ex;

} VulkanState;

typedef struct {
  VulkanState vk;
  SDL_Window* p_window;

  ivec2s window_sz;

} AppState;

void app_init_sdl(AppState* p_as);
void app_init_vulkan(AppState* p_as);

uint32_t app_init(void** pp_appstate, int argc, char** argv) {
  app_init_logger();

  AppState* p_as = SDL_calloc(1, sizeof(AppState));
  app_chk(p_as != NULL, SDL_GetError());
  memset(p_as, 0, sizeof(AppState));

  *pp_appstate = p_as;

  app_init_sdl(p_as);
  app_init_vulkan(p_as);

  return SDL_APP_CONTINUE;  // App is ready to run
}

uint32_t app_event(void* p_appstate, SDL_Event* p_event) {
  SDL_Event* ev = p_event;

  switch (ev->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN: {
      switch (ev->key.scancode) {
        case SDL_SCANCODE_AC_BACK:
        case SDL_SCANCODE_ESCAPE: {
          return SDL_APP_SUCCESS;
        } break;

        default: {
        } break;
      }
    } break;
  }

  return SDL_APP_CONTINUE;
}

uint32_t app_iterate(void* p_appstate) {
  AppState* p_as                            = (AppState*)p_appstate;
  SDL_Window* p_window                      = p_as->p_window;
  VulkanState* p_vks                        = &p_as->vk;
  VkInstance* p_inst                        = &p_vks->inst;
  uint32_t* p_dev_cnt                       = &p_vks->devs_cnt;
  VkPhysicalDevice* p_devs                  = p_vks->devs;
  uint32_t* p_dev_idx                       = &p_vks->dev_idx;
  VkDevice* p_dev                           = &p_vks->dev;
  VkQueue* p_queue                          = &p_vks->queue;
  VmaAllocator* p_vma                       = &p_vks->vma;
  VkSurfaceKHR* p_surface                   = &p_vks->surface;
  ivec2s* p_window_sz                       = &p_as->window_sz;
  VkSurfaceCapabilitiesKHR* p_surface_capas = &p_vks->surface_capas;
  VkFormat* p_img_fmt                       = &p_vks->ex.img_fmt;
  VkSwapchainCreateInfoKHR* p_sc_ci         = &p_vks->ex.sc_ci;
  VkSwapchainKHR* p_sc                      = &p_vks->sc;
  const uint32_t img_cnt                    = p_vks->ex.img_cnt;
  VkImage* p_sc_imgs                        = p_vks->sc_imgs;
  VkImageView* p_sc_img_views               = p_vks->sc_img_views;
  const uint32_t max_fifo                   = APP_VK_MAX_FIFO;
  VkFence* p_fncs                           = p_vks->fncs;
  VkSemaphore* p_present_sems               = p_vks->present_sems;
  VkSemaphore* p_render_sems                = p_vks->render_sems;
  VkCommandPool* p_cp                       = &p_vks->cp;
  VkCommandBuffer* p_cb                     = p_vks->cb;
  const uint32_t fncs_sz                    = p_vks->fncs_sz;
  const uint32_t present_sems_sz            = p_vks->present_sems_sz;
  const uint32_t render_sems_sz             = p_vks->render_sems_sz;
  uint32_t* p_frame_idx                     = &p_vks->frame_idx;
  uint32_t* p_img_idx                       = &p_vks->img_idx;
  bool* p_update_sc                         = &p_vks->update_sc;

  // Wait for the specific "Frame in Flight" to be finished by the GPU
  app_chk_vk(vkWaitForFences(*p_dev, 1, &p_fncs[*p_frame_idx], VK_TRUE, UINT64_MAX),
             "vkWaitForFences");
  app_chk_vk(vkResetFences(*p_dev, 1, &p_fncs[*p_frame_idx]), "vkResetFences");

  app_chk_vk_sc(vkAcquireNextImageKHR(*p_dev, *p_sc, UINT64_MAX, p_present_sems[*p_frame_idx],
                                      VK_NULL_HANDLE, p_img_idx),
                p_update_sc);

  VkCommandBuffer cb = p_cb[*p_frame_idx];
  app_chk_vk(vkResetCommandBuffer(cb, 0), "vkResetCommandBuffer");
  VkCommandBufferBeginInfo cb_bi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  app_chk_vk(vkBeginCommandBuffer(cb, &cb_bi), "vkBeginCommandBuffer");

  VkImageMemoryBarrier2 barrier = {.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                   .srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   .dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                   .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
                                   .newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   .image         = p_vks->sc_imgs[p_vks->img_idx],
                                   .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

  VkDependencyInfo barrier_di   = {.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                   .imageMemoryBarrierCount = 1,
                                   .pImageMemoryBarriers    = &barrier};
  vkCmdPipelineBarrier2(cb, &barrier_di);

  // TODO Dynamic Rendering
  VkRenderingAttachmentInfo col_atchi = {.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                         .imageView   = p_sc_img_views[*p_img_idx],
                                         .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                         .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                         .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                                         .clearValue  = {.color = {0.1f, 0.1f, 0.2f, 1.0f}}};

  VkRenderingInfo ri = {.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
                        .renderArea           = {.extent = {.width  = (uint32_t)p_window_sz->x,
                                                            .height = (uint32_t)p_window_sz->y}},
                        .layerCount           = 1,
                        .colorAttachmentCount = 1,
                        .pColorAttachments    = &col_atchi};
  vkCmdBeginRendering(cb, &ri);

  // TODO End Rendering
  vkCmdEndRendering(cb);

  // Barrier: Transition to PRESENT_SRC_KHR so the screen can show it
  barrier.oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = 0;
  vkCmdPipelineBarrier2(cb, &barrier_di);
  app_chk_vk(vkEndCommandBuffer(cb), "vkEndCommandBuffer");

  // TODO Submit & Present
  VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo si                  = {.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                      .waitSemaphoreCount   = 1,
                                      .pWaitSemaphores      = &p_present_sems[*p_frame_idx],
                                      .pWaitDstStageMask    = &wait_stages,
                                      .commandBufferCount   = 1,
                                      .pCommandBuffers      = &cb,
                                      .signalSemaphoreCount = 1,
                                      .pSignalSemaphores    = &p_render_sems[*p_img_idx]};
  app_chk_vk(vkQueueSubmit(*p_queue, 1, &si, p_fncs[*p_frame_idx]), "vkQueueSubmit");
  *p_frame_idx        = (*p_frame_idx + 1) % max_fifo;
  VkPresentInfoKHR pi = {.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                         .waitSemaphoreCount = 1,
                         .pWaitSemaphores    = &p_render_sems[*p_img_idx],
                         .swapchainCount     = 1,
                         .pSwapchains        = p_sc,
                         .pImageIndices      = p_img_idx};
  app_chk_vk_sc(vkQueuePresentKHR(*p_queue, &pi), "vkQueuePresentKHR");

  return SDL_APP_CONTINUE;  // Continue running
}

void app_quit(void* p_appstate, SDL_AppResult result) {
  AppState* p_as                            = (AppState*)p_appstate;
  SDL_Window* p_window                      = p_as->p_window;
  VulkanState* p_vks                        = &p_as->vk;
  VkInstance* p_inst                        = &p_vks->inst;
  uint32_t* p_dev_cnt                       = &p_vks->devs_cnt;
  VkPhysicalDevice* p_devs                  = p_vks->devs;
  uint32_t* p_dev_idx                       = &p_vks->dev_idx;
  VkDevice* p_dev                           = &p_vks->dev;
  VkQueue* p_queue                          = &p_vks->queue;
  VmaAllocator* p_vma                       = &p_vks->vma;
  VkSurfaceKHR* p_surface                   = &p_vks->surface;
  ivec2s* p_window_sz                       = &p_as->window_sz;
  VkSurfaceCapabilitiesKHR* p_surface_capas = &p_vks->surface_capas;
  VkFormat* p_img_fmt                       = &p_vks->ex.img_fmt;
  VkSwapchainCreateInfoKHR* p_sc_ci         = &p_vks->ex.sc_ci;
  VkSwapchainKHR* p_sc                      = &p_vks->sc;
  const uint32_t img_cnt                    = p_vks->ex.img_cnt;
  VkImage* p_sc_imgs                        = p_vks->sc_imgs;
  VkImageView* p_sc_img_views               = p_vks->sc_img_views;
  const uint32_t max_fifo                   = APP_VK_MAX_FIFO;
  VkFence* p_fncs                           = p_vks->fncs;
  VkSemaphore* p_present_sems               = p_vks->present_sems;
  VkSemaphore* p_render_sems                = p_vks->render_sems;
  VkCommandPool* p_cp                       = &p_vks->cp;
  VkCommandBuffer* p_cb                     = p_vks->cb;
  const uint32_t fncs_sz                    = p_vks->fncs_sz;
  const uint32_t present_sems_sz            = p_vks->present_sems_sz;
  const uint32_t render_sems_sz             = p_vks->render_sems_sz;

  app_chk_vk(vkDeviceWaitIdle(*p_dev), "vkDeviceWaitIdle");

  if (p_dev != VK_NULL_HANDLE) {
    for (int i = 0; i < fncs_sz; ++i) {
      vkDestroyFence(*p_dev, p_fncs[i], NULL);
      vkDestroySemaphore(*p_dev, p_present_sems[i], NULL);
    }
    for (int i = 0; i < render_sems_sz; ++i) {
      vkDestroySemaphore(*p_dev, p_render_sems[i], NULL);
    }
    for (int i = 0; i < img_cnt; i++) {
      vkDestroyImageView(*p_dev, p_sc_img_views[i], NULL);
    }

    vkDestroySwapchainKHR(*p_dev, *p_sc, NULL);
    vkDestroyCommandPool(*p_dev, *p_cp, NULL);

    vmaDestroyAllocator(*p_vma);
    vkDestroyDevice(*p_dev, NULL);
  }

  if (p_surface != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(*p_inst, *p_surface, NULL);

  if (p_inst != VK_NULL_HANDLE)
    vkDestroyInstance(*p_inst, NULL);

  SDL_DestroyWindow(p_window);
  SDL_Quit();

  SDL_free(p_as);
}

void app_init_sdl(AppState* p_as) {
  SDL_Window** p_window = &p_as->p_window;

  app_chk(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");

  SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  SDL_WindowFlags window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN;

  *p_window =
      SDL_CreateWindow(APP_NAME, (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
  app_chk(p_window != NULL, SDL_GetError());

  SDL_SetWindowPosition(*p_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(*p_window);
}

void app_init_vulkan(AppState* p_as) {
  SDL_Window* p_window                      = p_as->p_window;
  VulkanState* p_vks                        = &p_as->vk;
  VkInstance* p_inst                        = &p_vks->inst;
  uint32_t* p_dev_cnt                       = &p_vks->devs_cnt;
  VkPhysicalDevice* p_devs                  = p_vks->devs;
  uint32_t* p_dev_idx                       = &p_vks->dev_idx;
  VkDevice* p_dev                           = &p_vks->dev;
  VkQueue* p_queue                          = &p_vks->queue;
  VmaAllocator* p_vma                       = &p_vks->vma;
  VkSurfaceKHR* p_surface                   = &p_vks->surface;
  ivec2s* p_window_sz                       = &p_as->window_sz;
  VkSurfaceCapabilitiesKHR* p_surface_capas = &p_vks->surface_capas;
  VkFormat* p_img_fmt                       = &p_vks->ex.img_fmt;
  VkSwapchainCreateInfoKHR* p_sc_ci         = &p_vks->ex.sc_ci;
  VkSwapchainKHR* p_sc                      = &p_vks->sc;
  uint32_t* p_img_cnt                       = &p_vks->ex.img_cnt;
  VkImage* p_sc_imgs                        = p_vks->sc_imgs;
  VkImageView* p_sc_img_views               = p_vks->sc_img_views;
  const uint32_t max_fifo                   = APP_VK_MAX_FIFO;
  VkFence* p_fncs                           = p_vks->fncs;
  VkSemaphore* p_present_sems               = p_vks->present_sems;
  VkSemaphore* p_render_sems                = p_vks->render_sems;
  VkCommandPool* p_cp                       = &p_vks->cp;
  VkCommandBuffer* p_cb                     = p_vks->cb;
  uint32_t* p_fncs_sz                       = &p_vks->fncs_sz;
  uint32_t* p_present_sems_sz               = &p_vks->present_sems_sz;
  uint32_t* p_render_sems_sz                = &p_vks->render_sems_sz;

  // INIT INSTANCE
  app_chk(SDL_Vulkan_LoadLibrary(NULL), SDL_GetError());
  volkInitialize();

  VkApplicationInfo app_i      = {.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                  .pApplicationName = APP_NAME,
                                  .apiVersion       = VK_API_VERSION_1_3};

  uint32_t inst_ext_cnt        = {0};
  char const* const* inst_exts = {SDL_Vulkan_GetInstanceExtensions(&inst_ext_cnt)};
  VkInstanceCreateInfo inst_ci = {.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                  .pApplicationInfo        = &app_i,
                                  .enabledExtensionCount   = inst_ext_cnt,
                                  .ppEnabledExtensionNames = inst_exts};

  // LOAD FUNCTION
  app_chk_vk(vkCreateInstance(&inst_ci, NULL, p_inst), "vkCreateInstance");
  volkLoadInstance(*p_inst);

  // INIT PHYSICAL DEVICE
  *p_dev_cnt = 1;
  app_chk_vk(vkEnumeratePhysicalDevices(*p_inst, p_dev_cnt, NULL), "vkEnumeratePhysicalDevices");
  log_info("Vk Physical Device Count: %d", *p_dev_cnt);
  app_chk(*p_dev_cnt < APP_VK_MAX_PDEVS, "*p_dev_cnt < APP_VK_MAX_PDEVS");
  app_chk_vk(vkEnumeratePhysicalDevices(*p_inst, p_dev_cnt, p_devs), "vkEnumeratePhysicalDevices");
  *p_dev_idx                            = 0;

  VkPhysicalDeviceProperties2 dev_props = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(p_devs[*p_dev_idx], &dev_props);

  // INIT QUEUE
  uint32_t q_fam_cnt = {0};
  vkGetPhysicalDeviceQueueFamilyProperties(p_devs[*p_dev_idx], &q_fam_cnt, NULL);
  VkQueueFamilyProperties q_fams[q_fam_cnt];
  vkGetPhysicalDeviceQueueFamilyProperties(p_devs[*p_dev_idx], &q_fam_cnt, q_fams);

  uint32_t q_fam = {0};
  for (size_t i = 0; i < q_fam_cnt; ++i) {
    if (q_fams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      q_fam = i;
      break;
    }
  }

  app_chk(SDL_Vulkan_GetPresentationSupport(*p_inst, p_devs[*p_dev_idx], q_fam),
          "SDL_Vulkan_GetPresentationSupport");

  const float q_pri            = {1.0f};
  VkDeviceQueueCreateInfo q_ci = {.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                  .queueFamilyIndex = q_fam,
                                  .queueCount       = 1,
                                  .pQueuePriorities = &q_pri};

  // INIT DEVICE
  VkPhysicalDeviceVulkan12Features vk12_f = {
      .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .descriptorIndexing = true,
      .shaderSampledImageArrayNonUniformIndexing = true,
      .descriptorBindingVariableDescriptorCount  = true,
      .runtimeDescriptorArray                    = true,
      .bufferDeviceAddress                       = true};
  VkPhysicalDeviceVulkan13Features vk13_f = {
      .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext            = &vk12_f,
      .synchronization2 = true,
      .dynamicRendering = true};
  VkPhysicalDeviceFeatures vk10_f = {.samplerAnisotropy = VK_TRUE};
  const char* dev_exts[]          = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo dev_ci       = {.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                     .pNext                   = &vk13_f,
                                     .queueCreateInfoCount    = 1,
                                     .pQueueCreateInfos       = &q_ci,
                                     .enabledExtensionCount   = (uint32_t)ARRAY_SIZE(dev_exts),
                                     .ppEnabledExtensionNames = dev_exts,
                                     .pEnabledFeatures        = &vk10_f};
  app_chk_vk(vkCreateDevice(p_devs[*p_dev_idx], &dev_ci, NULL, p_dev), "vkCreateDevice");
  vkGetDeviceQueue(*p_dev, q_fam, 0, p_queue);

  // INIT VMA
  VmaVulkanFunctions vma_vk_fn = {
      .vkGetInstanceProcAddr               = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr                 = vkGetDeviceProcAddr,
      .vkGetPhysicalDeviceProperties       = vkGetPhysicalDeviceProperties,
      .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
      .vkAllocateMemory                    = vkAllocateMemory,
      .vkFreeMemory                        = vkFreeMemory,
      .vkMapMemory                         = vkMapMemory,
      .vkUnmapMemory                       = vkUnmapMemory,
      .vkFlushMappedMemoryRanges           = vkFlushMappedMemoryRanges,
      .vkInvalidateMappedMemoryRanges      = vkInvalidateMappedMemoryRanges,
      .vkBindBufferMemory                  = vkBindBufferMemory,
      .vkBindImageMemory                   = vkBindImageMemory,
      .vkGetBufferMemoryRequirements       = vkGetBufferMemoryRequirements,
      .vkGetImageMemoryRequirements        = vkGetImageMemoryRequirements,
      .vkCreateBuffer                      = vkCreateBuffer,
      .vkDestroyBuffer                     = vkDestroyBuffer,
      .vkCreateImage                       = vkCreateImage,
      .vkDestroyImage                      = vkDestroyImage,
      .vkCmdCopyBuffer                     = vkCmdCopyBuffer,
      // Required for VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT:
      .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2};

  VmaAllocatorCreateInfo vma_ci = {.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                   .physicalDevice = p_devs[*p_dev_idx],
                                   .device         = *p_dev,
                                   .pVulkanFunctions = &vma_vk_fn,
                                   .instance         = *p_inst};
  app_chk_vk(vmaCreateAllocator(&vma_ci, p_vma), "vmaCreateAllocator");

  // CREATE SURFACE AND GET WINDOW SIZE
  app_chk(SDL_Vulkan_CreateSurface(p_window, *p_inst, NULL, p_surface), SDL_GetError());
  app_chk(SDL_GetWindowSize(p_window, &p_window_sz->x, &p_window_sz->y), SDL_GetError());
  log_info("SDL window size: %dpx x %dpx", p_window_sz->x, p_window_sz->y);
  app_chk_vk(
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_devs[*p_dev_idx], *p_surface, p_surface_capas),
      "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

  // CREATE SWAPCHAINS AND IMAGE VIEWS
  VkExtent2D swp_ext = p_surface_capas->currentExtent;
  if (p_surface_capas->currentExtent.width == 0xFFFFFFFF) {
    swp_ext = (VkExtent2D){.width = (uint32_t)p_window_sz->x, .height = (uint32_t)p_window_sz->y};
  }
  *p_img_fmt = VK_FORMAT_B8G8R8A8_SRGB;
  *p_sc_ci   = (VkSwapchainCreateInfoKHR){.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                          .surface = *p_surface,
                                          .minImageCount    = p_surface_capas->minImageCount,
                                          .imageFormat      = *p_img_fmt,
                                          .imageColorSpace  = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
                                          .imageExtent      = swp_ext,
                                          .imageArrayLayers = 1,
                                          .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                          .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                                          .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                          .presentMode      = VK_PRESENT_MODE_FIFO_KHR};
  app_chk_vk(vkCreateSwapchainKHR(*p_dev, p_sc_ci, NULL, p_sc), "vkCreateSwapchainKHR");
  app_chk_vk(vkGetSwapchainImagesKHR(*p_dev, *p_sc, p_img_cnt, NULL), "vkGetSwapchainImagesKHR");
  app_chk(*p_img_cnt < APP_VK_SC_IMGS_CAP, "*p_img_cnt < APP_VK_SC_IMGS_CAP");
  log_info("Vk Swaphain Images Count: %d", *p_img_cnt);
  app_chk_vk(vkGetSwapchainImagesKHR(*p_dev, *p_sc, p_img_cnt, p_sc_imgs),
             "vkGetSwapchainImagesKHR");

  for (int i = 0; i < *p_img_cnt; ++i) {
    VkImageViewCreateInfo img_view_ci = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = p_sc_imgs[i],
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = *p_img_fmt,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};

    app_chk_vk(vkCreateImageView(*p_dev, &img_view_ci, NULL, &p_sc_img_views[i]),
               "vkCreateImageView");
  }

  // CREATE SYNC OBJECTS
  // "Frame-in-Flight" system with 2 Fences and 2 sets of Semaphores (for image acquisition and rendering completion).
  VkSemaphoreCreateInfo sem_ci = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fnc_ci     = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                  .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  *p_fncs_sz                   = max_fifo;
  *p_present_sems_sz           = max_fifo;
  for (int i = 0; i < max_fifo; ++i) {
    app_chk_vk(vkCreateFence(*p_dev, &fnc_ci, NULL, &p_fncs[i]), "vkCreateFence");
    app_chk_vk(vkCreateSemaphore(*p_dev, &sem_ci, NULL, &p_present_sems[i]), "vkCreateSemaphore");
  }

  *p_render_sems_sz = *p_img_cnt;
  for (int i = 0; i < *p_render_sems_sz; ++i) {
    app_chk_vk(vkCreateSemaphore(*p_dev, &sem_ci, NULL, &p_render_sems[i]), "vkCreateSemaphore");
  }

  // CREATE COMMAND POOL & BUFFERS
  VkCommandPoolCreateInfo cp_ci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                   .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                   .queueFamilyIndex = q_fam};
  app_chk_vk(vkCreateCommandPool(*p_dev, &cp_ci, NULL, p_cp), "vkCreateCommandPool");
  VkCommandBufferAllocateInfo cb_ai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                       .commandPool        = *p_cp,
                                       .commandBufferCount = max_fifo};
  app_chk_vk(vkAllocateCommandBuffers(*p_dev, &cb_ai, p_cb), "vkAllocateCommandBuffers");
}
