#pragma once

#include <cassert>
#include <cstdlib>
#include <vector>

#include <SDL3/SDL_vulkan.h>
#include <slang-com-ptr.h>
#include <tiny_obj_loader.h>
#include <vk_mem_alloc.h>
#include <volk.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <ktx.h>
#include <ktxvulkan.h>

#include "core/core.hpp"
#include "main/global.hpp"

#define APP_VKCHK(vkres)                                   \
  do {                                                     \
    if ((vkres) != VK_SUCCESS) {                           \
      LOGE("[app] APP_VKCHK failed: {}", (uint32_t)vkres); \
      exit(vkres);                                         \
    }                                                      \
  } while (0);

#define APP_VKCHKSWAPCHAIN(vkres)                                   \
  do {                                                              \
    if ((vkres) < VK_SUCCESS) {                                     \
      if ((vkres) == VK_ERROR_OUT_OF_DATE_KHR) {                    \
        m_swapchainneedupdate = true;                               \
        break;                                                      \
      }                                                             \
      LOGE("[app] APP_VKCHKSWAPCHAIN failed: {}", (uint32_t)vkres); \
      exit(vkres);                                                  \
    }                                                               \
  } while (0);

namespace app_vk {

// GOBACK
static constexpr uint32_t m_devidx            = 0;
static constexpr uint32_t m_maxframesinflight = 2;
static uint32_t m_imgidxg                     = 0;
static uint32_t m_frameidx                    = 0;
static VkInstance m_instance                  = VK_NULL_HANDLE;
static VkDevice m_device                      = VK_NULL_HANDLE;
static VkQueue m_queue                        = VK_NULL_HANDLE;
static VkSurfaceKHR m_surface                 = VK_NULL_HANDLE;
static bool m_swapchainneedupdate             = false;
static VkSwapchainKHR m_swapchain             = VK_NULL_HANDLE;
static VkCommandPool m_cmdpool                = VK_NULL_HANDLE;
static VkPipeline m_pipeline                  = VK_NULL_HANDLE;
static VkPipelineLayout m_pipelinelayout      = VK_NULL_HANDLE;
static VkImage m_depthimg                     = {0};
static VmaAllocator m_allocator               = VK_NULL_HANDLE;
static VmaAllocation m_depthimgallocation;
static VkImageView m_depthimgview;
static std::vector<VkImage> m_swapchainimgs;
static std::vector<VkImageView> m_swapchainimgviews;
static std::array<VkCommandBuffer, m_maxframesinflight> m_cmdbufs;
static std::array<VkFence, m_maxframesinflight> fences;
static std::array<VkSemaphore, m_maxframesinflight> m_presentsemaphores;
static std::vector<VkSemaphore> rendersemaphores;
static VmaAllocation m_vbufallocation = VK_NULL_HANDLE;
static VkBuffer m_vbuf                = VK_NULL_HANDLE;

static struct shader_data_t {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model[3];
  glm::vec4 lightPos{0.0f, -10.0f, 10.0f, 0.0f};
  uint32_t selected{1};
} m_shaderdata{};
struct shader_data_buffer_t {
  VmaAllocation allocation          = VK_NULL_HANDLE;
  VmaAllocationInfo allocation_info = {};
  VkBuffer buffer                   = VK_NULL_HANDLE;
  VkDeviceAddress device_addr       = {};
};
static std::array<shader_data_buffer_t, m_maxframesinflight> m_shaderdatabufs;
struct texture_t {
  VmaAllocation allocation = VK_NULL_HANDLE;
  VkImage image            = VK_NULL_HANDLE;
  VkImageView view         = VK_NULL_HANDLE;
  VkSampler sampler        = VK_NULL_HANDLE;
};
static std::array<texture_t, 3> m_textures{};
static VkDescriptorPool m_descpool{VK_NULL_HANDLE};
static VkDescriptorSetLayout m_descsetlayouttex{VK_NULL_HANDLE};
static VkDescriptorSet m_descsettex{VK_NULL_HANDLE};
static Slang::ComPtr<slang::IGlobalSession> m_slang_g_session;
static glm::vec3 m_campos{0.0f, 0.0f, -6.0f};
static glm::vec3 m_objrotation[3]{};
struct vertex_t {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

// tmp
static VkSwapchainCreateInfoKHR m_swapchain_ci = {};
static uint32_t m_imgcount                     = 0;
static std::vector<VkPhysicalDevice> m_devices = {};
static VkSurfaceCapabilitiesKHR m_surfacecaps  = {};
static uint32_t m_queuefam                     = 0;
static const VkFormat m_imgformat              = VK_FORMAT_B8G8R8A8_SRGB;
static VkFormat m_depthimgformat               = VK_FORMAT_UNDEFINED;
static VkImageCreateInfo m_depthimg_ci         = {};

static VkDeviceSize m_idxcount                 = 0;
static VkDeviceSize m_vbufsz                   = 0;
static VkDeviceSize m_ibufsz                   = 0;

static VkShaderModule m_shadermodule           = {};

static uint64_t m_lasttime                     = {0};

/*
 *
 *
 * init_m_device
 *
 *
 * */
static inline bool init_device(app_state_t*) {  //
  if (!SDL_Vulkan_LoadLibrary(NULL)) {
    LOGE("[app] SDL_Vulkan_LoadLibrary: {}", SDL_GetError());
    return false;
  }

  int vkres = 0;

  volkInitialize();

  // Instance
  VkApplicationInfo appinfo        = {.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                      .pApplicationName = APP_NAME,
                                      .apiVersion       = VK_API_VERSION_1_3};
  uint32_t instanceextcount        = 0;
  char const* const* instanceext   = SDL_Vulkan_GetInstanceExtensions(&instanceextcount);
  VkInstanceCreateInfo instance_ci = {
      .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo        = &appinfo,
      .enabledExtensionCount   = instanceextcount,
      .ppEnabledExtensionNames = instanceext,
  };
  APP_VKCHK(vkCreateInstance(&instance_ci, nullptr, &m_instance));
  volkLoadInstance(m_instance);
  // Device
  uint32_t devcount = 0;
  APP_VKCHK(vkEnumeratePhysicalDevices(m_instance, &devcount, nullptr));
  m_devices.resize(devcount);
  APP_VKCHK(vkEnumeratePhysicalDevices(m_instance, &devcount, m_devices.data()));
  uint32_t m_devidx = 0;
  // if (argc > 1) {
  //   m_devidx = std::stoi(argv[1]);
  //   assert(m_devidx < devcount);
  // }
  VkPhysicalDeviceProperties2 devprops = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(m_devices[m_devidx], &devprops);
  std::cout << "Selected m_device: " << devprops.properties.deviceName << "\n";
  // Find a m_queue family for graphics
  uint32_t queuefamcount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(m_devices[m_devidx], &queuefamcount, nullptr);
  std::vector<VkQueueFamilyProperties> queuefams(queuefamcount);
  vkGetPhysicalDeviceQueueFamilyProperties(m_devices[m_devidx], &queuefamcount, queuefams.data());
  m_queuefam = 0;
  for (size_t i = 0; i < queuefams.size(); i++) {
    if (queuefams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      m_queuefam = i;
      break;
    }
  }

  if (!SDL_Vulkan_GetPresentationSupport(m_instance, m_devices[m_devidx], m_queuefam)) {
    LOGE("[app] SDL_Vulkan_GetPresentationSupport: {}", SDL_GetError());
    return false;
  }
  // Logical m_device
  const float qfpriorities{1.0f};
  VkDeviceQueueCreateInfo queue_ci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                      .queueFamilyIndex = m_queuefam,
                                      .queueCount       = 1,
                                      .pQueuePriorities = &qfpriorities};
  VkPhysicalDeviceVulkan12Features enabledvk12features = {
      .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .descriptorIndexing = true,
      .shaderSampledImageArrayNonUniformIndexing = true,
      .descriptorBindingVariableDescriptorCount  = true,
      .runtimeDescriptorArray                    = true,
      .bufferDeviceAddress                       = true};
  VkPhysicalDeviceVulkan13Features enabledvk13features = {
      .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext            = &enabledvk12features,
      .synchronization2 = true,
      .dynamicRendering = true};
  const std::vector<const char*> devexts             = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const VkPhysicalDeviceFeatures enabledvk10features = {.samplerAnisotropy = VK_TRUE};
  VkDeviceCreateInfo dev_ci = {.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                               .pNext                   = &enabledvk13features,
                               .queueCreateInfoCount    = 1,
                               .pQueueCreateInfos       = &queue_ci,
                               .enabledExtensionCount   = static_cast<uint32_t>(devexts.size()),
                               .ppEnabledExtensionNames = devexts.data(),
                               .pEnabledFeatures        = &enabledvk10features};
  APP_VKCHK(vkCreateDevice(m_devices[m_devidx], &dev_ci, nullptr, &m_device));
  vkGetDeviceQueue(m_device, m_queuefam, 0, &m_queue);
  // VMA
  VmaVulkanFunctions vkfns              = {.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
                                           .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
                                           .vkCreateImage         = vkCreateImage};
  VmaAllocatorCreateInfo m_allocator_ci = {.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                           .physicalDevice   = m_devices[m_devidx],
                                           .device           = m_device,
                                           .pVulkanFunctions = &vkfns,
                                           .instance         = m_instance};
  APP_VKCHK(vmaCreateAllocator(&m_allocator_ci, &m_allocator));
  return true;
}

/*
 *
 *
 * init_m_surface
 *
 *
 * */
static inline bool init_surface(app_state_t* p_appstate) {
  if (!SDL_Vulkan_CreateSurface(p_appstate->p_wdow, app_vk::m_instance, NULL, &app_vk::m_surface)) {
    LOGE("[app] SDL_Vulkan_CreateSurface: {}", SDL_GetError());
    return false;
  }
  if (!SDL_GetWindowSize(p_appstate->p_wdow, &p_appstate->wdow_width, &p_appstate->wdow_height)) {
    LOGE("[app] SDL_GetWindowSize: {}", SDL_GetError());
    return false;
  }

  APP_VKCHK(
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_devices[m_devidx], m_surface, &m_surfacecaps));

