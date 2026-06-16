#pragma once

#include <assert.h>
// #include <math.h>
#include <stdbool.h>

#include <SDL3/SDL.h>

#include <ktx.h>

#include "global.h"
#include "utils.h"

#include "components.h"

#ifdef APP_USE_VK
#include <SDL3/SDL_vulkan.h>
#include <volk.h>
//
#include <ktxvulkan.h>
#include <vk_mem_alloc.h>
#include "vkgui.h"
#endif  // APP_USE_VK

#include "app_gui.h"

#include "main/res/data/anim_data.h"
#include "main/res/data/demo_scenario_data.h"
#include "main/res/data/unit_data.h"

/*
 * 
 * app_vkinit_device
 *
 * */
bool app_vkinit_device() {
  // INIT INSTANCE
  app_chk(SDL_Vulkan_LoadLibrary(NULL), SDL_GetError());
  volkInitialize();

  VkApplicationInfo app_i = {
      .sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = APP_NAME,
      .apiVersion       = VK_API_VERSION_1_3,
  };

  uint32_t inst_ext_cnt        = {0};
  char const* const* inst_exts = {SDL_Vulkan_GetInstanceExtensions(&inst_ext_cnt)};
  VkInstanceCreateInfo inst_ci = {
      .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo        = &app_i,
      .enabledExtensionCount   = inst_ext_cnt,
      .ppEnabledExtensionNames = inst_exts,
  };

  // LOAD FUNCTION
  app_vkchk(vkCreateInstance(&inst_ci, NULL, &g_vkinst), "vkCreateInstance");
  volkLoadInstance(g_vkinst);

  // INIT PHYSICAL DEVICE
  g_vkphydevs_cnt = APP_VK_MAX_PDEVS;
  app_vkchk(vkEnumeratePhysicalDevices(g_vkinst, &g_vkphydevs_cnt, NULL),
            "vkEnumeratePhysicalDevices");
  log_info("Vk Physical Device Count: %d", g_vkphydevs_cnt);
  app_chk(g_vkphydevs_cnt < APP_VK_MAX_PDEVS, "g_vkphydevs_cnt < APP_VK_MAX_PDEVS");
  app_vkchk(vkEnumeratePhysicalDevices(g_vkinst, &g_vkphydevs_cnt, g_vkphydevs),
            "vkEnumeratePhysicalDevices");
  g_vkphydev_idx                        = 0;

  VkPhysicalDeviceProperties2 dev_props = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(g_vkphydevs[g_vkphydev_idx], &dev_props);

  // INIT QUEUE
  uint32_t g_vkq_fam_cnt = {0};
  vkGetPhysicalDeviceQueueFamilyProperties(g_vkphydevs[g_vkphydev_idx], &g_vkq_fam_cnt, NULL);
  VkQueueFamilyProperties g_vkq_fams[g_vkq_fam_cnt];
  vkGetPhysicalDeviceQueueFamilyProperties(g_vkphydevs[g_vkphydev_idx], &g_vkq_fam_cnt, g_vkq_fams);

  // uint32_t
  g_vkq_fam = 0;
  for (size_t i = 0; i < g_vkq_fam_cnt; ++i) {
    if (g_vkq_fams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      g_vkq_fam = i;
      break;
    }
  }

  app_chk(SDL_Vulkan_GetPresentationSupport(g_vkinst, g_vkphydevs[g_vkphydev_idx], g_vkq_fam),
          "SDL_Vulkan_GetPresentationSupport");

  const float q_pri            = {1.0f};
  VkDeviceQueueCreateInfo q_ci = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = g_vkq_fam,
      .queueCount       = 1,
      .pQueuePriorities = &q_pri,
  };

  // INIT DEVICE
  VkPhysicalDeviceVulkan12Features vk12_f = {
      .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .descriptorIndexing = true,
      .shaderSampledImageArrayNonUniformIndexing = true,
      .descriptorBindingVariableDescriptorCount  = true,
      .runtimeDescriptorArray                    = true,
      .bufferDeviceAddress                       = true,
  };
  VkPhysicalDeviceVulkan13Features vk13_f = {
      .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext            = &vk12_f,
      .synchronization2 = true,
      .dynamicRendering = true,
  };
  VkPhysicalDeviceFeatures vk10_f = {
      .samplerAnisotropy = VK_TRUE,

  };

  DESKTOP_ONLY({ vk10_f.textureCompressionBC = VK_TRUE; })
  ANDROID_ONLY({ vk10_f.textureCompressionASTC_LDR = VK_TRUE; })

  const char* dev_exts[]    = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo dev_ci = {
      .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext                   = &vk13_f,
      .queueCreateInfoCount    = 1,
      .pQueueCreateInfos       = &q_ci,
      .enabledExtensionCount   = (uint32_t)APP_GET_ARRAY_SIZE(dev_exts),
      .ppEnabledExtensionNames = dev_exts,
      .pEnabledFeatures        = &vk10_f,
  };
  app_vkchk(vkCreateDevice(g_vkphydevs[g_vkphydev_idx], &dev_ci, NULL, &g_vkdev), "vkCreateDevice");
  vkGetDeviceQueue(g_vkdev, g_vkq_fam, 0, &g_vkqueue);

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
      .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2,
  };

  VmaAllocatorCreateInfo vma_ci = {
      .flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice   = g_vkphydevs[g_vkphydev_idx],
      .device           = g_vkdev,
      .pVulkanFunctions = &vma_vk_fn,
      .instance         = g_vkinst,
  };
  app_vkchk(vmaCreateAllocator(&vma_ci, &g_vkvma_allocator), "vmaCreateAllocator");

  return true;
}

/*
 * 
 * app_vkinit_surface
 *
 * */
bool app_vkinit_surface() {

  // CREATE SURFACE AND GET WINDOW SIZE
  app_chk(SDL_Vulkan_CreateSurface(g_pmainwdow, g_vkinst, NULL, &g_vksurface), SDL_GetError());
  // app_chk(SDL_GetWindowSize(g_pmainwdow, APP_MAIN_WINDOW_WIDTH, APP_MAIN_WINDOW_HEIGHT), SDL_GetError());
  log_info("SDL window size: %dpx x %dpx", APP_MAIN_WINDOW_WIDTH, APP_MAIN_WINDOW_HEIGHT);
  app_vkchk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_vkphydevs[g_vkphydev_idx], g_vksurface,
                                                      &g_vksurface_capas),
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

  return true;
}

/*
 *
 * app_vkinit_swapchain
 *
 * */
