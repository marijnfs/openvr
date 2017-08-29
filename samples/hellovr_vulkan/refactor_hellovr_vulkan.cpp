//========= Copyright Valve Corporation ============//

#if defined( _WIN32 )
	#define VK_USE_PLATFORM_WIN32_KHR
#else
	#define SDL_VIDEO_DRIVER_X11
	#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <inttypes.h>
#include <openvr.h>
#include <deque>
#include <iostream>
#include <fstream>

#include "shared/lodepng.h"
#include "shared/Matrices.h"
#include "shared/pathtools.h"

#if defined(POSIX)
#include "unistd.h"
#endif

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

void ThreadSleep( unsigned long nMilliseconds )
{
#if defined(_WIN32)
	::Sleep( nMilliseconds );
#elif defined(POSIX)
	usleep( nMilliseconds * 1000 );
#endif
}

struct FencedCommandBuffer
{
  VkCommandBuffer cmd_buffer;
  VkFence fence;
};
  

struct VulkanSystem {
  VkInstance inst;

  VkDevice dev;
  VkPhysicalDevice phys_dev;
  VkPhysicalDeviceProperties prop;
  VkPhysicalDeviceMemoryProperties mem_prop;
  VkPhysicalDeviceFeatures features;

  VkQueue queue;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  
  uint32_t n_swap;
  uint32_t frame_idx;
  uint32_t current_frame;
  std::vector< VkImage > swapchain_img;
  std::vector< VkImageView > swapchain_view;
  std::vector< VkFramebuffer > swapchain_framebuffers;
  std::vector< VkSemaphore > swapchain_semaphores;
  VkRenderPass m_pSwapchainRenderPass;

  VkCommandPool cmd_pool;
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_sets[ NUM_DESCRIPTOR_SETS ];

  std::deque< FencedCommandBuffer > cmd_buffers;;
  
  
  VkSampler sampler;

  VkDebugReportCallbackEXT debug_callback;


};

struct VertexBuffer {
};

struct IndexBuffer {
};

struct VRSystem {
  vr::IVRSystem *hmd;
  vr::IVRRenderModels *render_models;
  //std::string driver_str;
  //std::string display_str;
  vr::TrackedDevicePose_t tracked_pose[ vr::k_unMaxTrackedDeviceCount ];
  Matrix4 device_pose[ vr::k_unMaxTrackedDeviceCount ];  
};

struct WindowSystem {
  SDL_Window *window;
  uint32_t width, height;
};

struct Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;

};

struct ViewedBuffer {
	Buffer buffer;
	VkBufferView buffer_view;
};

struct Image {
  VkImage img;
  VkDeviceMemory mem;
  VkImageLayout layout;
  VkImageView view;

  Image() {
    auto vk = Global::vk();
    
  }
};

struct FrameBuffer {
  Image image, depth_stencil;
  VkRenderPass render_pass;
  VkFramebuffer framebuffer;
};

struct Global {
	Global() {
		vk_ptr = new VulkanSystem();
		vr_ptr = new VRSystem();
		ws_ptr = new WindowSystem();
	}

	static Global &inst() {
		static Global *g = new Global();
		return g;
	}

	static VulkanSystem &vk() {
		return *(inst().vk_ptr);
	}

	static VRSystem &vr() {
		return *(inst().vr_ptr);
	}

	static WindowSystem &ws() {
		return *(inst().ws_ptr);
	}

	VulkanSystem *vk_ptr;
	VRSystem *vr_ptr;
	WindowSystem *ws_ptr;
};

int main() {
	Global::inst();

	
}