  return true;
}

/*
 *
 *
 *
 *
 *
 *
 * */
static inline bool init_swapchain(app_state_t* appstate) {
  // Swap chain
  m_swapchain_ci =
      VkSwapchainCreateInfoKHR{.sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                               .surface         = m_surface,
                               .minImageCount   = m_surfacecaps.minImageCount,
                               .imageFormat     = m_imgformat,
                               .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
                               .imageExtent{.width  = m_surfacecaps.currentExtent.width,
                                            .height = m_surfacecaps.currentExtent.height},
                               .imageArrayLayers = 1,
                               .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                               .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                               .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                               .presentMode      = VK_PRESENT_MODE_FIFO_KHR};
  APP_VKCHK(vkCreateSwapchainKHR(m_device, &m_swapchain_ci, nullptr, &m_swapchain));
  m_imgcount = 0;
  APP_VKCHK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imgcount, nullptr));
  m_swapchainimgs.resize(m_imgcount);
  APP_VKCHK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imgcount, m_swapchainimgs.data()));
  m_swapchainimgviews.resize(m_imgcount);
  for (auto i = 0; i < m_imgcount; i++) {
    VkImageViewCreateInfo view_ci = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = m_swapchainimgs[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = m_imgformat,
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
    APP_VKCHK(vkCreateImageView(m_device, &view_ci, nullptr, &m_swapchainimgviews[i]));
  }

  return true;
}

