#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

enum Location {
	HOST,
	DEVICE,
	HOST_COHERENT
};

struct Buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;

  Buffer();

  Buffer(size_t size, VkBufferUsageFlags usage);

  template <typename T>
  void init(size_t size, VkBufferUsageFlags usage, Location loc, std::vector<T> &init_data);

  void init(size_t size, VkBufferUsageFlags usage, Location loc);

  template <typename T>
  void map(T **ptr);
  
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
    VkSampler sampler;

    unsigned width = 0, height = 0;
    int mip_levels = 1;

    Image();
    Image(int width, int height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
  Image(std::string path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

    void init(int width, int height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, int mip_levels_ = 1);
    void init_from_img(std::string img_path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);

    void to_colour_optimal();
    void to_depth_optimal();
    void to_read_optimal();
    void to_transfer_dst();

};

struct FrameRenderBuffer {
    Image img, depth_stencil;
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
    int width, height;

    void init(int width_, int height_);



	void start_render_pass();
	void end_render_pass();
};

#endif