bool app_vkinit_swapchain() {
  // CREATE SWAPCHAINS AND IMAGE VIEWS
  VkExtent2D swp_ext = g_vksurface_capas.currentExtent;
  if (g_vksurface_capas.currentExtent.width == 0xFFFFFFFF) {
    swp_ext = (VkExtent2D){.width  = (uint32_t)APP_MAIN_WINDOW_WIDTH,
                           .height = (uint32_t)APP_MAIN_WINDOW_HEIGHT};
  }
  // g_vkimg_fmt = VK_FORMAT_B8G8R8A8_SRGB; // ImGUI color would off
  g_vkimg_fmt = VK_FORMAT_B8G8R8A8_UNORM;  // Change to this because ImGUI
  g_vksc_ci   = (VkSwapchainCreateInfoKHR){
      .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface          = g_vksurface,
      .minImageCount    = g_vksurface_capas.minImageCount,
      .imageFormat      = g_vkimg_fmt,
      .imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
      .imageExtent      = swp_ext,
      .imageArrayLayers = 1,
      .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode      = VK_PRESENT_MODE_FIFO_KHR,
  };
  app_vkchk(vkCreateSwapchainKHR(g_vkdev, &g_vksc_ci, NULL, &g_vksc), "vkCreateSwapchainKHR");
  app_vkchk(vkGetSwapchainImagesKHR(g_vkdev, g_vksc, &g_vkimg_cnt, NULL),
            "vkGetSwapchainImagesKHR");
  app_chk(g_vkimg_cnt < APP_VK_SC_IMGS_CAP, "*g_vkimg_cnt < APP_VK_SC_IMGS_CAP");
  log_info("Vk Swaphain Images Count: %d", g_vkimg_cnt);
  app_vkchk(vkGetSwapchainImagesKHR(g_vkdev, g_vksc, &g_vkimg_cnt, g_vksc_imgs),
            "vkGetSwapchainImagesKHR");

  for (int i = 0; i < g_vkimg_cnt; ++i) {
    VkImageViewCreateInfo img_view_ci = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = g_vksc_imgs[i],
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = g_vkimg_fmt,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .levelCount = 1,
                             .layerCount = 1},
    };

    app_vkchk(vkCreateImageView(g_vkdev, &img_view_ci, NULL, &g_vksc_img_views[i]),
              "vkCreateImageView");
  }
  return true;
}

/*
 *
 * app_vkinit_depthimg
 *
 * */
bool app_vkinit_depthimg() {
  return true;
}

/*
 *
 * app_vkinit_syncobj
 *
 * */
bool app_vkinit_syncobj() {
  // CREATE SYNC OBJECTS
  // "Frame-in-Flight" system with 2 Fences and 2 sets of Semaphores (for image acquisition and rendering completion).
  VkSemaphoreCreateInfo sem_ci = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fnc_ci     = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                  .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  g_vkfncs_sz                  = APP_VK_MAX_FIFO;
  g_vkpresent_sems_sz          = APP_VK_MAX_FIFO;
  for (int i = 0; i < APP_VK_MAX_FIFO; ++i) {
    app_vkchk(vkCreateFence(g_vkdev, &fnc_ci, NULL, &g_vkfncs[i]), "vkCreateFence");
    app_vkchk(vkCreateSemaphore(g_vkdev, &sem_ci, NULL, &g_vkpresent_sems[i]), "vkCreateSemaphore");
  }

  g_vkrender_sems_sz = g_vkimg_cnt;
  for (int i = 0; i < g_vkrender_sems_sz; ++i) {
    app_vkchk(vkCreateSemaphore(g_vkdev, &sem_ci, NULL, &g_vkrender_sems[i]), "vkCreateSemaphore");
  }

  return true;
}

/*
 * app_vkinit_cmdpool
 *
 * */
bool app_vkinit_cmdpool() {
  // CREATE COMMAND POOL & BUFFERS
  VkCommandPoolCreateInfo cp_ci = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = g_vkq_fam,
  };
  app_vkchk(vkCreateCommandPool(g_vkdev, &cp_ci, NULL, &g_vkcp), "vkCreateCommandPool");
  VkCommandBufferAllocateInfo cb_ai = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = g_vkcp,
      .commandBufferCount = APP_VK_MAX_FIFO,
  };
  app_vkchk(vkAllocateCommandBuffers(g_vkdev, &cb_ai, g_vkcb), "vkAllocateCommandBuffers");

  return true;
}

/*
 *
 * app_vkinit_shader
 *
 * */
bool app_vkinit_shader() {
  // LOAD SHADER CODE
  SDL_IOStream* vshdr_io = SDL_IOFromFile(APP_VK_VERTEX_SHADER_SOURCE_PATH, "rb");
  if (!vshdr_io) {
    log_error("[app] SDL_IOFromFile: %s", SDL_GetError());
    return false;
  }

  size_t vshdr_fsz;
  void* vshdr_buf = SDL_LoadFile_IO(vshdr_io, &vshdr_fsz, true);
  if (!vshdr_buf) {
    log_error("[app] SDL_LoadFile_IO: %s", SDL_GetError());
    return false;
  }

  SDL_IOStream* fshdr_io = SDL_IOFromFile(APP_VK_FRAGMENT_SHADER_SOURCE_PATH, "rb");
  if (!fshdr_io) {
    log_error("[app] SDL_IOFromFile: %s", SDL_GetError());
    return false;
  }

  size_t fshdr_fsz;
  void* fshdr_buf = SDL_LoadFile_IO(fshdr_io, &fshdr_fsz, true);
  if (!fshdr_buf) {
    log_error("[app] SDL_LoadFile_IO: %s", SDL_GetError());
    return false;
  }

  // CREATE SHADER MODULE
  VkShaderModuleCreateInfo vshdrm_ci = {
      .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = vshdr_fsz,
      .pCode    = vshdr_buf,
  };

  app_vkchk(vkCreateShaderModule(g_vkdev, &vshdrm_ci, NULL, &g_vkmainvshdrm),
            "vkCreateShaderModule");

  VkShaderModuleCreateInfo fshdrm_ci = {
      .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = fshdr_fsz,
      .pCode    = fshdr_buf,
  };

  app_vkchk(vkCreateShaderModule(g_vkdev, &fshdrm_ci, NULL, &g_vkmainfshdrm),
            "vkCreateShaderModule");

  // CLEANUP
  SDL_free(vshdr_buf);
  SDL_free(fshdr_buf);

  return true;
}

bool app_vkinit_descriptor_layout() {
  static_assert(APP_UTIL_ARRAY_LEN(g_vkmain_game_data) == APP_VK_MAX_FIFO,
                "Size of game tiles data is equivalent to N-BUFFER (MAX_FIFO)");

  VkDescriptorPoolSize dsetpoolszs[] = {
      (VkDescriptorPoolSize){
          .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = APP_UTIL_ARRAY_LEN(g_vkmain_game_data),
      },
      (VkDescriptorPoolSize){
          .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = APP_VK_MAX_TEXTURES * APP_VK_MAX_FIFO,  // Max textures indexed
      },
  };

  VkDescriptorPoolCreateInfo dsetpool_ci = {
      .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets       = APP_VK_MAX_FIFO,
      .poolSizeCount = 2,
      .pPoolSizes    = dsetpoolszs,
  };

  app_vkchk(vkCreateDescriptorPool(g_vkdev, &dsetpool_ci, NULL, &g_vkmaindsetpool),
            "vkCreateDescriptorPool");

  VkDescriptorSetLayoutBinding dsetlbs[] = {
      (VkDescriptorSetLayoutBinding){
          .binding            = 0,
          .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount    = 1,
          .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
          .pImmutableSamplers = NULL,
      },
      (VkDescriptorSetLayoutBinding){
          .binding            = 1,
          .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount    = 128,
          .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
          .pImmutableSamplers = NULL,
      },
  };

  VkDescriptorBindingFlags dbfs[] = {
      0,  // Shader Data
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
          VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT,  // Textures Data
  };

  VkDescriptorSetLayoutBindingFlagsCreateInfo dsetlbfs_ci = {
      .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount  = 2,
      .pBindingFlags = dbfs,
  };

  VkDescriptorSetLayoutCreateInfo dsetl_ci = {
      .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext        = &dsetlbfs_ci,
      .bindingCount = 2,
      .pBindings    = dsetlbs,
  };

  app_vkchk(vkCreateDescriptorSetLayout(g_vkdev, &dsetl_ci, NULL, &g_vkmaindsetlayout),
            "vkCreateDescriptorSetLayout");

  return true;
}