/*
 *
 *
 *
 *
 *
 *
 *
 * */
static inline bool init_depthimg(app_state_t* p_appstate) {
  // Depth attachment
  std::vector<VkFormat> depthimgformatlist{VK_FORMAT_D32_SFLOAT_S8_UINT,
                                           VK_FORMAT_D24_UNORM_S8_UINT};
  m_depthimgformat = VK_FORMAT_UNDEFINED;
  for (VkFormat& format : depthimgformatlist) {
    VkFormatProperties2 formatprops = {.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
    vkGetPhysicalDeviceFormatProperties2(m_devices[m_devidx], format, &formatprops);
    if (formatprops.formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      m_depthimgformat = format;
      break;
    }
  }
  assert(m_depthimgformat != VK_FORMAT_UNDEFINED);
  // VkImageCreateInfo
  m_depthimg_ci = {
      .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format    = m_depthimgformat,
      .extent{.width  = static_cast<uint32_t>(p_appstate->wdow_width),
              .height = static_cast<uint32_t>(p_appstate->wdow_height),
              .depth  = 1},
      .mipLevels     = 1,
      .arrayLayers   = 1,
      .samples       = VK_SAMPLE_COUNT_1_BIT,
      .tiling        = VK_IMAGE_TILING_OPTIMAL,
      .usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VmaAllocationCreateInfo alloc_ci = {.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                                      .usage = VMA_MEMORY_USAGE_AUTO};
  APP_VKCHK(vmaCreateImage(m_allocator, &m_depthimg_ci, &alloc_ci, &m_depthimg,
                           &m_depthimgallocation, nullptr));
  VkImageViewCreateInfo depthview_ci = {
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image    = m_depthimg,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = m_depthimgformat,
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}};
  APP_VKCHK(vkCreateImageView(m_device, &depthview_ci, nullptr, &m_depthimgview));

  return true;
}

/*
 *
 *
 *
 *
 *
 *
 *
 *
 * */
static inline bool init_mesh_assets(app_state_t* p_appstate) {
  // Mesh data

  std::string objerr;
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &objerr,
                        APP_ASSETS_DIR_PATH "obj/suzanne.obj", APP_ASSETS_DIR_PATH "obj/")) {
    LOGE("[app] tinyobj::LoadObj: {}", objerr);
    return false;
  }
  // const VkDeviceSize
  m_idxcount = shapes[0].mesh.indices.size();

  std::vector<vertex_t> vertices{};
  std::vector<uint16_t> indices{};
  // Load vertex and index data
  for (auto& index : shapes[0].mesh.indices) {
    vertex_t v{.pos    = {attrib.vertices[index.vertex_index * 3],
                          -attrib.vertices[index.vertex_index * 3 + 1],
                          attrib.vertices[index.vertex_index * 3 + 2]},
               .normal = {attrib.normals[index.normal_index * 3],
                          -attrib.normals[index.normal_index * 3 + 1],
                          attrib.normals[index.normal_index * 3 + 2]},
               .uv     = {attrib.texcoords[index.texcoord_index * 2],
                          1.0 - attrib.texcoords[index.texcoord_index * 2 + 1]}};
    vertices.push_back(v);
    indices.push_back(indices.size());
  }
  // VkDeviceSize
  m_vbufsz = sizeof(vertex_t) * vertices.size();
  // VkDeviceSize
  m_ibufsz                  = sizeof(uint16_t) * indices.size();
  VkBufferCreateInfo buf_ci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = m_vbufsz + m_ibufsz,
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT};
  VmaAllocationCreateInfo vbuf_alloc_ci = {
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
               VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
               VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO};
  VmaAllocationInfo vbuf_allocinfo = {};
  APP_VKCHK(vmaCreateBuffer(m_allocator, &buf_ci, &vbuf_alloc_ci, &m_vbuf, &m_vbufallocation,
                            &vbuf_allocinfo));
  memcpy(vbuf_allocinfo.pMappedData, vertices.data(), m_vbufsz);
  memcpy(((char*)vbuf_allocinfo.pMappedData) + m_vbufsz, indices.data(), m_ibufsz);
  // Shader data buffers
  for (auto i = 0; i < m_maxframesinflight; ++i) {
    VkBufferCreateInfo ubuf_ci            = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                             .size  = sizeof(shader_data_t),
                                             .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT};
    VmaAllocationCreateInfo ubuf_alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO};
    APP_VKCHK(vmaCreateBuffer(m_allocator, &ubuf_ci, &ubuf_alloc_ci, &m_shaderdatabufs[i].buffer,
                              &m_shaderdatabufs[i].allocation,
                              &m_shaderdatabufs[i].allocation_info));
    VkBufferDeviceAddressInfo ubuf_bufaddrinfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = m_shaderdatabufs[i].buffer};
    m_shaderdatabufs[i].device_addr = vkGetBufferDeviceAddress(m_device, &ubuf_bufaddrinfo);
  }

  return true;
}

