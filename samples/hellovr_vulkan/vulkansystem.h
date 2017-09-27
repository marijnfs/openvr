#ifndef __VULKANSYSTEM_H__
#define __VULKANSYSTEM_H__

#if defined( _WIN32 )
	#define VK_USE_PLATFORM_WIN32_KHR
#else
	#define SDL_VIDEO_DRIVER_X11
	#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <vulkan/vulkan.h>
#include <vector>
#include <queue>
#include "buffer.h"
#include "shared/Matrices.h"

enum PSO //shader files
{
  PSO_SCENE = 0,
  PSO_AXES,
  PSO_RENDERMODEL,
  PSO_COMPANION,
  PSO_COUNT
};

struct FencedCommandBuffer {
  VkCommandBuffer cmd_buffer;
  VkFence fence;

  void begin();
  void end();
  bool finished();
  void reset();
  void init();

};


struct RenderModel {
	Buffer mat_buffer;
	Buffer vertex_buf, index_buf;

	VkDescriptorSet desc_set;

	void init();
};

struct Descriptor {
	int idx = 0;
	VkDescriptorSet desc;

	void init();
	void register_texture(VkImageView view);
	void register_model_texture(VkBuffer buf, VkImageView view, VkSampler sampler);
};


struct GraphicsObject {
	Descriptor desc;
	Buffer vertex_buf;
	Buffer index_buf;

	int n_vertex, n_index;
	Image texture;

	std::vector<float> v;


	void draw();
	void init_cube(Matrix4 pos);
	void add_vertex(float fl0, float fl1, float fl2, float fl3, float fl4);

};


struct Swapchain {
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  std::vector< VkImage > images;
  std::vector< VkImageView > views;
  std::vector< VkFramebuffer > framebuffers;
  std::vector< VkSemaphore > semaphores;

  uint32_t n_swap;

  VkRenderPass renderpass, companion_renderpass;

  void init();
  void to_present(int i);
};

struct VulkanSystem {
  VkInstance inst;

  VkDevice dev;
  VkPhysicalDevice phys_dev;
  VkPhysicalDeviceProperties prop;
  VkPhysicalDeviceMemoryProperties mem_prop;
  VkPhysicalDeviceFeatures features;

  VkQueue queue;

  int graphics_queue;
  uint32_t frame_idx;
  uint32_t current_frame;
  uint32_t msaa = 1;

  VkCommandPool cmd_pool;
  Swapchain swapchain;


  VkDescriptorPool desc_pool;
  std::vector<VkDescriptorSet> desc_sets;
  VkDescriptorSetLayout desc_set_layout;

  Buffer scene_constant_buffer[2]; //for both eyes
	VkImage scene_img;
	VkDeviceMemory scene_img_mem;
	VkImageView scene_img_view;
	Buffer scene_staging;
	VkBuffer scene_staging_buffer;
	VkDeviceMemory scene_staging_buffer_memory;
	VkSampler scene_sampler;

  //Shader stuff
  VkShaderModule shader_modules_vs[PSO_COUNT], shader_modules_ps[PSO_COUNT];
  VkPipeline pipelines[PSO_COUNT];
  VkPipelineLayout pipeline_layout;
  VkPipelineCache pipeline_cache;

  std::deque< FencedCommandBuffer > cmd_buffers;
  VkCommandBuffer cur_cmd_buffer;

  VkSampler sampler;

  VkDebugReportCallbackEXT debug_callback;

  VulkanSystem() ;

  void submit(FencedCommandBuffer fcb);

  void wait_queue();

  void init(); //general init

  void init_instance();

  void init_device();

  void init_descriptor_sets();

  void init_swapchain();

  void init_shaders();

  void init_texture_maps();

  void add_desc_set();

  void swapchain_to_present(int i);

  void init_vulkan();

  VkCommandBuffer cmd_buffer();
};

int get_mem_type( uint32_t mem_bits, VkMemoryPropertyFlags mem_prop );

#endif