/*
 *
 * app_vkinit_pipeline
 *
 * */
bool app_vkinit_pipeline() {
  // PIPELINE LAYOUT
  VkPushConstantRange pushconstrange     = {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                                            .size       = APP_GAME_CAMERA_DATA_SZ};

  VkPipelineLayoutCreateInfo pllayout_ci = {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount         = 1,
      .pSetLayouts            = &g_vkmaindsetlayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges    = &pushconstrange,
  };

  app_vkchk(vkCreatePipelineLayout(g_vkdev, &pllayout_ci, NULL, &g_vkmainpllayout),
            "vkCreatePipelineLayout");

  // GRAPHIC PIPELINE
  VkPipelineShaderStageCreateInfo shdr_stages[] = {
      (VkPipelineShaderStageCreateInfo){
          .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage  = VK_SHADER_STAGE_VERTEX_BIT,
          .module = g_vkmainvshdrm,
          .pName  = "main",
      },
      (VkPipelineShaderStageCreateInfo){
          .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = g_vkmainfshdrm,
          .pName  = "main",
      },
  };

  // fovsjvo

  VkPipelineVertexInputStateCreateInfo vi_state = {
      .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount   = 0,
      .pVertexBindingDescriptions      = NULL,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions    = NULL,
  };

  VkPipelineInputAssemblyStateCreateInfo iass_state = {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport vp = {
      .x        = 0.f,
      .y        = 0.f,
      .width    = APP_MAIN_WINDOW_WIDTH,
      .height   = APP_MAIN_WINDOW_HEIGHT,
      .minDepth = 0.f,
      .maxDepth = 1.f,
  };

  VkRect2D scis = {
      .offset = {0, 0},
      .extent = {APP_MAIN_WINDOW_WIDTH, APP_MAIN_WINDOW_HEIGHT},
  };

  VkPipelineViewportStateCreateInfo vp_state = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports    = &vp,
      .scissorCount  = 1,
      .pScissors     = &scis,
  };

  VkPipelineRasterizationStateCreateInfo ras_state = {
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthBiasEnable         = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode             = VK_POLYGON_MODE_FILL,
      .lineWidth               = 1.f,
      .cullMode                = VK_CULL_MODE_NONE,  // VK_CULL_MODE_BACK_BIT,
      .frontFace               = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable         = VK_FALSE,
  };

  VkPipelineMultisampleStateCreateInfo ms_state = {
      .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable  = VK_FALSE,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineColorBlendAttachmentState colb_atchstate = {
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable         = VK_TRUE,

      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
  };

  VkPipelineColorBlendStateCreateInfo colb_state = {
      .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable   = VK_FALSE,
      .logicOp         = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments    = &colb_atchstate,
      .blendConstants  = {0.f, 0.f, 0.f, 0.f},
  };

  VkPipelineDepthStencilStateCreateInfo desten_state = {
      .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable  = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp   = VK_COMPARE_OP_ALWAYS,
  };

  VkPipelineRenderingCreateInfo rder_ci = {
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount    = 1,
      .pColorAttachmentFormats = &g_vkscimgfmt,
  };

  VkGraphicsPipelineCreateInfo pl_ci = {
      .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext               = &rder_ci,
      .stageCount          = 2,
      .pStages             = shdr_stages,
      .pVertexInputState   = &vi_state,
      .pInputAssemblyState = &iass_state,
      .pViewportState      = &vp_state,
      .pRasterizationState = &ras_state,
      .pMultisampleState   = &ms_state,
      .pColorBlendState    = &colb_state,
      .pDepthStencilState  = &desten_state,
      .renderPass          = VK_NULL_HANDLE,
      .layout              = g_vkmainpllayout,
  };

  app_vkchk(vkCreateGraphicsPipelines(g_vkdev, VK_NULL_HANDLE, 1, &pl_ci, NULL, &g_vkmainpl),
            "vkCreateGraphicsPipelines");

  return true;
}

bool app_vkinit_static_render_data() {
  // INIT TILES DATA (SSBOs)

  // DESCRIPTOR DATA
  for (size_t i = 0; i < APP_VK_MAX_FIFO; ++i) {
    // PREPAPARE BUFFER
    VkBufferCreateInfo buf_ci = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = APP_VK_MAIN_GAME_DATA_BUFFER_SZ,
        .usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo vma_aci = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo vma_ai = {0};
    app_vkchk(vmaCreateBuffer(g_vkvma_allocator, &buf_ci, &vma_aci, &g_vkmain_game_data[i].ssbobuf,
                              &g_vkmain_game_data[i].ssboalloc, &vma_ai),
              "vmaCreateBuffer");

    g_vkmain_game_data[i].pmapped_data = vma_ai.pMappedData;

    // LINK ALLOCATION TO DESCRIPTOR SETS
    uint32_t maxbindingcount[]                                        = {128};
    VkDescriptorSetVariableDescriptorCountAllocateInfo dsetvdcount_ai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = 1,
        .pDescriptorCounts  = maxbindingcount,
    };

    VkDescriptorSetAllocateInfo dset_ai = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = &dsetvdcount_ai,
        .descriptorPool     = g_vkmaindsetpool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &g_vkmaindsetlayout,
    };

    // g_vkmain_game_data is per frame data (g_vk_frames)
    app_vkchk(vkAllocateDescriptorSets(g_vkdev, &dset_ai, &g_vkmain_game_data[i].dset),
              "vkAllocateDescriptorSets");

    // WRITE DATA
    VkDescriptorBufferInfo dset_bufi = {
        .buffer = g_vkmain_game_data[i].ssbobuf,
        .offset = 0,
        .range  = APP_VK_MAIN_GAME_DATA_BUFFER_SZ,
    };

    VkWriteDescriptorSet wdset = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = g_vkmain_game_data[i].dset,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo     = &dset_bufi,
    };

    vkUpdateDescriptorSets(g_vkdev, 1, &wdset, 0, NULL);
  }

  app_game_render_data_t* game_data_mapped_data[] = {
      (app_game_render_data_t*)g_vkmain_game_data[0].pmapped_data,
      (app_game_render_data_t*)g_vkmain_game_data[1].pmapped_data,
  };

  if (game_data_mapped_data[0] == NULL) {
    log_error("[app] g_vkmain_game_data[%d].pmapped_data", 0);
    return false;
  }

  if (game_data_mapped_data[1] == NULL) {
    log_error("[app] g_vkmain_game_data[%d].pmapped_data", 1);
    return false;
  }

  // ADD INSTANCE
  app_game_render_data_t static_data[] = {
      // BACKGROUND
      (app_game_render_data_t){
          .coordx  = 0.f,
          .coordy  = 0.f,

          .colorr  = 1.f,
          .colorg  = 1.f,
          .colorb  = 1.f,
          .colora  = 1.f,

          .scalex  = APP_GAME_GRID_DIMENSION_X,
          .scaley  = APP_GAME_GRID_DIMENSION_Y,

          .rot     = 0.f,

          .transx  = 0.f,
          .transy  = 0.f,

          .texslot = 0,
          .texid   = 1,
      },

  };

  const size_t GAME_COL = APP_GAME_GRID_MAX_COL;
  const size_t GAME_ROW = APP_GAME_GRID_MAX_ROW;

  app_game_render_data_t tiles_data[GAME_COL * GAME_ROW];

  // HEXAGON GRID
  for (size_t col = 0; col < GAME_COL; ++col) {
    for (size_t row = 0; row < GAME_ROW; ++row) {

      float x                 = 3. / 2. * col;
      float y                 = SDL_sqrt(3) * (row + .5f * (col & 1));

      x                       = x * APP_GAME_TILE_RADIUS;
      y                       = y * APP_GAME_TILE_RADIUS;

      size_t idx              = (col * GAME_ROW) + row;

      tiles_data[idx].coordx  = (x - ((APP_GAME_GRID_DIMENSION_X / 2.f) - APP_GAME_TILE_RADIUS));
      tiles_data[idx].coordy  = (y - ((APP_GAME_GRID_DIMENSION_Y / 2.f) - APP_GAME_TILE_INRADIUS));
      tiles_data[idx].colorr  = .15f;
      tiles_data[idx].colorg  = .15f;
      tiles_data[idx].colorb  = .15f;
      tiles_data[idx].colora  = 1.f;
      tiles_data[idx].scalex  = APP_GAME_TILE_SCALE_X;
      tiles_data[idx].scaley  = APP_GAME_TILE_SCALE_Y;
      tiles_data[idx].rot     = 0.f;
      tiles_data[idx].transx  = 0;
      tiles_data[idx].transy  = 0;
      tiles_data[idx].texslot = 1;
      tiles_data[idx].texid   = 1;

      // log_debug("=============================");
      // log_debug("[app] tiles_data[%zu]: ", idx);
      // log_debug("[app] coordx: %zu coordy: %zu ", col, row);
      // log_debug("[app] transx: %f transy: %f ", tiles_data[idx].transx, tiles_data[idx].transy);
    }
  }

  app_game_render_data_t units_data[] = {};

  // DOUBLE BUFFER
  for (size_t i = 0; i < APP_VK_MAX_FIFO; ++i) {
    memcpy(game_data_mapped_data[i], static_data, sizeof(static_data));
    memcpy(game_data_mapped_data[i] + APP_GAME_RENDER_TILES_DATA_OFFSET, tiles_data,
           sizeof(tiles_data));
    memcpy(game_data_mapped_data[i] + APP_GAME_RENDER_UNIT_DATA_OFFSET, units_data,
           sizeof(units_data));
  }

  g_game_static_enitites_count += SDL_arraysize(static_data);
  g_game_tiles_entities_count += SDL_arraysize(tiles_data);
  g_game_units_entities_count += SDL_arraysize(units_data);

  g_game_object_count +=
      g_game_static_enitites_count + g_game_tiles_entities_count + g_game_units_entities_count;

  log_debug("[app] draw %d instances", g_game_object_count);

  return true;
}