/*
 *
 *
 *
 *
 *
 *
 *
 *
 * */
static inline bool init_syncobj(app_state_t* p_appstate) {
  // Sync objects
  VkSemaphoreCreateInfo semaphore_ci = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fence_ci         = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                        .flags = VK_FENCE_CREATE_SIGNALED_BIT};
  for (auto i = 0; i < m_maxframesinflight; i++) {
    APP_VKCHK(vkCreateFence(m_device, &fence_ci, nullptr, &fences[i]));
    APP_VKCHK(vkCreateSemaphore(m_device, &semaphore_ci, nullptr, &m_presentsemaphores[i]));
  }
  rendersemaphores.resize(m_swapchainimgs.size());
  for (auto& semaphore : rendersemaphores) {
    APP_VKCHK(vkCreateSemaphore(m_device, &semaphore_ci, nullptr, &semaphore));
  }

  return true;
}

/*
 *
 *
 *
 *
 *
 *
 * */
static inline bool init_cmdpool(app_state_t* p_appstate) {
  // Command pool
  VkCommandPoolCreateInfo cmdpool_ci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                        .queueFamilyIndex = m_queuefam};
  APP_VKCHK(vkCreateCommandPool(m_device, &cmdpool_ci, nullptr, &m_cmdpool));
  VkCommandBufferAllocateInfo cmdbuf_alloc_ci = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = m_cmdpool,
      .commandBufferCount = m_maxframesinflight};
  APP_VKCHK(vkAllocateCommandBuffers(m_device, &cmdbuf_alloc_ci, m_cmdbufs.data()));

  return true;
}

/*
 *
 *
 *
 *
 *
 * */
