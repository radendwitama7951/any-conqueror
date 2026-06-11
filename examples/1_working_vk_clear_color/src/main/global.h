#pragma once

#include <SDL3/SDL.h>

#include <volk.h>

#include <vk_mem_alloc.h>

#define APP_MAIN_WINDOW_SCALE 0.5
#define APP_MAIN_WINDOW_WIDTH (1920 * APP_MAIN_WINDOW_SCALE)
#define APP_MAIN_WINDOW_HEIGHT (1080 * APP_MAIN_WINDOW_SCALE)

SDL_Window* g_pmainwdow;

/*
 *
 * VK
 *
 * */

#define APP_VK_MAX_PDEVS 4
#define APP_VK_SC_IMGS_CAP 8
#define APP_VK_MAX_FIFO 2
#define APP_VK_DEVICE_COUNT 1

VkInstance g_vkinst;
VkPhysicalDevice g_vkdevs[APP_VK_MAX_PDEVS];
uint32_t g_vkdevs_cnt;
uint32_t g_vkdev_idx;
VkDevice g_vkdev;

VkQueue g_vkqueue;

VmaAllocator g_vkvma;

VkSurfaceKHR g_vksurface;
VkSurfaceCapabilitiesKHR g_vksurface_capas;

uint32_t g_vkimg_idx;
uint32_t g_vkframe_idx;

VkSwapchainKHR g_vksc;
bool g_vkupdate_sc;
VkImage g_vksc_imgs[APP_VK_SC_IMGS_CAP];
VkImageView g_vksc_img_views[APP_VK_SC_IMGS_CAP];

VkFence g_vkfncs[APP_VK_MAX_FIFO];
uint32_t g_vkfncs_sz;
VkSemaphore g_vkpresent_sems[APP_VK_MAX_FIFO];
uint32_t g_vkpresent_sems_sz;
VkSemaphore g_vkrender_sems[APP_VK_SC_IMGS_CAP];
uint32_t g_vkrender_sems_sz;

VkCommandPool g_vkcp;
VkCommandBuffer g_vkcb[APP_VK_MAX_FIFO];

VkFormat g_vkimg_fmt;
VkSwapchainCreateInfoKHR g_vksc_ci;
uint32_t g_vkimg_cnt;

uint32_t g_vkq_fam;