/*
 *
 * app_vkinit_frame_render_data
 *
 * */
bool app_vkinit_frame_render_data() {
  if ((g_game_units_entities_flag & APP_GAME_ENTITY_DIRTY_FLAG) == 0) {
    return true;
  }

  log_debug("[app] %s: init_frame_data", __FUNCTION__);

  const size_t GAME_COL = APP_GAME_GRID_MAX_COL;
  const size_t GAME_ROW = APP_GAME_GRID_MAX_ROW;

  for (size_t i = 0; i < APP_VK_MAX_FIFO; ++i) {
    app_game_render_data_t* game_data_mapped_data =
        (app_game_render_data_t*)g_vkmain_game_data[i].pmapped_data;

    if (game_data_mapped_data == NULL) {
      log_error("[app] g_vkmain_game_data[%d].pmapped_data", 0);
      return false;
    }

    for (size_t entt_idx = 0; entt_idx < g_game_units_entities_count; ++entt_idx) {
      size_t tile_idx =
          (g_game_coord_comp[entt_idx].col * GAME_ROW) + g_game_coord_comp[entt_idx].row;

      log_debug(
          "[app] %s: col=%d row=%d tile_idx=%d scalex=%.2f scaley=%.2f transx=%.2f transy=%.2f",
          __FUNCTION__, g_game_coord_comp[entt_idx].col, g_game_coord_comp[entt_idx].row, tile_idx,
          g_game_transform_comp[entt_idx].scalex, g_game_transform_comp[entt_idx].scaley, tile_idx,
          g_game_transform_comp[entt_idx].transx, g_game_transform_comp[entt_idx].transy, tile_idx);

      app_game_render_data_t* p_unit =
          &game_data_mapped_data[APP_GAME_RENDER_UNIT_DATA_OFFSET + entt_idx];
      app_game_render_data_t* p_tile =
          &game_data_mapped_data[APP_GAME_RENDER_TILES_DATA_OFFSET + tile_idx];

      p_unit->coordx  = p_tile->coordx;
      p_unit->coordy  = p_tile->coordy;

      p_unit->colorr  = 1.f;
      p_unit->colorg  = 1.f;
      p_unit->colorb  = 1.f;
      p_unit->colora  = 1.f;

      p_unit->rot     = g_game_transform_comp[entt_idx].rot;

      p_unit->scalex  = g_game_transform_comp[entt_idx].scalex * APP_MAIN_WINDOW_SCALE;
      p_unit->scaley  = g_game_transform_comp[entt_idx].scaley * APP_MAIN_WINDOW_SCALE;

      p_unit->transx  = g_game_transform_comp[entt_idx].transx * APP_MAIN_WINDOW_SCALE;
      p_unit->transy  = g_game_transform_comp[entt_idx].transy * APP_MAIN_WINDOW_SCALE;

      p_unit->texslot = g_game_render_comp[entt_idx].texslot;
      p_unit->texid   = g_game_render_comp[entt_idx].texid;

      log_debug("[app] %s: texid=%d", __FUNCTION__, g_game_render_comp[entt_idx].texid);
    }
  }

  g_game_object_count += g_game_units_entities_count;

  g_game_units_entities_flag &= ~APP_GAME_ENTITY_DIRTY_FLAG;

  return true;
}

/*
 *
 * app_vkninit_mesh_assets
 *
 * */
bool app_vkinit_mesh_assets() {
  return true;
}

/*
 *
 * app_vkinit_tex_assets
 *
 * */
