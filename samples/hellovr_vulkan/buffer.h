#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <vector>
#include <string>
#include "lvulkan.h"

enum Location {
	HOST,
	DEVICE,
	HOST_COHERENT
};

struct Buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  size_t n = 0;
  
  Buffer();
  Buffer(size_t size, VkBufferUsageFlags usage, Location loc = DEVICE);
  
  template <typename T>
  Buffer(std::vector<T> &init_data, VkBufferUsageFlags usage, Location loc = DEVICE);
  
  template <typename T>
  Buffer(T init_data[], int n_, VkBufferUsageFlags usage, Location loc = DEVICE);

  template <typename T>
  void init(std::vector<T> &init_data, VkBufferUsageFlags usage, Location loc = DEVICE);
  
  template <typename T>
  void init(T init_data[], int n_, VkBufferUsageFlags usage, Location loc = DEVICE);
  
  void init(size_t size, VkBufferUsageFlags usage, Location loc);

  template <typename T>
  void map(T **ptr);

  size_t size() { return n; }
};

struct ViewedBuffer {
    Buffer buffer;
    VkBufferView buffer_view;
};

struct Image {
    VkImage img = 0;
    VkDeviceMemory mem = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkAccessFlags access_flags = 0;
  VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  
  VkImageView view = 0;
    VkSampler sampler = 0;

    unsigned width = 0, height = 0;
    int mip_levels = 1;

Image(VkImage img_) : img(img_) {
  }
  
    Image();
  Image(int width, int height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT, int msaa_sample_count = 1);
  Image(std::string path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

  void init(int width_, int height_, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect_, int mip_levels_ = 1, int msaa_sample_count = 1);
  void init_from_img(std::string img_path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);

  void barrier(VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkImageLayout new_layout);

  //void to_colour_optimal();
  //void to_depth_optimal();
  //void to_read_optimal();
  //void to_transfer_dst();
  //void to_transfer_src();

};



#endif