static inline bool init_tex_assets(app_state_t* p_appstate) {
  // texture_t images
  std::vector<VkDescriptorImageInfo> texdescs{};
  for (auto i = 0; i < m_textures.size(); i++) {
    ktxTexture* ktxtex   = NULL;
    std::string filename = APP_ASSETS_DIR_PATH "obj/suzanne" + std::to_string(i) + ".ktx";
    ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                   &ktxtex);
    VkImageCreateInfo teximg_ci = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = ktxTexture_GetVkFormat(ktxtex),
        .extent        = {.width = ktxtex->baseWidth, .height = ktxtex->baseHeight, .depth = 1},
        .mipLevels     = ktxtex->numLevels,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    VmaAllocationCreateInfo teximg_alloc_ci{.usage = VMA_MEMORY_USAGE_AUTO};
    APP_VKCHK(vmaCreateImage(m_allocator, &teximg_ci, &teximg_alloc_ci, &m_textures[i].image,
                             &m_textures[i].allocation, nullptr));
    VkImageViewCreateInfo texview_ci = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = m_textures[i].image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = teximg_ci.format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .levelCount = ktxtex->numLevels,
                             .layerCount = 1}};
    APP_VKCHK(vkCreateImageView(m_device, &texview_ci, nullptr, &m_textures[i].view));
    // Upload
    VkBuffer imgsrcbuf{};
    VmaAllocation imgsrc_alloc{};
    VkBufferCreateInfo imgsrcbuf_ci         = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                               .size  = (uint32_t)ktxtex->dataSize,
                                               .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
    VmaAllocationCreateInfo imgsrc_alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO};
    VmaAllocationInfo imgsrc_allocinfo = {};
    APP_VKCHK(vmaCreateBuffer(m_allocator, &imgsrcbuf_ci, &imgsrc_alloc_ci, &imgsrcbuf,
                              &imgsrc_alloc, &imgsrc_allocinfo));
    memcpy(imgsrc_allocinfo.pMappedData, ktxtex->pData, ktxtex->dataSize);
    VkFenceCreateInfo fenceonetime_ci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fenceOneTime              = {};
    APP_VKCHK(vkCreateFence(m_device, &fenceonetime_ci, nullptr, &fenceOneTime));
    VkCommandBuffer cmdbuf_onetime               = {};
    VkCommandBufferAllocateInfo cmdbuf_allocinfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_cmdpool,
        .commandBufferCount = 1};
    APP_VKCHK(vkAllocateCommandBuffers(m_device, &cmdbuf_allocinfo, &cmdbuf_onetime));
    VkCommandBufferBeginInfo cmdbuf_begininfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    APP_VKCHK(vkBeginCommandBuffer(cmdbuf_onetime, &cmdbuf_begininfo));
    VkImageMemoryBarrier2 barrierteximg = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE,
        .dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image            = m_textures[i].image,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .levelCount = ktxtex->numLevels,
                             .layerCount = 1}};
    VkDependencyInfo barriertexinfo = {.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                       .imageMemoryBarrierCount = 1,
                                       .pImageMemoryBarriers    = &barrierteximg};
    vkCmdPipelineBarrier2(cmdbuf_onetime, &barriertexinfo);
    std::vector<VkBufferImageCopy> copyRegions{};
    for (auto j = 0; j < ktxtex->numLevels; j++) {
      ktx_size_t mipOffset{0};
      KTX_error_code ret = ktxTexture_GetImageOffset(ktxtex, j, 0, 0, &mipOffset);
      copyRegions.push_back({
          .bufferOffset = mipOffset,
          .imageSubresource{
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1},
          .imageExtent{
              .width = ktxtex->baseWidth >> j, .height = ktxtex->baseHeight >> j, .depth = 1},
      });
    }
    vkCmdCopyBufferToImage(cmdbuf_onetime, imgsrcbuf, m_textures[i].image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
    VkImageMemoryBarrier2 barriertexread = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        .image            = m_textures[i].image,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .levelCount = ktxtex->numLevels,
                             .layerCount = 1}};
    barriertexinfo.pImageMemoryBarriers = &barriertexread;
    vkCmdPipelineBarrier2(cmdbuf_onetime, &barriertexinfo);
    APP_VKCHK(vkEndCommandBuffer(cmdbuf_onetime));
    VkSubmitInfo onetime_submitinfo = {.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                       .commandBufferCount = 1,
                                       .pCommandBuffers    = &cmdbuf_onetime};
    APP_VKCHK(vkQueueSubmit(m_queue, 1, &onetime_submitinfo, fenceOneTime));
    APP_VKCHK(vkWaitForFences(m_device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
    vkDestroyFence(m_device, fenceOneTime, nullptr);
    vmaDestroyBuffer(m_allocator, imgsrcbuf, imgsrc_alloc);
    // Sampler
    VkSamplerCreateInfo sampler_ci = {
        .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter        = VK_FILTER_LINEAR,
        .minFilter        = VK_FILTER_LINEAR,
        .mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy    = 8.0f,
        .maxLod           = (float)ktxtex->numLevels,
    };
    APP_VKCHK(vkCreateSampler(m_device, &sampler_ci, nullptr, &m_textures[i].sampler));
    ktxTexture_Destroy(ktxtex);
    texdescs.push_back({.sampler     = m_textures[i].sampler,
                        .imageView   = m_textures[i].view,
                        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL});
  }
  // Descriptor (indexing)
  VkDescriptorBindingFlags descvarflag = {VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT};
  VkDescriptorSetLayoutBindingFlagsCreateInfo descbindingflags = {
      .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount  = 1,
      .pBindingFlags = &descvarflag};
  VkDescriptorSetLayoutBinding desclayoutbindingtex = {
      .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(m_textures.size()),
      .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT};
  VkDescriptorSetLayoutCreateInfo desclayouttex_ci = {
      .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext        = &descbindingflags,
      .bindingCount = 1,
      .pBindings    = &desclayoutbindingtex};
  APP_VKCHK(vkCreateDescriptorSetLayout(m_device, &desclayouttex_ci, nullptr, &m_descsetlayouttex));
  VkDescriptorPoolSize poolsz = {.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 .descriptorCount = static_cast<uint32_t>(m_textures.size())};
  VkDescriptorPoolCreateInfo m_descpool_ci = {
      .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets       = 1,
      .poolSizeCount = 1,
      .pPoolSizes    = &poolsz};
  APP_VKCHK(vkCreateDescriptorPool(m_device, &m_descpool_ci, nullptr, &m_descpool));
  uint32_t vardesccount{static_cast<uint32_t>(m_textures.size())};
  VkDescriptorSetVariableDescriptorCountAllocateInfo vardesccount_allocinfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
      .descriptorSetCount = 1,
      .pDescriptorCounts  = &vardesccount};
  VkDescriptorSetAllocateInfo texdescsetalloc = {
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext              = &vardesccount_allocinfo,
      .descriptorPool     = m_descpool,
      .descriptorSetCount = 1,
      .pSetLayouts        = &m_descsetlayouttex};
  APP_VKCHK(vkAllocateDescriptorSets(m_device, &texdescsetalloc, &m_descsettex));
  VkWriteDescriptorSet writedescset = {.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                       .dstSet          = m_descsettex,
                                       .dstBinding      = 0,
                                       .descriptorCount = static_cast<uint32_t>(texdescs.size()),
                                       .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                       .pImageInfo      = texdescs.data()};
  vkUpdateDescriptorSets(m_device, 1, &writedescset, 0, nullptr);

  return true;
}

/*
 *
 *
 *
 *
 *
 *
 * */