bool app_vkinit_tex_assets() {
  // INIT TEXTURES (bindless uniform sampler2DArray)
  // UPLOAD COMMAND BUFFER
  VkFence uploadfnc;
  VkFenceCreateInfo fnc_ci = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
  };
  app_vkchk(vkCreateFence(g_vkdev, &fnc_ci, NULL, &uploadfnc), "vkCreateFence");

  VkCommandBuffer uploadcb;
  VkCommandBufferAllocateInfo cb_ai = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = g_vkcp,
      .commandBufferCount = 1,
  };

  app_vkchk(vkAllocateCommandBuffers(g_vkdev, &cb_ai, &uploadcb), "vkAllocateCommandBuffers");

  VkCommandBufferBeginInfo cb_bi = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  app_vkchk(vkBeginCommandBuffer(uploadcb, &cb_bi), "vkBeginCommandBuffer");

  VkBuffer stagingbuf[APP_GAME_TEXTURE_ARRAY_COUNT]        = {};
  VmaAllocation stagingalloc[APP_GAME_TEXTURE_ARRAY_COUNT] = {};

  // UPLOAD EACH TEXTURE ARRAY
  for (size_t i = 0; i < APP_GAME_TEXTURE_ARRAY_COUNT; ++i) {
    // CREATE TEXTURE OBJECT
    SDL_IOStream* io = SDL_IOFromFile(APP_GAME_TEXTURE_ARRAY_SOURCE_PATH[i], "rb");
    if (!io) {
      log_error("[app] SDL_IOFromFile: %s", SDL_GetError());
      return false;
    }

    size_t filesz;
    void* buf = SDL_LoadFile_IO(io, &filesz, true);
    if (!buf) {
      log_error("[app] SDL_LoadFile_IO: %s", SDL_GetError());
      return false;
    }
    ktxTexture2* ptex    = NULL;
    KTX_error_code texec = ktxTexture2_CreateFromMemory(  //
        (const ktx_uint8_t*)buf,                          //
        filesz,                                           //
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,           //
        &ptex                                             //
    );
    if (texec != KTX_SUCCESS) {
      log_error(                                                        //
          "[app] ktxTexture2_CreateFromMemory: %s:%s",                  //
          APP_GAME_TEXTURE_ARRAY_SOURCE_PATH[i], ktxErrorString(texec)  //
      );

      return false;
    }

    log_debug("=============================================");
    log_debug("[app] TEXTURE: %s", APP_GAME_TEXTURE_ARRAY_SOURCE_PATH[i]);
    log_debug("[app] ptex: classId %d", ptex->classId);
    log_debug("[app] ptex: vkFormat %d", ptex->vkFormat);
    log_debug("[app] ptex: isArray ? %s", ptex->isArray ? "Yes" : "No");
    log_debug("[app] ptex: isCompressed? %s", ptex->isCompressed ? "Yes" : "No");
    log_debug("[app] ptex: dataSize %d", ptex->dataSize);
    log_debug("[app] ptex: baseDepth %d", ptex->baseDepth);
    log_debug("[app] ptex: numLayers %d", ptex->numLayers);
    log_debug("[app] ptex: numLevels %d", ptex->numLevels);
    log_debug("[app] ptex: baseWidth %d", ptex->baseWidth);
    log_debug("[app] ptex: baseHeight %d", ptex->baseHeight);
    log_debug("=============================================");

    if (ktxTexture2_NeedsTranscoding(ptex)) {
      log_debug("[app] ptex: need transcode %s", APP_GAME_TEXTURE_ARRAY_SOURCE_PATH[i]);
      ktx_transcode_fmt_e tcodefmt = KTX_TTF_BC7_RGBA;

      KTX_error_code tcodeec       = ktxTexture2_TranscodeBasis(ptex, tcodefmt, 0);

      if (tcodeec != KTX_SUCCESS) {
        log_error("[app] Failed to transcode UASTC texture: %s:%s",
                  APP_GAME_TEXTURE_ARRAY_SOURCE_PATH[i], ktxErrorString(tcodeec));
        return false;
      }
    }

    VkImageCreateInfo img_ci = {
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format    = ptex->vkFormat,
        .extent =
            {
                .width  = ptex->baseWidth,
                .height = ptex->baseHeight,
                .depth  = 1,
            },
        .mipLevels     = ptex->numLevels,  // APP_VK_MAIN_GAME_TEXTURE_ARRAY_MIPLEVELS,
        .arrayLayers   = ptex->numLayers,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo alloc_ci = {.usage = VMA_MEMORY_USAGE_AUTO};
    app_vkchk(vmaCreateImage(g_vkvma_allocator, &img_ci, &alloc_ci, &g_vkmain_game_texs[i].img,
                             &g_vkmain_game_texs[i].alloc, NULL),
              "vmaCreateImage");

    VkImageViewCreateInfo imgv_ci = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = g_vkmain_game_texs[i].img,
        .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        .format   = img_ci.format,
        // .format = VK_FORMAT_BC7_RGBA_SRGB_BLOCK,
        .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = ptex->numLevels,
                .baseArrayLayer = 0,
                .layerCount     = ptex->numLayers,
            },
    };

    app_vkchk(vkCreateImageView(g_vkdev, &imgv_ci, NULL, &g_vkmain_game_texs[i].view),
              "vkCreateImageView");
    g_vkmain_game_texs[i].layer_count = ptex->numLayers;

    // UPLOAD
    {

      VkBufferCreateInfo buf_ci = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size  = ptex->dataSize,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      };
      VmaAllocationCreateInfo vma_aci = {
          .flags = (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT),
          .usage = VMA_MEMORY_USAGE_AUTO,
      };
      VmaAllocationInfo vma_ai = {};

      app_vkchk(vmaCreateBuffer(g_vkvma_allocator, &buf_ci, &vma_aci, &stagingbuf[i],
                                &stagingalloc[i], &vma_ai),
                "vmaCreateBuffer");

      memcpy(vma_ai.pMappedData, ptex->pData, ptex->dataSize);

      // SOURCE TO STAGING
      VkImageMemoryBarrier2 teximg_barrier = {
          .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
          .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
          .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          .image         = g_vkmain_game_texs[i].img,
          .subresourceRange =
              {
                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                  .levelCount = ptex->numLevels,
                  .layerCount = ptex->numLayers,
              },
      };

      VkDependencyInfo teximg_di = {
          .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
          .imageMemoryBarrierCount = 1,
          .pImageMemoryBarriers    = &teximg_barrier,
      };

      // COPY DATA TO SHADER
      vkCmdPipelineBarrier2(uploadcb, &teximg_di);

      uint32_t region_count = ptex->numLevels;
      VkBufferImageCopy2* cpregions =
          (VkBufferImageCopy2*)SDL_calloc(region_count, sizeof(VkBufferImageCopy2));

      for (uint32_t lv = 0; lv < ptex->numLevels; ++lv) {
        ktx_size_t offset = 0;
        ktxTexture2_GetImageOffset(ptex, lv, 0, 0, &offset);

        uint32_t mipw = SDL_max(1, ptex->baseWidth >> lv);
        uint32_t miph = SDL_max(1, ptex->baseHeight >> lv);

        cpregions[lv] = (VkBufferImageCopy2){
            .sType        = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
            .bufferOffset = offset,
            .imageSubresource =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel       = lv,
                    .baseArrayLayer = 0,
                    .layerCount     = ptex->numLayers,
                },
            .imageExtent =
                {
                    .width  = mipw,
                    .height = miph,
                    .depth  = 1,
                },

        };
      }

      VkCopyBufferToImageInfo2 cpinfo = {
          .sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
          .srcBuffer      = stagingbuf[i],
          .dstImage       = g_vkmain_game_texs[i].img,
          .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          .regionCount    = region_count,
          .pRegions       = cpregions,
      };

      vkCmdCopyBufferToImage2(uploadcb, &cpinfo);

      // STAGING TO SHADER
      VkImageMemoryBarrier2 texread_barrier = {
          .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
          .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
          .dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
          .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
          .oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          .newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .image         = g_vkmain_game_texs[i].img,
          .subresourceRange =
              {
                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                  .levelCount = ptex->numLevels,
                  .layerCount = ptex->numLayers,
              },
      };

      VkDependencyInfo texread_di = {
          .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
          .imageMemoryBarrierCount = 1,
          .pImageMemoryBarriers    = &texread_barrier,
      };

      vkCmdPipelineBarrier2(uploadcb, &texread_di);

      SDL_free(cpregions);
    }

    ktxTexture2_Destroy(ptex);
    SDL_free(buf);
  }

  app_vkchk(vkEndCommandBuffer(uploadcb), "vkEndCommandBuffer");
  VkSubmitInfo si = {
      .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers    = &uploadcb,
  };
  app_vkchk(vkQueueSubmit(g_vkqueue, 1, &si, uploadfnc), "vkQueueSubmit");
  app_vkchk(vkWaitForFences(g_vkdev, 1, &uploadfnc, VK_TRUE, UINT64_MAX), "vkWaitForFences");

  // INIT DESCRIPTOR
  VkSampler texsampler;
  VkSamplerCreateInfo sampler_ci = {
      .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter        = VK_FILTER_LINEAR,
      .minFilter        = VK_FILTER_LINEAR,

      .addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT,

      .minLod           = 0.0f,
      .maxLod           = 11.f,
      .mipLodBias       = 0.0f,

      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy    = 16.f,

      .mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
  };
  app_vkchk(vkCreateSampler(g_vkdev, &sampler_ci, NULL, &texsampler), "vkCreateSampler");

  for (size_t i = 0; i < APP_GAME_TEXTURE_ARRAY_COUNT; ++i) {
    VkDescriptorImageInfo dsetimginfo = {
        .sampler     = texsampler,
        .imageView   = g_vkmain_game_texs[i].view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    // DOUBLE BUFFER UPDATE
    for (size_t frame = 0; frame < APP_VK_MAX_FIFO; ++frame) {
      VkWriteDescriptorSet wdset = {
          .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet          = g_vkmain_game_data[frame].dset,
          .dstBinding      = 1,
          .dstArrayElement = (uint32_t)i,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo      = &dsetimginfo,
      };

      vkUpdateDescriptorSets(g_vkdev, 1, &wdset, 0, NULL);
    }
  }

  // CLEANUP STAGING BUFFER
  for (size_t i = 0; i < APP_GAME_TEXTURE_ARRAY_COUNT; ++i) {
    vmaDestroyBuffer(g_vkvma_allocator, stagingbuf[i], stagingalloc[i]);
  }

  vkFreeCommandBuffers(g_vkdev, g_vkcp, 1, &uploadcb);
  vkDestroyFence(g_vkdev, uploadfnc, NULL);

  return true;
}

