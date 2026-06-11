#pragma once

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk.h>

#include <vk_mem_alloc.h>

#include "global.h"
#include "utils.h"

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
  g_vkdevs_cnt = APP_VK_MAX_PDEVS;
  app_vkchk(vkEnumeratePhysicalDevices(g_vkinst, &g_vkdevs_cnt, NULL),
            "vkEnumeratePhysicalDevices");
  log_info("Vk Physical Device Count: %d", g_vkdevs_cnt);
  app_chk(g_vkdevs_cnt < APP_VK_MAX_PDEVS, "g_vkdevs_cnt < APP_VK_MAX_PDEVS");
  app_vkchk(vkEnumeratePhysicalDevices(g_vkinst, &g_vkdevs_cnt, g_vkdevs),
            "vkEnumeratePhysicalDevices");
  g_vkdev_idx                           = 0;

  VkPhysicalDeviceProperties2 dev_props = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(g_vkdevs[g_vkdev_idx], &dev_props);

  // INIT QUEUE
  uint32_t g_vkq_fam_cnt = {0};
  vkGetPhysicalDeviceQueueFamilyProperties(g_vkdevs[g_vkdev_idx], &g_vkq_fam_cnt, NULL);
  VkQueueFamilyProperties g_vkq_fams[g_vkq_fam_cnt];
  vkGetPhysicalDeviceQueueFamilyProperties(g_vkdevs[g_vkdev_idx], &g_vkq_fam_cnt, g_vkq_fams);

  // uint32_t
  g_vkq_fam = 0;
  for (size_t i = 0; i < g_vkq_fam_cnt; ++i) {
    if (g_vkq_fams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      g_vkq_fam = i;
      break;
    }
  }

  app_chk(SDL_Vulkan_GetPresentationSupport(g_vkinst, g_vkdevs[g_vkdev_idx], g_vkq_fam),
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
  VkPhysicalDeviceFeatures vk10_f = {.samplerAnisotropy = VK_TRUE};
  const char* dev_exts[]          = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo dev_ci       = {
      .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext                   = &vk13_f,
      .queueCreateInfoCount    = 1,
      .pQueueCreateInfos       = &q_ci,
      .enabledExtensionCount   = (uint32_t)APP_GET_ARRAY_SIZE(dev_exts),
      .ppEnabledExtensionNames = dev_exts,
      .pEnabledFeatures        = &vk10_f,
  };
  app_vkchk(vkCreateDevice(g_vkdevs[g_vkdev_idx], &dev_ci, NULL, &g_vkdev), "vkCreateDevice");
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
      .physicalDevice   = g_vkdevs[g_vkdev_idx],
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
  app_vkchk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_vkdevs[g_vkdev_idx], g_vksurface,
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
  g_vkimg_fmt = VK_FORMAT_B8G8R8A8_SRGB;
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
 * app_vkninit_mesh_assets
 *
 * */
bool app_vkinit_mesh_assets() {
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
 * app_vkinit_tex_assets
 *
 * */
bool app_vkinit_tex_assets() {
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

bool app_vkinit_descriptor_pool() {
  static_assert(APP_UTIL_ARRAY_LEN(g_vkmain_game_tiles) == APP_VK_MAX_FIFO,
                "Size of game tiles data is equivalent to N-BUFFER (MAX_FIFO)");

  VkDescriptorPoolSize dsetpoolszs[] = {
      (VkDescriptorPoolSize){
          .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = APP_UTIL_ARRAY_LEN(g_vkmain_game_tiles),
      },
      (VkDescriptorPoolSize){
          .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 128 * APP_VK_MAX_FIFO,  // Max textures indexed
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

  return true;
}

/*
 *
 * app_vkinit_pipeline
 *
 * */
bool app_vkinit_pipeline() {

  // DESCRIPTOR
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
      0,
      (VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
       VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT),
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

  VkPipelineLayoutCreateInfo pllayout_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      // .flags =
      //     VK_PIPELINE_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,  // Required for bindless arrays
      .setLayoutCount         = 1,
      .pSetLayouts            = &g_vkmaindsetlayout,
      .pushConstantRangeCount = 0,
  };

  app_vkchk(vkCreatePipelineLayout(g_vkdev, &pllayout_ci, NULL, &g_vkmainpllayout),
            "vkCreatePipelineLayout");

  // DESCRIPTOR DATA
  for (size_t i = 0; i < APP_VK_MAX_FIFO; ++i) {
    // PREPAPARE BUFFER
    VkBufferCreateInfo buf_ci = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = APP_VK_MAIN_GAME_TILES_BUFFER_SZ,
        .usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo vma_aci = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo vma_ai = {0};
    app_vkchk(vmaCreateBuffer(g_vkvma_allocator, &buf_ci, &vma_aci, &g_vkmain_game_tiles[i].ssbobuf,
                              &g_vkmain_game_tiles[i].ssboalloc, &vma_ai),
              "vmaCreateBuffer");

    g_vkmain_game_tiles[i].pmapped_data = vma_ai.pMappedData;

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

    // g_vkmain_game_tiles is per frame data (g_vk_frames)
    app_vkchk(vkAllocateDescriptorSets(g_vkdev, &dset_ai, &g_vkmain_game_tiles[i].dset),
              "vkAllocateDescriptorSets");

    // WRITE DATA
    VkDescriptorBufferInfo dset_bufi = {
        .buffer = g_vkmain_game_tiles[i].ssbobuf,
        .offset = 0,
        .range  = APP_VK_MAIN_GAME_TILES_BUFFER_SZ,
    };

    VkWriteDescriptorSet wdset = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = g_vkmain_game_tiles[i].dset,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo     = &dset_bufi,
    };

    vkUpdateDescriptorSets(g_vkdev, 1, &wdset, 0, NULL);
  }

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
      .cullMode                = VK_CULL_MODE_BACK_BIT,
      .frontFace               = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable         = VK_FALSE,
  };

  VkPipelineMultisampleStateCreateInfo ms_state = {
      .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable  = VK_FALSE,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineColorBlendAttachmentState colb_atchstate = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable    = VK_FALSE,
  };

  VkPipelineColorBlendStateCreateInfo colb_state = {
      .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable   = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments    = &colb_atchstate,
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
      .renderPass          = VK_NULL_HANDLE,
      .layout              = g_vkmainpllayout,
  };

  app_vkchk(vkCreateGraphicsPipelines(g_vkdev, VK_NULL_HANDLE, 1, &pl_ci, NULL, &g_vkmainpl),
            "vkCreateGraphicsPipelines");

  return true;
}

/*
 *
 * app_vkhandle_event
 *
 * */
bool app_vkhandle_event(SDL_Event* p_ev) {
  return true;
}

/*
 *
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
      .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask    = 0,
      .dstStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .image            = g_vksc_imgs[g_vkimg_idx],
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };

  VkDependencyInfo barrier_di = {
      .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers    = &barrier,
  };
  vkCmdPipelineBarrier2(cb, &barrier_di);

  // log_debug("[app] IM HERE");
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkmainpl);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vkmainpllayout, 0, 1,
                          &g_vkmain_game_tiles[g_vkframe_idx].dset, 0, NULL);

  // BEGIN RENDER
  VkRenderingAttachmentInfo col_atchi = {
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView   = g_vksc_img_views[g_vkimg_idx],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue  = {
          .color = {APP_UTIL_COLOR_SRGBA(0.529f, 0.808f, 0.922f, 1.0f)},
      }};

  VkRenderingInfo ri = {
      .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea           = {.extent = {.width  = (uint32_t)APP_MAIN_WINDOW_WIDTH,
                                          .height = (uint32_t)APP_MAIN_WINDOW_HEIGHT}},
      .layerCount           = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &col_atchi,
  };
  vkCmdBeginRendering(cb, &ri);

  // vkCmdDraw(cb, 3, 1, 0, 0);
  vkCmdDraw(cb, 6, 2, 0, 0);

  // End Rendering
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

bool app_vkdestroy() {

  app_vkchk(vkDeviceWaitIdle(g_vkdev), "vkDeviceWaitIdle");

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
      vmaDestroyBuffer(g_vkvma_allocator, g_vkmain_game_tiles[i].ssbobuf,
                       g_vkmain_game_tiles[i].ssboalloc);
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
 * APP_GAME
 * 
 *
 * */
bool app_gameinit() {
  // INIT TILES
  size_t offset                                  = sizeof(g_vkmain_game_tiles);
  app_game_tile_data_t* game_tiles_mapped_data[] = {
      (app_game_tile_data_t*)g_vkmain_game_tiles[0].pmapped_data,
      (app_game_tile_data_t*)g_vkmain_game_tiles[1].pmapped_data,
  };

  if (game_tiles_mapped_data[0] == NULL) {
    log_error("[app] g_vkmain_game_tiles[%d].pmapped_data", 0);
    return false;
  }

  if (game_tiles_mapped_data[1] == NULL) {
    log_error("[app] g_vkmain_game_tiles[%d].pmapped_data", 1);
    return false;
  }

  // ADD INSTANCE
  app_game_tile_data_t tiles[] = {
      (app_game_tile_data_t){
          .coordx = 0.f,
          .coordy = 0.f,

          .colorr = 1.f,
          .colorg = 1.f,
          .colorb = 0.f,
          .colora = 1.f,

          .scalex = 1.f,
          .scaley = 1.5f,

          .rot    = 0.f,

          .transx = 0.f,
          .transy = 0.f,

          .texid  = 0.f,
      },
      (app_game_tile_data_t){
          .coordx = 0.f,
          .coordy = 0.f,

          .colorr = 1.f,
          .colorg = .0f,
          .colorb = 1.f,
          .colora = 1.f,

          .scalex = 0.25f,
          .scaley = 0.5f,

          .rot    = 0.f,

          .transx = 0.f,
          .transy = 0.f,

          .texid  = 0.f,
      },
  };

  // DOUBLE BUFFER
  memcpy(game_tiles_mapped_data[0], tiles, sizeof(tiles));
  memcpy(game_tiles_mapped_data[1], tiles, sizeof(tiles));

  return true;
}
