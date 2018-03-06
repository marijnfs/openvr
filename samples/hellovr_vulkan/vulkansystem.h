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
#include "flywheel.h"


inline Matrix4 glm_to_mat4(glm::mat4 mat) {
  auto m = &mat[0];
  return Matrix4(m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1], 
		m[0][2], m[1][2], m[2][2], m[3][2], 
          m[0][3], m[1][3], m[2][3], m[3][3]);
}

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


/*
struct RenderModel {
	Buffer mat_buffer;
	Buffer vertex_buf, index_buf;

	VkDescriptorSet desc_set;

	void init();
	};*/

struct Descriptor {
	int idx = 0;
	VkDescriptorSet desc;
  VkImageView *view_ptr = 0;
  VkSampler *sampler_ptr = 0;
  VkBuffer *buffer_ptr = 0;


  Descriptor();
	void init();
	void register_texture(VkImageView &view);
	void register_model_texture(VkBuffer &buf, VkImageView &view, VkSampler &sampler);

  void bind();
};


struct GraphicsObject {
  Descriptor desc_left, desc_right;
 
  int n_vertex = 0, n_index = 0;
  Buffer vertex_buf;
  Buffer index_buf;
  
  Matrix4 *mvp_left, *mvp_right;
  Buffer mvp_buffer_left, mvp_buffer_right;
  
  //Image texture;
  
  std::vector<float> v;
  
  GraphicsObject();
  virtual void render(Matrix4 &mvp, bool right);
    void init_buffers();
  void add_vertex(float x, float y, float z, float tx, float ty);  
  
};

struct GraphicsCanvas : public GraphicsObject {
  std::string texture; //flywheel is responsible for keeping image resources

  GraphicsCanvas();
  GraphicsCanvas(std::string texture_);

  void init();
  
  //void render(Matrix4 &mvp, bool right);
  
  void change_texture(std::string texture_) {
    texture = texture_;
    auto *img = ImageFlywheel::image(texture);
    desc_left.register_texture(img->view);
    desc_right.register_texture(img->view);
  }
  
};

struct GraphicsCube : public GraphicsObject {
  GraphicsCube();
  
  //virtual void render(Matrix4 &mvp, bool right);

};

struct Swapchain {
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  std::vector< VkImage > images;
  std::vector< VkImageView > views;
  std::vector< VkFramebuffer > framebuffers;
  std::vector< VkSemaphore > semaphores;

  uint32_t n_swap;
  uint32_t frame_idx = 0;
  uint32_t current_swapchain_image = 0;

  VkRenderPass renderpass, companion_renderpass;

  void init();
  void to_present(int i);

  void get_image();  
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
  VkDescriptorSetLayout desc_set_layout;
  std::vector<VkDescriptorSet> desc_sets;
  
  //Buffer scene_constant_buffer[2]; //for both eyes

	//VkImage scene_img;
	//VkDeviceMemory scene_img_mem;
	//VkImageView scene_img_view;

	// Buffer scene_staging;
	// VkBuffer scene_staging_buffer;
	// VkDeviceMemory scene_staging_buffer_memory;

	// VkSampler scene_sampler;

  //Shader stuff
  VkShaderModule shader_modules_vs[PSO_COUNT], shader_modules_ps[PSO_COUNT];
  VkPipeline pipelines[PSO_COUNT];
  VkPipelineLayout pipeline_layout;
  VkPipelineCache pipeline_cache;

  std::deque< FencedCommandBuffer > cmd_buffers;
  VkCommandBuffer cur_cmd_buffer;
  VkFence cur_fence;

  VkSampler sampler;

  VkDebugReportCallbackEXT debug_callback;

  VulkanSystem();
  void init(); //general init


  void submit(FencedCommandBuffer &fcb);

  void wait_queue();

  void init_instance();

  void init_device();

  void init_descriptor_sets();

  void init_swapchain();

  void init_shaders();

  void init_texture_maps();

  void add_desc_set();

   void swapchain_to_present(int i);

 //void init_vulkan();

  void start_cmd_buffer();

  void end_cmd_buffer();

  void submit_cmd_buffer();

  VkCommandBuffer cmd_buffer();
};

#include "scene.h"
struct DrawVisitor : public ObjectVisitor {
  GraphicsCube gcube;
  GraphicsCanvas gcanvas;
  Matrix4 mvp;
  bool right = false;
  
  void visit(Canvas &canvas) {
    auto mat = mvp * glm_to_mat4(canvas.to_mat4());
    
    gcanvas.render(mat, right);
  }
  
  void visit(Controller &controller) {
  }
  
  void visit(Point &point) {
  }
  
  void visit(HMD &hmd) {
  }
                              
};

int get_mem_type( uint32_t mem_bits, VkMemoryPropertyFlags mem_prop );

#endif