/*
 *
 * app_vkhandle_event
 *
 * */
bool app_vkhandle_event(SDL_Event* p_ev) {
  app_vkgui_event(p_ev);
  return true;
}

/*
 * RENDER
 * app_vkbegin_render 
 *
 * */
bool app_vkbegin_render() {

  // Wait for the specific "Frame in Flight" to be finished by the GPU
  app_vkchk(vkWaitForFences(g_vkdev, 1, &g_vkfncs[g_vkframe_idx], VK_TRUE, UINT64_MAX),
            "vkWaitForFences");
  app_vkchk(vkResetFences(g_vkdev, 1, &g_vkfncs[g_vkframe_idx]), "vkResetFences");

  app_vkchk_sc(vkAcquireNextImageKHR(g_vkdev, g_vksc, UINT64_MAX, g_vkpresent_sems[g_vkframe_idx],
                                     VK_NULL_HANDLE, &g_vkimg_idx),
               &g_vkupdate_sc);

  // BEGIN RENDER COMMAND
  VkCommandBuffer cb = g_vkcb[g_vkframe_idx];
  app_vkchk(vkResetCommandBuffer(cb, 0), "vkResetCommandBuffer");
  VkCommandBufferBeginInfo cb_bi = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  app_vkchk(vkBeginCommandBuffer(cb, &cb_bi), "vkBeginCommandBuffer");

  // PREPARE PIPELINE
  VkImageMemoryBarrier2 barrier = {
      .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .image         = g_vksc_imgs[g_vkimg_idx],  // <-- swapchain images
      .subresourceRange =
          {
              .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel   = 0,
              .levelCount     = 1,
              .baseArrayLayer = 0,
              .layerCount     = 1,
          },
  };

  VkDependencyInfo barrier_di = {
      .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers    = &barrier,
  };
  vkCmdPipelineBarrier2(cb, &barrier_di);

  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkmainpl);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkmainpllayout, 0, 1,
                          &g_vkmain_game_data[g_vkframe_idx].dset, 0, NULL);

  app_game_camera_data_t current_frame_cam = g_game_cam;
  vkCmdPushConstants(cb, g_vkmainpllayout, VK_SHADER_STAGE_VERTEX_BIT, 0, APP_GAME_CAMERA_DATA_SZ,
                     &current_frame_cam);

  // BEGIN RENDER
  VkRenderingAttachmentInfo col_atchi = {
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView   = g_vksc_img_views[g_vkimg_idx],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue =
          {
              .color = {g_appctx.clrscr[0], g_appctx.clrscr[1], g_appctx.clrscr[2], 1.f},
          },
  };

  VkRenderingInfo ri = {
      .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea           = {.extent = {.width  = (uint32_t)APP_MAIN_WINDOW_WIDTH,
                                          .height = (uint32_t)APP_MAIN_WINDOW_HEIGHT}},
      .layerCount           = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &col_atchi,
  };

  // BEGIN RENDER
  vkCmdBeginRendering(cb, &ri);
  app_vkgui_begin();

  // vkCmdDraw(cb, 3, 1, 0, 0);
  vkCmdDraw(cb, 6, g_game_object_count, 0, 0);

  app_guimain(&g_appctx);

  // END RENDER
  app_vkgui_end(&cb);
  vkCmdEndRendering(cb);

  // Barrier: Transition to PRESENT_SRC_KHR so the screen can show it
  barrier.oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstStageMask  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  barrier.dstAccessMask = 0;
  vkCmdPipelineBarrier2(cb, &barrier_di);
  app_vkchk(vkEndCommandBuffer(cb), "vkEndCommandBuffer");

  // SUBMIT & PRESENT
  VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo si                  = {
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount   = 1,
      .pWaitSemaphores      = &g_vkpresent_sems[g_vkframe_idx],
      .pWaitDstStageMask    = &wait_stages,
      .commandBufferCount   = 1,
      .pCommandBuffers      = &cb,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores    = &g_vkrender_sems[g_vkimg_idx],
  };
  app_vkchk(vkQueueSubmit(g_vkqueue, 1, &si, g_vkfncs[g_vkframe_idx]), "vkQueueSubmit");
  g_vkframe_idx       = (g_vkframe_idx + 1) % APP_VK_MAX_FIFO;
  VkPresentInfoKHR pi = {
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores    = &g_vkrender_sems[g_vkimg_idx],
      .swapchainCount     = 1,
      .pSwapchains        = &g_vksc,
      .pImageIndices      = &g_vkimg_idx,
  };
  app_vkchk_sc(vkQueuePresentKHR(g_vkqueue, &pi), "vkQueuePresentKHR");

  return true;
}

