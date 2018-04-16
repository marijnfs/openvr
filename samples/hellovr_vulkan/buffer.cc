#include "buffer.h"

#include "util.h"
#include "global.h"

#include "shared/lodepng.h"


using namespace std;

Buffer::Buffer() {}


Buffer::Buffer(size_t n_, VkBufferUsageFlags usage, Location loc) : n(n_) {
	init(n, usage, loc);
}


void Buffer::init(size_t n_, VkBufferUsageFlags usage, Location loc) {
  if (buffer)
    destroy();
  n = n_;
	auto &vk = Global::vk();
// Create the vertex buffer and fill with data
	VkBufferCreateInfo bci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bci.size = n;
	bci.usage = usage;
	check( vkCreateBuffer( vk.dev, &bci, nullptr, &buffer ), "vkCreateBuffer");

	VkMemoryRequirements memreq = {};
	vkGetBufferMemoryRequirements( vk.dev, buffer, &memreq );

	VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    
	alloc_info.memoryTypeIndex = get_mem_type( memreq.memoryTypeBits, 
		(loc == HOST) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 
        (loc == HOST_COHERENT) ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT) :
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
    //cout << "buf mem size: " << memreq.size << " " << alloc_info.memoryTypeIndex << endl;
	alloc_info.allocationSize = memreq.size;

	check( vkAllocateMemory( vk.dev, &alloc_info, nullptr, &memory ), "vkCreateBuffer" );
    
	check( vkBindBufferMemory( vk.dev, buffer, memory, 0 ), "vkBindBufferMemory" );
};

Buffer::~Buffer() {
  auto &vk = Global::vk();

  destroy();
}

void Buffer::destroy() {
  auto &vk = Global::vk();
  if (buffer) {
    vkDestroyBuffer(vk.dev, buffer, nullptr);
    vkFreeMemory(vk.dev, memory, nullptr);
    buffer = 0;
    memory = 0;
    n = 0;
  }
}

template <typename T>
void Buffer::map(T **ptr) {
  vkMapMemory( Global::vk().dev, memory, 0, VK_WHOLE_SIZE, 0, (void**)ptr );
}

template <typename T>
Buffer::Buffer(std::vector<T> &init_data, VkBufferUsageFlags usage, Location loc) {
  init(init_data, usage, loc);
}

template <typename T>
Buffer::Buffer(T init_data[], int n_, VkBufferUsageFlags usage, Location loc) {
  init(init_data, n_, usage, loc);
}

template <typename T>
void Buffer::update(std::vector<T> &init_data) {
  if (n != init_data.size() * sizeof(T))
    throw StringException("wrong update size");  
  
  auto &vk = Global::vk();
  void *data(0);
  //cout << n << endl;
  check( vkMapMemory( vk.dev, memory, 0, VK_WHOLE_SIZE, 0, &data ), "vkMapMemory");
  
  memcpy( data, &init_data[0], n );
  
  vkUnmapMemory(vk.dev , memory);
  
  VkMappedMemoryRange mem_range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
  mem_range.memory = memory;
  mem_range.size = VK_WHOLE_SIZE;
  vkFlushMappedMemoryRanges( vk.dev, 1, &mem_range );
}

template <typename T>
void Buffer::init(std::vector<T> &init_data, VkBufferUsageFlags usage, Location loc) {
  n = init_data.size() * sizeof(T);
  init(n, usage, loc);
  
  auto &vk = Global::vk();
  void *data(0);
  //cout << n << endl;
  check( vkMapMemory( vk.dev, memory, 0, VK_WHOLE_SIZE, 0, &data ), "vkMapMemory");
  
  memcpy( data, &init_data[0], n );
  
  vkUnmapMemory(vk.dev , memory);
  
  VkMappedMemoryRange mem_range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
  mem_range.memory = memory;
  mem_range.size = VK_WHOLE_SIZE;
  vkFlushMappedMemoryRanges( vk.dev, 1, &mem_range );
}

