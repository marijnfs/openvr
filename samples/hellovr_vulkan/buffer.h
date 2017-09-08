#ifndef __BUFFER_H__
#define __BUFFER_H__

struct Buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;

  Buffer();

  Buffer(size_T size, VkBufferUsageFlags usage);

  template <typename T>
  void init(size_T size, VkBufferUsageFlags usage, Location loc, std::vector<T> &init_data);

  void init(size_T size, VkBufferUsageFlags usage, Location loc);
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

    Image();

    Image(int width, int height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);

    void init(int width, int height, VkFormat format, VkImageUsageFlags usage, Vk);
};

struct FrameRenderBuffer {
    Image image, depth_stencil;
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
    int width, height;

    void init(int width_, int height_);

    void to_colour_optimal();
	void to_depth_optimal();
	void to_read_optimal();

	void start_render_pass();
	void end_render_pass();
};

#endif