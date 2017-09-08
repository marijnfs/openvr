#ifndef __VULKANSYSTEM_H__
#define __VULKANSYSTEM_H__

#include <vulkan/vulkan.h>

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
};


struct RenderModel {
	Buffer mat_buffer;
	Buffer vertex_buf, index_buf;

	VkDescriptorSet desc_set;

	void init();
};


struct GraphicsObject {
	vector<float> v;


	void draw();
	void init_cube(Matrix4 pos);
	void add_vertex(t fl0, float fl1, float fl2, float fl3, float fl4);

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

  int graphics_queue;
  uint32_t n_swap;
  uint32_t frame_idx;
  uint32_t current_frame;

  std::vector< VkImage > swapchain_img;
  std::vector< VkImageView > swapchain_view;
  std::vector< VkFramebuffer > swapchain_framebuffers;
  std::vector< VkSemaphore > swapchain_semaphores;

  VkRenderPass swapchain_renderpass;

  VkCommandPool cmd_pool;
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_sets[ NUM_DESCRIPTOR_SETS ];
  VkDescriptorSetLayout desc_layout;

  Buffer scene_constant_buffer[2]; //for both eyes
	VkImage scene_img;
	VkDeviceMemory scene_img_mem;
	VkImageView scene_img_view;
	Buffer scene_staging
	VkBuffer scene_staging_buffer;
	VkDeviceMemory scene_staging_buffer_memory;
	VkSampler scene_sampler;

  //Shader stuff
  VkShaderModule shader_modules_vs[PSO_COUNT], shader_modules_ps[PSO_COUNT];
  VkPipelines pipelines[PSO_COUNT];
  VkPipelineLayout pipeline_layout;
  VkPipelineCache pipeline_cache;

  std::deque< FencedCommandBuffer > cmd_buffers;
	
	
  VkSampler sampler;

  VkDebugReportCallbackEXT debug_callback;

  VulkanSystem() ;

  void submit(FencedCommandBuffer fcb);

  void wait_queue();

  FencedCommandBuffer get_cmd_buffer();

  void init_instance();

  void init_device();

  void init_descriptor_sets();

  void init_swapchain();

  void init_shaders();

  void init_texture_maps();

  void swapchain_to_present(int i);

  void init_vulkan();

  VkCommandBuffer cmd_buffer();
};

int get_mem_type( uint32_t mem_bits, VkMemoryPropertyFlags mem_prop );

#endif