template <typename T>
void Buffer::init(T init_data[], int size, VkBufferUsageFlags usage, Location loc) { 
  n = size * sizeof(T); //size in bytes
  init(n, usage, loc);

  //cout << n << endl;
  auto &vk = Global::vk();
  void *data(0);
  check( vkMapMemory( vk.dev, memory, 0, VK_WHOLE_SIZE, 0, &data ), "vkMapMemory");
  
  memcpy( data, &init_data[0], size );
  
  vkUnmapMemory(vk.dev, memory);
  
  VkMappedMemoryRange mem_range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
  mem_range.memory = memory;
  mem_range.size = VK_WHOLE_SIZE;
  vkFlushMappedMemoryRanges( vk.dev, 1, &mem_range );
}


//nasty forward declarations
template void Buffer::map<int>(int **ptr);
template void Buffer::map<float>(float **ptr);
template void Buffer::map<double>(double **ptr);
template void Buffer::map<void>(void **ptr);


Image::Image() {}

Image::Image(VkImage img_, VkFormat format_, VkImageAspectFlags aspect_) :
  img(img_), format(format_), aspect(aspect_) {
  auto &vk = Global::vk();
	VkImageViewCreateInfo img_view_ci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	img_view_ci.flags = 0;
	img_view_ci.image = img;
	img_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	img_view_ci.format = format;
	img_view_ci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	img_view_ci.subresourceRange.aspectMask = aspect;
	img_view_ci.subresourceRange.baseMipLevel = 0;
	img_view_ci.subresourceRange.levelCount = 1;
	img_view_ci.subresourceRange.baseArrayLayer = 0;
	img_view_ci.subresourceRange.layerCount = mip_levels;
	check( vkCreateImageView( vk.dev, &img_view_ci, nullptr, &view ), "vkCreateImageView");  
}

Image::Image(int width, int height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, int msaa_sample_count) {
  init(width, height, format, usage, aspect, 1, msaa_sample_count, false);
}

Image::Image(string path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
  init_from_img(path, format, usage, aspect);  
}