static inline bool init_shader(app_state_t* p_appstate) {
  // Initialize Slang shader compiler
  slang::createGlobalSession(m_slang_g_session.writeRef());
  auto slangTargets{
      std::to_array<slang::TargetDesc>({
          {
              .format  = SLANG_SPIRV,
              .profile = m_slang_g_session->findProfile("spirv_1_4"),
          },
      }),
  };
  auto slangOptions{std::to_array<slang::CompilerOptionEntry>(
      {{slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}}})};
  slang::SessionDesc slangsessiondesc = {
      .targets                  = slangTargets.data(),
      .targetCount              = SlangInt(slangTargets.size()),
      .defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
      .compilerOptionEntries    = slangOptions.data(),
      .compilerOptionEntryCount = uint32_t(slangOptions.size())};
  // Load shader
  Slang::ComPtr<slang::ISession> slangsession;
  m_slang_g_session->createSession(slangsessiondesc, slangsession.writeRef());
  Slang::ComPtr<slang::IModule> slangmodule{slangsession->loadModuleFromSource(
      "triangle", APP_ASSETS_DIR_PATH "shaders/shader.slang", nullptr, nullptr)};
  Slang::ComPtr<ISlangBlob> spirv;
  slangmodule->getTargetCode(0, spirv.writeRef());
  VkShaderModuleCreateInfo shadermodel_ci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                             .codeSize = spirv->getBufferSize(),
                                             .pCode    = (uint32_t*)spirv->getBufferPointer()};
  // VkShaderModule
  m_shadermodule = {};
  APP_VKCHK(vkCreateShaderModule(m_device, &shadermodel_ci, nullptr, &m_shadermodule));
  // Pipeline
  VkPushConstantRange pushconstantrange          = {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                                                    .size       = sizeof(VkDeviceAddress)};
  VkPipelineLayoutCreateInfo m_pipelinelayout_ci = {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount         = 1,
      .pSetLayouts            = &m_descsetlayouttex,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges    = &pushconstantrange};
  APP_VKCHK(vkCreatePipelineLayout(m_device, &m_pipelinelayout_ci, nullptr, &m_pipelinelayout));
  std::vector<VkPipelineShaderStageCreateInfo> shaderstages = {
      {.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage  = VK_SHADER_STAGE_VERTEX_BIT,
       .module = m_shadermodule,
       .pName  = "main"},
      {.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = m_shadermodule,
       .pName  = "main"}};
  VkVertexInputBindingDescription vertexbinding = {
      .binding = 0, .stride = sizeof(vertex_t), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  std::vector<VkVertexInputAttributeDescription> vertexattrs = {
      {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT},
      {.location = 1,
       .binding  = 0,
       .format   = VK_FORMAT_R32G32B32_SFLOAT,
       .offset   = offsetof(vertex_t, normal)},
      {.location = 2,
       .binding  = 0,
       .format   = VK_FORMAT_R32G32_SFLOAT,
       .offset   = offsetof(vertex_t, uv)},
  };
  VkPipelineVertexInputStateCreateInfo vertexinput_state = {
      .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount   = 1,
      .pVertexBindingDescriptions      = &vertexbinding,
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexattrs.size()),
      .pVertexAttributeDescriptions    = vertexattrs.data(),
  };
  VkPipelineInputAssemblyStateCreateInfo inputassembly_state = {
      .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
  std::vector<VkDynamicState> dynstates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state = {
      .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates    = dynstates.data()};
  VkPipelineViewportStateCreateInfo vp_state = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount  = 1};
  VkPipelineRasterizationStateCreateInfo rasterization_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .lineWidth = 1.0f};
  VkPipelineMultisampleStateCreateInfo multisample_state = {
      .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
  VkPipelineDepthStencilStateCreateInfo depthstencil_state = {
      .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable  = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL};
  VkPipelineColorBlendAttachmentState blendattachment  = {.colorWriteMask = 0xF};
  VkPipelineColorBlendStateCreateInfo colorblend_state = {
      .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments    = &blendattachment};
  VkPipelineRenderingCreateInfo rendering_ci = {
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount    = 1,
      .pColorAttachmentFormats = &m_imgformat,
      .depthAttachmentFormat   = m_depthimgformat};
  VkGraphicsPipelineCreateInfo m_pipeline_ci = {
      .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext               = &rendering_ci,
      .stageCount          = 2,
      .pStages             = shaderstages.data(),
      .pVertexInputState   = &vertexinput_state,
      .pInputAssemblyState = &inputassembly_state,
      .pViewportState      = &vp_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState   = &multisample_state,
      .pDepthStencilState  = &depthstencil_state,
      .pColorBlendState    = &colorblend_state,
      .pDynamicState       = &dynamic_state,
      .layout              = m_pipelinelayout};
  APP_VKCHK(
      vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &m_pipeline_ci, nullptr, &m_pipeline));

  return true;
}