/*
 *
 * app_vkdestroy
 *
 *
 * */
bool app_vkdestroy() {

  app_vkchk(vkDeviceWaitIdle(g_vkdev), "vkDeviceWaitIdle");

  // DESTROY GUI
  app_chk(app_vkgui_destroy(), "app_vkgui_destroy");

  vkDestroyShaderModule(g_vkdev, g_vkmainvshdrm, NULL);
  vkDestroyShaderModule(g_vkdev, g_vkmainfshdrm, NULL);

  if (g_vkdev != VK_NULL_HANDLE) {
    for (size_t i = 0; i < g_vkfncs_sz; ++i) {
      vkDestroyFence(g_vkdev, g_vkfncs[i], NULL);
      vkDestroySemaphore(g_vkdev, g_vkpresent_sems[i], NULL);
    }
    for (size_t i = 0; i < g_vkrender_sems_sz; ++i) {
      vkDestroySemaphore(g_vkdev, g_vkrender_sems[i], NULL);
    }
    for (size_t i = 0; i < g_vkimg_cnt; i++) {
      vkDestroyImageView(g_vkdev, g_vksc_img_views[i], NULL);
    }

    for (size_t i = 0; i < APP_VK_MAX_FIFO; ++i) {
      vmaDestroyBuffer(g_vkvma_allocator, g_vkmain_game_data[i].ssbobuf,
                       g_vkmain_game_data[i].ssboalloc);
    }

    for (size_t i = 0; i < APP_GAME_TEXTURE_ARRAY_COUNT; ++i) {
      vkDestroyImageView(g_vkdev, g_vkmain_game_texs[i].view, NULL);
      vkDestroySampler(g_vkdev, g_vkmain_game_texs[i].sampler, NULL);
      vmaDestroyImage(g_vkvma_allocator, g_vkmain_game_texs[i].img, g_vkmain_game_texs[i].alloc);
    }

    vkDestroySwapchainKHR(g_vkdev, g_vksc, NULL);
    vkDestroyCommandPool(g_vkdev, g_vkcp, NULL);

    vmaDestroyAllocator(g_vkvma_allocator);
    vkDestroyDevice(g_vkdev, NULL);
  }

  if (g_vksurface != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(g_vkinst, g_vksurface, NULL);

  if (g_vkinst != VK_NULL_HANDLE)
    vkDestroyInstance(g_vkinst, NULL);

  return true;
}

/*
 * 
 *
 * app_gameev_camera_control
 *
 * */
bool app_gameev_camera_control(const SDL_Event* p_ev) {
  static bool isdragging  = false;
  static float lastmousex = 0;
  static float lastmousey = 0;

  const float zoom        = g_game_cam.zoom;
  const float camx        = g_game_cam.posx;
  const float camy        = g_game_cam.posy;

  if (p_ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    isdragging = true;
    lastmousex = p_ev->button.x;
    lastmousey = p_ev->button.y;
  } else if (p_ev->type == SDL_EVENT_MOUSE_BUTTON_UP) {
    isdragging = false;
  } else if (p_ev->type == SDL_EVENT_MOUSE_MOTION && isdragging) {
    const float dx = (p_ev->motion.x - lastmousex) / zoom;
    const float dy = (p_ev->motion.y - lastmousey) / zoom;

    g_game_cam.posx -= dx;
    g_game_cam.posy -= dy;

    lastmousex = p_ev->motion.x;
    lastmousey = p_ev->motion.y;
  } else if (p_ev->type == SDL_EVENT_KEY_DOWN) {
    switch (p_ev->key.key) {
      case SDLK_LEFT: {
        g_game_cam.posx -= APP_GAME_TILE_SPACE_HORIZ;
      } break;
      case SDLK_RIGHT: {
        g_game_cam.posx -= APP_GAME_TILE_SPACE_HORIZ;
      } break;
    }
  }

  /* KEYBOARD ZOOM LOGIC */
  DESKTOP_ONLY({
    if (p_ev->type == SDL_EVENT_MOUSE_WHEEL) {
      g_game_cam.zoom = APP_UTIL_CLAMP(zoom + (p_ev->wheel.y * .1f), 1.f, 4.f);
    }
  });

  /*
 * TOUCH SCREEN ZOOM PINCH LOGIC BY GOOGLE GEMINI
 * * Logic Overview:
 * 1. State Tracking: Maintains a relative scale by capturing the initial 
 * distance between two fingers (g_start_distance) and the camera's
 * starting zoom level (g_start_zoom) upon the start of a pinch.
 *
 * 2. Multiplicative Scaling: Uses a distance ratio (cur_distance / start_distance)
 * to multiply the start_zoom, allowing for fluid, bi-directional 
 * zooming (in and out) rather than additive offsets.
 *
 * 3. Event-Driven: Specifically designed for SDL3 touch events, avoiding 
 * deprecated API queries by tracking finger state via 
 * SDL_EVENT_FINGER_DOWN/MOTION/UP.
 * * Integration:
 * - Update g_game_cam.zoom within the render loop using the derived scale factor.
 * - Ensure coordinate mapping for multi-touch uses SDL3 fingerId tracking.
 */
  ANDROID_ONLY({
    static struct {
      float x;
      float y;
      bool active;
    } fingers[2]                = {0};

    static float start_distance = 0.0f;
    static float start_zoom     = 1.0f;
    static bool is_pinching     = false;

    if (p_ev->type == SDL_EVENT_FINGER_DOWN ||  //
        p_ev->type == SDL_EVENT_FINGER_MOTION) {
      int idx = (int)p_ev->tfinger.fingerID - 1;
      log_debug("[app] SDL_EVENT_FINGER_DOWN || SDL_EVENT_FINGER_MOTION: idx %d", idx);
      if (idx < 2) {
        fingers[idx].x      = p_ev->tfinger.x;
        fingers[idx].y      = p_ev->tfinger.y;
        fingers[idx].active = true;
      }
    } else if (p_ev->type == SDL_EVENT_FINGER_UP) {
      int idx = (int)p_ev->tfinger.fingerID - 1;
      log_debug("[app] SDL_EVENT_FINGER_UP: idx %d", idx);
      if (idx < 2)
        fingers[idx].active = false;
    }

    if (fingers[0].active && fingers[1].active && !is_pinching) {
      is_pinching    = true;
      start_zoom     = g_game_cam.zoom;

      float dx       = fingers[0].x - fingers[1].x;
      float dy       = fingers[0].y - fingers[1].y;
      start_distance = sqrt(dx * dx + dy * dy);

      log_debug("[app] fingers is_pinching: start_distance %f ", start_distance);
    }

    if (is_pinching && fingers[0].active && fingers[1].active) {
      float dx           = fingers[0].x - fingers[1].x;
      float dy           = fingers[0].y - fingers[1].y;
      float cur_distance = sqrtf(dx * dx + dy * dy);

      log_debug("[app] fingers start zoom: cur_distance %f ", cur_distance);
      // Prevent division by zero
      if (start_distance > 0.001f) {
        // The ratio of current distance to start distance
        float ratio = cur_distance / start_distance;

        // Multiply original zoom by ratio to get the new zoom
        g_game_cam.zoom = APP_UTIL_CLAMP(start_zoom * ratio, 1.0f, 4.0f);

        log_debug("[app] fingers is_pinching: ratio %f ", ratio);
      }
    } else {
      is_pinching = false;
    }
  });

  return true;
}

/*
 *
 *
 * ecs_system_game_unit_selection_system
 *
 * handling tile grid touch -> on unit event
 *
 * grid is 32 columns (r) by 16 row (q) flat top odd-q hex grid
 * on top of 1920 by 1080 world coordinate
 *
 *
 * */
bool app_gameev_unit_selection(const SDL_Event* p_ev) {
  if (p_ev->type != SDL_EVENT_MOUSE_BUTTON_DOWN)
    return true;

  static float finalx     = 0;
  static float finaly     = 0;

  const float zoom_factor = g_game_cam.zoom;  // Use it so mousex and mousey still align
  const float scrcenterx  = (g_game_cam.resow / 2.f);
  const float scrcentery  = (g_game_cam.resoh / 2.f);

  const float offsetx     = p_ev->button.x - scrcenterx;
  const float offsety     = p_ev->button.y - scrcentery;

  log_debug("[app] offsetx: %.2f", offsetx);
  log_debug("[app] offsety: %.2f", offsety);

  const float worldx =
      ((offsetx / zoom_factor) + scrcenterx + g_game_cam.posx) - APP_GAME_TILE_RADIUS;
  const float worldy =
      ((offsety / zoom_factor) + scrcentery + (g_game_cam.posy)) + APP_GAME_TILE_INRADIUS;

  log_debug("[app] worldx: %.2f", worldx);
  log_debug("[app] worldy: %.2f", worldy);

  // PIXEL TO HEX https://www.redblobgames.com/grids/hexagons/#rounding
  float x = worldx / APP_GAME_TILE_RADIUS;
  float y = worldy / APP_GAME_TILE_RADIUS;

  float q = (2.f / 3.f * x);
  float r = (-1.f / 3.f * x + SDL_sqrt(3.f) / 3.f * y);

  // AXIAL ROUND https://observablehq.com/@jrus/hexround
  const float xgrid = SDL_round(q);
  const float ygrid = SDL_round(r);
  q -= xgrid;
  r -= ygrid;

  const float dx = SDL_round(q + .5f * r) * (q * q >= r * r);
  const float dy = SDL_round(r + .5f * q) * (q * q < r * r);

  finalx         = xgrid + dx;
  finaly         = ygrid + dy - 1;

  // log_debug("[app] finalx: %.2f", finalx);
  // log_debug("[app] finaly: %.2f", finaly);

  SDL_Event ev;
  SDL_zero(ev);
  ev.type       = SDL_EVENT_USER;
  ev.user.code  = 1001;
  ev.user.data1 = (void*)(intptr_t)finalx;
  ev.user.data2 = (void*)(intptr_t)finaly;

  SDL_PushEvent(&ev);

  return true;
}

/*
 *
 *
 * ecs_system_game_unit_selection_system
 *
 * handling tile grid touch -> on unit event
 *
 * grid is 32 columns (r) by 16 row (q) flat top odd-q hex grid
 * on top of 1920 by 1080 world coordinate
 *
 *
 * */
bool app_gameev_unit_action(const SDL_Event* p_ev) {

  if (p_ev->type == SDL_EVENT_USER && p_ev->user.code == 1001) {
    int q = (int)(intptr_t)p_ev->user.data1;
    int r = (int)(intptr_t)p_ev->user.data2;

    log_debug("[app] %s: Selected (axial) q %d r %d", __FUNCTION__, q, r);

    // CONVERT AXIAL TO OFFSET https://www.redblobgames.com/grids/hexagons/#conversions
    const int parity = q & 1;
    const int col    = q;
    const int row    = r + (q - parity) / 2;

    if ((col >= 0 && col < APP_GAME_GRID_MAX_COL) && (row >= 0 && row < APP_GAME_GRID_MAX_ROW)) {
      log_debug("[app] %s: Selected (offset) col %d row %d", __FUNCTION__, col, row);

      for (size_t i = 0; i < APP_VK_MAX_FIFO; ++i) {
        app_game_render_data_t* game_data_mapped_data =
            (app_game_render_data_t*)g_vkmain_game_data[i].pmapped_data;  // array

        if (game_data_mapped_data == NULL) {
          log_error("[app] g_vkmain_game_data[%d].pmapped_data", i);
          return false;
        }

        app_game_render_data_t* selected =
            &game_data_mapped_data[APP_GAME_RENDER_TILES_DATA_OFFSET +
                                   (col * APP_GAME_GRID_MAX_ROW) + row];
        // game_data_mapped_data + ((APP_GAME_RENDER_TILES_DATA_OFFSET + col + row));

        selected->colorr = (selected->colorr == 1.f) ? .15f : 1.f;
        selected->colorg = (selected->colorg == 0.f) ? .15f : 0.f;
        selected->colorb = (selected->colorb == 0.f) ? .15f : 0.f;
      }
    }
  }

  return true;
}

/*
 *
 *
 * APP_GAME
 * 
 *
 * */
bool app_gameinit() {
  g_game_units_entities_count = SDL_arraysize(g_app_res_demo_scenario_data.initial_units);

  for (size_t i = 0; i < g_game_units_entities_count; ++i) {
    g_game_ents[i].bitset           = APP_GAME_COORD_COMPONENT_BIT | APP_GAME_RENDER_COMPONENT_BIT |
                                      APP_GAME_TRANSFORM_COMPONENT_BIT;
    g_game_coord_comp[i].col        = g_app_res_demo_scenario_data.initial_units[i].col;
    g_game_coord_comp[i].row        = g_app_res_demo_scenario_data.initial_units[i].row;

    const uint64_t UNIT_ID          = g_app_res_demo_scenario_data.initial_units[i].id;

    g_game_render_comp[i].texslot   = APP_VK_MAIN_GAME_TEXTURE_ARRAY_1024X1024_SLOT;
    g_game_render_comp[i].texid     = APP_RES_UNIT_DATA[UNIT_ID].texid;
    g_game_transform_comp[i].rot    = APP_RES_UNIT_DATA[UNIT_ID].rot;
    g_game_transform_comp[i].scalex = APP_RES_UNIT_DATA[UNIT_ID].scalex;
    g_game_transform_comp[i].scaley = APP_RES_UNIT_DATA[UNIT_ID].scaley;
    g_game_transform_comp[i].transx = APP_RES_UNIT_DATA[UNIT_ID].transx;
    g_game_transform_comp[i].transy = APP_RES_UNIT_DATA[UNIT_ID].transy;
  }

  g_game_units_entities_flag |= APP_GAME_ENTITY_DIRTY_FLAG;

  return true;
}