void Image::init(int width_, int height_, VkFormat format_, VkImageUsageFlags usage, VkImageAspectFlags aspect_, int mip_levels_, int msaa_sample_count, bool make_sampler) {
  cout << "Image Init" << endl;
	width = width_;
	height = height_;
    aspect = aspect_;
    format = format_;
    
	auto &vk = Global::vk();
	mip_levels = mip_levels_;
    cout << "MIP LEVELS: " << mip_levels << endl;
    cout << "samples: " << msaa_sample_count << endl;
    
	VkImageCreateInfo imgci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imgci.imageType = VK_IMAGE_TYPE_2D;
	imgci.extent.width = width;
	imgci.extent.height = height;
	imgci.extent.depth = 1;
	imgci.mipLevels = mip_levels;
	imgci.arrayLayers = 1;
	imgci.format = format;
	imgci.tiling = VK_IMAGE_TILING_OPTIMAL;
	imgci.samples = ( VkSampleCountFlagBits ) msaa_sample_count;
	imgci.usage = usage;
	imgci.flags = 0;	

	check( vkCreateImage( vk.dev, &imgci, nullptr, &img ), "vkCreateImage");

	VkMemoryRequirements mem_req = {};
	vkGetImageMemoryRequirements( vk.dev, img, &mem_req );

	VkMemoryAllocateInfo mem_alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	mem_alloc_info.allocationSize = mem_req.size;
	mem_alloc_info.memoryTypeIndex = get_mem_type( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	check( vkAllocateMemory( vk.dev, &mem_alloc_info, nullptr, &mem), "vkAllocateMemory");
	check( vkBindImageMemory( vk.dev, img, mem, 0 ), "vkBindImageMemory");

	//create view
	VkImageViewCreateInfo img_view_ci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	img_view_ci.flags = 0;
	img_view_ci.image = img;
	img_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	img_view_ci.format = format;
	img_view_ci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	img_view_ci.subresourceRange.aspectMask = aspect;
	img_view_ci.subresourceRange.baseMipLevel = 0;
	img_view_ci.subresourceRange.levelCount = mip_levels;
	img_view_ci.subresourceRange.baseArrayLayer = 0;
	img_view_ci.subresourceRange.layerCount = 1;
	check( vkCreateImageView( vk.dev, &img_view_ci, nullptr, &view ), "vkCreateImageView");

	if (make_sampler) { //Do we always need sampler?
		VkSamplerCreateInfo sampler_ci = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		sampler_ci.magFilter = VK_FILTER_LINEAR;
		sampler_ci.minFilter = VK_FILTER_LINEAR;
		sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_ci.anisotropyEnable = VK_TRUE;
		sampler_ci.maxAnisotropy = 16.0f;
		sampler_ci.minLod = 0.0f;
		sampler_ci.maxLod = ( float ) mip_levels; 
		vkCreateSampler( vk.dev, &sampler_ci, nullptr, &sampler );
	}
}

void Image::init_from_img(string img_path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect_) {
  aspect = aspect_;
	auto &vk = Global::vk();
	std::vector< unsigned char > imageRGBA;
	if (lodepng::decode( imageRGBA, width, height, img_path.c_str() ) != 0) {
		throw StringException("failed to load texture lodepng");
	}
	
	// Copy the base level to a buffer, reserve space for mips (overreserve by a bit to avoid having to calc mipchain size ahead of time)
	VkDeviceSize buf_size = 0;
	uint8_t *ptr = new uint8_t[ width * height * 4 * 2 ];
	uint8_t *prev_buffer = ptr;
	uint8_t *cur_buffer = ptr;
	memcpy( cur_buffer, &imageRGBA[0], sizeof( uint8_t ) * width * height * 4 );
	cur_buffer += sizeof( uint8_t ) * width * height * 4;

	std::vector< VkBufferImageCopy > img_copies;
	VkBufferImageCopy buf_img_cpy = {};
	buf_img_cpy.bufferOffset = 0;
	buf_img_cpy.bufferRowLength = 0;
	buf_img_cpy.bufferImageHeight = 0;
	buf_img_cpy.imageSubresource.baseArrayLayer = 0;
	buf_img_cpy.imageSubresource.layerCount = 1;
	buf_img_cpy.imageSubresource.mipLevel = 0;
	buf_img_cpy.imageSubresource.aspectMask = aspect;
	buf_img_cpy.imageOffset.x = 0;
	buf_img_cpy.imageOffset.y = 0;
	buf_img_cpy.imageOffset.z = 0;
	buf_img_cpy.imageExtent.width = width;
	buf_img_cpy.imageExtent.height = height;
	buf_img_cpy.imageExtent.depth = 1;
	img_copies.push_back( buf_img_cpy );

	int mip_width = width;
	int mip_height = height;

	while( mip_width > 1 && mip_height > 1 )
	{
      //cout << mip_height << " " << mip_width << endl;
		gen_mipmap_rgba( prev_buffer, cur_buffer, mip_width, mip_height, &mip_width, &mip_height );
		buf_img_cpy.bufferOffset = cur_buffer - ptr;
		buf_img_cpy.imageSubresource.mipLevel++;
		buf_img_cpy.imageExtent.width = mip_width;
		buf_img_cpy.imageExtent.height = mip_height;
		img_copies.push_back( buf_img_cpy );
		prev_buffer = cur_buffer;
		cur_buffer += ( mip_width * mip_height * 4 * sizeof( uint8_t ) );
	}
	buf_size = cur_buffer - ptr;

	init(width, height, format, usage, aspect, img_copies.size(), 1, true);

	staging_buffer.init(ptr, buf_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST);

    //to_transfer_dst();
    barrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkCmdCopyBufferToImage( vk.cmd_buffer(), staging_buffer.buffer, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ( uint32_t ) img_copies.size(), &img_copies[ 0 ] );

    barrier(VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    delete [] ptr;
}

Image::~Image() {
  auto &vk = Global::vk();
  vkDestroyImageView(vk.dev, view, nullptr);
  vkDestroyImage(vk.dev, img, nullptr);
  vkFreeMemory(vk.dev, mem, nullptr);
  
}

void Image::barrier(VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkImageLayout new_layout) {
  auto &vk = Global::vk();
  VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.image = img;
    barrier.srcAccessMask = access_flags;
	barrier.dstAccessMask = access_flags = dst_access;
    barrier.oldLayout = layout;
	barrier.newLayout = layout = new_layout;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mip_levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
    barrier.srcQueueFamilyIndex = vk.graphics_queue;
    barrier.dstQueueFamilyIndex = vk.graphics_queue;
        

    vkCmdPipelineBarrier( vk.cmd_buffer(), src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier );

}

/*
void Image::to_colour_optimal() {
	auto &vk = Global::vk();
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    //barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.oldLayout = layout;
	layout = barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.image = img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcQueueFamilyIndex = vk.graphics_queue;
	barrier.dstQueueFamilyIndex = vk.graphics_queue;
	vkCmdPipelineBarrier( vk.cmd_buffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier );
}


void Image::to_depth_optimal() {
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.image = img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrier.oldLayout = layout;
	layout = barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	vkCmdPipelineBarrier( Global::vk().cmd_buffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier );
}

void Image::to_read_optimal() {
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.image = img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = layout;
	layout = barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier( Global::vk().cmd_buffer(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier );
}

void Image::to_transfer_dst() {
	// Transition the image to TRANSFER_DST to receive image
	auto &vk = Global::vk();

	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = layout;
	layout = barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.image = img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mip_levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcQueueFamilyIndex = vk.graphics_queue;
	barrier.dstQueueFamilyIndex = vk.graphics_queue;
	vkCmdPipelineBarrier( vk.cmd_buffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier );
}

void Image::to_transfer_src() {
	// Transition the image to TRANSFER_DST to receive image
	auto &vk = Global::vk();

	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout = layout;
	layout = barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.image = img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mip_levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcQueueFamilyIndex = vk.graphics_queue;
	barrier.dstQueueFamilyIndex = vk.graphics_queue;
	vkCmdPipelineBarrier( vk.cmd_buffer(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier );
}

*/

///Template implementations for Pos2Tex2
template
Buffer::Buffer<Pos2Tex2>(std::vector<Pos2Tex2> &init_data, VkBufferUsageFlags usage, Location loc);

template
Buffer::Buffer<Pos2Tex2>(Pos2Tex2 init_data[], int n_, VkBufferUsageFlags usage, Location loc);

template
void Buffer::init<Pos2Tex2>(std::vector<Pos2Tex2> &init_data, VkBufferUsageFlags usage, Location loc);

template
void Buffer::init<Pos2Tex2>(Pos2Tex2 init_data[], int size, VkBufferUsageFlags usage, Location loc);


///Template implementations for unsigned short
template
Buffer::Buffer<unsigned short>(std::vector<unsigned short> &init_data, VkBufferUsageFlags usage, Location loc);

template
Buffer::Buffer<unsigned short>(unsigned short init_data[], int n_, VkBufferUsageFlags usage, Location loc);

template
void Buffer::init<unsigned short>(std::vector<unsigned short> &init_data, VkBufferUsageFlags usage, Location loc);

template
void Buffer::init<unsigned short>(unsigned short init_data[], int size, VkBufferUsageFlags usage, Location loc);

template
void Buffer::update<unsigned short>(std::vector<unsigned short> &init_data);


///Template implementations for float
template
Buffer::Buffer<float>(std::vector<float> &init_data, VkBufferUsageFlags usage, Location loc);

template
Buffer::Buffer<float>(float init_data[], int n_, VkBufferUsageFlags usage, Location loc);

template
void Buffer::init<float>(std::vector<float> &init_data, VkBufferUsageFlags usage, Location loc);

template
void Buffer::init<float>(float init_data[], int size, VkBufferUsageFlags usage, Location loc);

template
void Buffer::update<float>(std::vector<float> &init_data);


///Template implementations for vr::RenderModel_Vertex_t
template
Buffer::Buffer<vr::RenderModel_Vertex_t>(std::vector<vr::RenderModel_Vertex_t> &init_data, VkBufferUsageFlags usage, Location loc);

template
Buffer::Buffer<vr::RenderModel_Vertex_t>(vr::RenderModel_Vertex_t init_data[], int n_, VkBufferUsageFlags usage, Location loc);

template
void Buffer::init<vr::RenderModel_Vertex_t>(std::vector<vr::RenderModel_Vertex_t> &init_data, VkBufferUsageFlags usage, Location loc);

template
void Buffer::init<vr::RenderModel_Vertex_t>(vr::RenderModel_Vertex_t init_data[], int size, VkBufferUsageFlags usage, Location loc);

template
void Buffer::update<vr::RenderModel_Vertex_t>(std::vector<vr::RenderModel_Vertex_t> &init_data);