static inline bool handle_event(app_state_t* p_appstate, SDL_Event* p_ev) {
  float elapsedtime = (SDL_GetTicks() - m_lasttime) / 1000.0f;
  m_lasttime        = SDL_GetTicks();
  if (p_ev->type == SDL_EVENT_MOUSE_MOTION) {
    if (p_ev->button.button == SDL_BUTTON_LEFT) {
      m_objrotation[m_shaderdata.selected].x -= (float)p_ev->motion.yrel * elapsedtime;
      m_objrotation[m_shaderdata.selected].y += (float)p_ev->motion.xrel * elapsedtime;
    }
  }

  if (p_ev->type == SDL_EVENT_MOUSE_WHEEL) {
    m_campos.z += (float)p_ev->wheel.y * elapsedtime * 10.0f;
  }
  if (p_ev->type == SDL_EVENT_KEY_DOWN) {
    if (p_ev->key.key == SDLK_PLUS || p_ev->key.key == SDLK_KP_PLUS) {
      m_shaderdata.selected = (m_shaderdata.selected < 2) ? m_shaderdata.selected + 1 : 0;
    }
    if (p_ev->key.key == SDLK_MINUS || p_ev->key.key == SDLK_KP_MINUS) {
      m_shaderdata.selected = (m_shaderdata.selected > 0) ? m_shaderdata.selected - 1 : 2;
    }
  }

  // Window resize
  if (p_ev->type == SDL_EVENT_WINDOW_RESIZED) {
    m_swapchainneedupdate = true;
  }

  if (m_swapchainneedupdate) {
    if (!SDL_GetWindowSize(p_appstate->p_wdow, &p_appstate->wdow_width, &p_appstate->wdow_height)) {
      LOGE("[app] SDL_GetWindowSize: {}", SDL_GetError());
      return false;
    }
    m_swapchainneedupdate = false;
    APP_VKCHK(vkDeviceWaitIdle(m_device));
    APP_VKCHK(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_devices[m_devidx], m_surface, &m_surfacecaps));
    m_swapchain_ci.oldSwapchain = m_swapchain;
    m_swapchain_ci.imageExtent  = {.width  = static_cast<uint32_t>(p_appstate->wdow_width),
                                   .height = static_cast<uint32_t>(p_appstate->wdow_height)};
    APP_VKCHK(vkCreateSwapchainKHR(m_device, &m_swapchain_ci, nullptr, &m_swapchain));
    for (auto i = 0; i < m_imgcount; i++) {
      vkDestroyImageView(m_device, m_swapchainimgviews[i], nullptr);
    }
    APP_VKCHK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imgcount, nullptr));
    m_swapchainimgs.resize(m_imgcount);
    APP_VKCHK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imgcount, m_swapchainimgs.data()));
    m_swapchainimgviews.resize(m_imgcount);
    for (auto i = 0; i < m_imgcount; i++) {
      VkImageViewCreateInfo view_ci = {
          .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image            = m_swapchainimgs[i],
          .viewType         = VK_IMAGE_VIEW_TYPE_2D,
          .format           = m_imgformat,
          .subresourceRange = {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
      APP_VKCHK(vkCreateImageView(m_device, &view_ci, nullptr, &m_swapchainimgviews[i]));
    }
    vkDestroySwapchainKHR(m_device, m_swapchain_ci.oldSwapchain, nullptr);
    vmaDestroyImage(m_allocator, m_depthimg, m_depthimgallocation);
    vkDestroyImageView(m_device, m_depthimgview, nullptr);
    m_depthimg_ci.extent             = {.width  = static_cast<uint32_t>(p_appstate->wdow_width),
                                        .height = static_cast<uint32_t>(p_appstate->wdow_height),
                                        .depth  = 1};
    VmaAllocationCreateInfo alloc_ci = {.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                                        .usage = VMA_MEMORY_USAGE_AUTO};
    APP_VKCHK(vmaCreateImage(m_allocator, &m_depthimg_ci, &alloc_ci, &m_depthimg,
                             &m_depthimgallocation, nullptr));
    VkImageViewCreateInfo view_ci = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = m_depthimg,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = m_depthimgformat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}};
    APP_VKCHK(vkCreateImageView(m_device, &view_ci, nullptr, &m_depthimgview));
  }

  return true;
}

/*
 *
 *
 *
 *
 *
 *
 *
 * */
static inline void begin_render(app_state_t* p_appstate) {
  // Sync
  APP_VKCHK(vkWaitForFences(m_device, 1, &fences[m_frameidx], true, UINT64_MAX));
  APP_VKCHK(vkResetFences(m_device, 1, &fences[m_frameidx]));
  APP_VKCHKSWAPCHAIN(vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                                           m_presentsemaphores[m_frameidx], VK_NULL_HANDLE,
                                           &m_imgidxg));
  // Update shader data
  m_shaderdata.projection =
      glm::perspective(glm::radians(45.0f),
                       (float)p_appstate->wdow_width / (float)p_appstate->wdow_height, 0.1f, 32.0f);
  m_shaderdata.view = glm::translate(glm::mat4(1.0f), m_campos);
  for (auto i = 0; i < 3; i++) {
    auto m_instancePos    = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
    m_shaderdata.model[i] = glm::translate(glm::mat4(1.0f), m_instancePos) *
                            glm::mat4_cast(glm::quat(m_objrotation[i]));
  }
  memcpy(m_shaderdatabufs[m_frameidx].allocation_info.pMappedData, &m_shaderdata,
         sizeof(shader_data_t));
  // Build command buffer
  auto cmdbuf = m_cmdbufs[m_frameidx];
  APP_VKCHK(vkResetCommandBuffer(cmdbuf, 0));
  VkCommandBufferBeginInfo cmdbuf_begininfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  APP_VKCHK(vkBeginCommandBuffer(cmdbuf, &cmdbuf_begininfo));
  std::array<VkImageMemoryBarrier2, 2> output_barriers = {
      VkImageMemoryBarrier2{
          .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          .srcAccessMask = 0,
          .dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstAccessMask =
              VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
          .image     = m_swapchainimgs[m_imgidxg],
          .subresourceRange{
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}},
      VkImageMemoryBarrier2{
          .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
          .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
          .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout     = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
          .image         = m_depthimg,
          .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                            .levelCount = 1,
                            .layerCount = 1}}};
  VkDependencyInfo barrier_depinfo = {.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                      .imageMemoryBarrierCount = 2,
                                      .pImageMemoryBarriers    = output_barriers.data()};
  vkCmdPipelineBarrier2(cmdbuf, &barrier_depinfo);
  VkRenderingAttachmentInfo color_atchinfo  = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                               .imageView   = m_swapchainimgviews[m_imgidxg],
                                               .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                               .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                               .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                                               .clearValue{.color{0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderingAttachmentInfo depth_attchinfo = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                               .imageView   = m_depthimgview,
                                               .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                               .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                               .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                               .clearValue  = {.depthStencil = {1.0f, 0}}};
  VkRenderingInfo renderinginfo             = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea{.extent{.width  = static_cast<uint32_t>(p_appstate->wdow_width),
                          .height = static_cast<uint32_t>(p_appstate->wdow_height)}},
      .layerCount           = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &color_atchinfo,
      .pDepthAttachment     = &depth_attchinfo};
  vkCmdBeginRendering(cmdbuf, &renderinginfo);
  VkViewport vp = {.width    = static_cast<float>(p_appstate->wdow_width),
                   .height   = static_cast<float>(p_appstate->wdow_height),
                   .minDepth = 0.0f,
                   .maxDepth = 1.0f};
  vkCmdSetViewport(cmdbuf, 0, 1, &vp);
  VkRect2D scissor = {.extent{.width  = static_cast<uint32_t>(p_appstate->wdow_width),
                              .height = static_cast<uint32_t>(p_appstate->wdow_height)}};
  vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
  vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
  vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelinelayout, 0, 1,
                          &m_descsettex, 0, nullptr);
  VkDeviceSize vOffset{0};
  vkCmdBindVertexBuffers(cmdbuf, 0, 1, &m_vbuf, &vOffset);
  vkCmdBindIndexBuffer(cmdbuf, m_vbuf, m_vbufsz, VK_INDEX_TYPE_UINT16);
  vkCmdPushConstants(cmdbuf, m_pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(VkDeviceAddress), &m_shaderdatabufs[m_frameidx].device_addr);
  vkCmdDrawIndexed(cmdbuf, m_idxcount, 3, 0, 0, 0);
  vkCmdEndRendering(cmdbuf);
  VkImageMemoryBarrier2 barrierPresent{
      .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = 0,
      .oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .image         = m_swapchainimgs[m_imgidxg],
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
  VkDependencyInfo barrierPresentDependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                                .imageMemoryBarrierCount = 1,
                                                .pImageMemoryBarriers    = &barrierPresent};
  vkCmdPipelineBarrier2(cmdbuf, &barrierPresentDependencyInfo);
  APP_VKCHK(vkEndCommandBuffer(cmdbuf));
  // Submit to graphics m_queue
  VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submitInfo{
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount   = 1,
      .pWaitSemaphores      = &m_presentsemaphores[m_frameidx],
      .pWaitDstStageMask    = &waitStages,
      .commandBufferCount   = 1,
      .pCommandBuffers      = &cmdbuf,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores    = &rendersemaphores[m_imgidxg],
  };
  APP_VKCHK(vkQueueSubmit(m_queue, 1, &submitInfo, fences[m_frameidx]));
  m_frameidx = (m_frameidx + 1) % m_maxframesinflight;
  VkPresentInfoKHR presentInfo{.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                               .waitSemaphoreCount = 1,
                               .pWaitSemaphores    = &rendersemaphores[m_imgidxg],
                               .swapchainCount     = 1,
                               .pSwapchains        = &m_swapchain,
                               .pImageIndices      = &m_imgidxg};
  APP_VKCHKSWAPCHAIN(vkQueuePresentKHR(m_queue, &presentInfo));
}

static inline void destroy(app_state_t* p_appstate) {
  // Tear down
  APP_VKCHK(vkDeviceWaitIdle(m_device));
  for (auto i = 0; i < m_maxframesinflight; i++) {
    vkDestroyFence(m_device, fences[i], nullptr);
    vkDestroySemaphore(m_device, m_presentsemaphores[i], nullptr);
    vmaDestroyBuffer(m_allocator, m_shaderdatabufs[i].buffer, m_shaderdatabufs[i].allocation);
  }
  for (auto i = 0; i < rendersemaphores.size(); i++) {
    vkDestroySemaphore(m_device, rendersemaphores[i], nullptr);
  }
  vmaDestroyImage(m_allocator, m_depthimg, m_depthimgallocation);
  vkDestroyImageView(m_device, m_depthimgview, nullptr);
  for (auto i = 0; i < m_swapchainimgviews.size(); i++) {
    vkDestroyImageView(m_device, m_swapchainimgviews[i], nullptr);
  }
  vmaDestroyBuffer(m_allocator, m_vbuf, m_vbufallocation);
  for (auto i = 0; i < m_textures.size(); i++) {
    vkDestroyImageView(m_device, m_textures[i].view, nullptr);
    vkDestroySampler(m_device, m_textures[i].sampler, nullptr);
    vmaDestroyImage(m_allocator, m_textures[i].image, m_textures[i].allocation);
  }
  vkDestroyDescriptorSetLayout(m_device, m_descsetlayouttex, nullptr);
  vkDestroyDescriptorPool(m_device, m_descpool, nullptr);
  vkDestroyPipelineLayout(m_device, m_pipelinelayout, nullptr);
  vkDestroyPipeline(m_device, m_pipeline, nullptr);
  vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  vkDestroyCommandPool(m_device, m_cmdpool, nullptr);
  vkDestroyShaderModule(m_device, m_shadermodule, nullptr);

  vmaDestroyAllocator(m_allocator);
  SDL_DestroyWindow(p_appstate->p_wdow);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();

  vkDestroyDevice(m_device, nullptr);
  vkDestroyInstance(m_instance, nullptr);
}
};  // namespace app_vk
