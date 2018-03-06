#include "buffer.h"

#include "util.h"
#include "global.h"

#include "shared/lodepng.h"


using namespace std;

Buffer::Buffer() {}

Buffer::Buffer(size_t size, VkBufferUsageFlags usage) {
	init(size, usage, DEVICE);
}

template <typename T>
void Buffer::init(size_t size, VkBufferUsageFlags usage, Location loc, std::vector<T> &init_data) {
	init(size, usage, loc);

	auto vk = Global::vk();
	void *data(0);
	check( vkMapMemory( vk.dev, memory, 0, VK_WHOLE_SIZE, 0, &data ), "vkMapMemory");

	memcpy( data, &init_data[0], sizeof(T) * init_data.size() );

	vkUnmapMemory(vk.dev , memory);

	VkMappedMemoryRange mem_range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
	mem_range.memory = memory;
	mem_range.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges( vk.dev, 1, &mem_range );
}

void Buffer::init(size_t size, VkBufferUsageFlags usage, Location loc) {
	auto vk = Global::vk();
// Create the vertex buffer and fill with data
	VkBufferCreateInfo bci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bci.size = size;
	bci.usage = usage;
	check( vkCreateBuffer( vk.dev, &bci, nullptr, &buffer ), "vkCreateBuffer");

	VkMemoryRequirements memreq = {};
	vkGetBufferMemoryRequirements( vk.dev, buffer, &memreq );

	VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	alloc_info.memoryTypeIndex = get_mem_type( memreq.memoryTypeBits, 
		(loc == HOST) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 
		(loc == HOST_COHERENT) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
	alloc_info.allocationSize = memreq.size;

	check( vkAllocateMemory( vk.dev, &alloc_info, nullptr, &memory ), "vkCreateBuffer" );

	check( vkBindBufferMemory( vk.dev, buffer, memory, 0 ), "vkBindBufferMemory" );
};

template <typename T>
void Buffer::map(T **ptr) {
  vkMapMemory( Global::vk().dev, memory, 0, VK_WHOLE_SIZE, 0, (void**)ptr );
}

//nasty forward declarations
template void Buffer::map<int>(int **ptr);
template void Buffer::map<float>(float **ptr);
template void Buffer::map<double>(double **ptr);
template void Buffer::map<void>(void **ptr);


Image::Image() {}

Image::Image(int width_, int height_, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect) : width(width_), height(height_) {
	init(width, height, format, usage, aspect);
}

Image::Image(string path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
  init_from_img(path, format, usage, aspect);  
}

void Image::init(int width_, int height_, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, int mip_levels_) {
	width = width_;
	height = height_;

	auto vk = Global::vk();
	mip_levels = mip_levels_;

	int msaa_sample_count(1);
	VkImageCreateInfo imgci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imgci.imageType = VK_IMAGE_TYPE_2D;
	imgci.extent.width = width;
	imgci.extent.height = height;
	imgci.extent.depth = 1;
	imgci.mipLevels = mip_levels;
	imgci.arrayLayers = 1;
	imgci.format = VK_FORMAT_R8G8B8A8_SRGB;
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
	img_view_ci.format = imgci.format;
	img_view_ci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	img_view_ci.subresourceRange.aspectMask = aspect;
	img_view_ci.subresourceRange.baseMipLevel = 0;
	img_view_ci.subresourceRange.levelCount = 1;
	img_view_ci.subresourceRange.baseArrayLayer = 0;
	img_view_ci.subresourceRange.layerCount = 1;
	check( vkCreateImageView( vk.dev, &img_view_ci, nullptr, &view ), "vkCreateImageView");

	if (true) { //Do we always need sampler?
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

void Image::init_from_img(string img_path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
	auto vk = Global::vk();
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
	buf_img_cpy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

	init(width, height, format, usage, aspect, img_copies.size());

	Buffer staging_buffer(buf_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	//TODO: copy data in staging buffer

	//TODO: create getpixel buffer?
	
	to_transfer_dst();
	vkCmdCopyBufferToImage( vk.cur_cmd_buffer, staging_buffer.buffer, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ( uint32_t ) img_copies.size(), &img_copies[ 0 ] );
	to_read_optimal();

	delete [] ptr;
}

void Image::to_colour_optimal() {
	auto vk = Global::vk();
	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
	barier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barier.oldLayout = layout;
	barier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barier.image = img;
	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barier.subresourceRange.baseMipLevel = 0;
	barier.subresourceRange.levelCount = 1;
	barier.subresourceRange.baseArrayLayer = 0;
	barier.subresourceRange.layerCount = 1;
	barier.srcQueueFamilyIndex = vk.graphics_queue;
	barier.dstQueueFamilyIndex = vk.graphics_queue;
	vkCmdPipelineBarrier( vk.cur_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
	layout = barier.newLayout;
}


void Image::to_depth_optimal() {
	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barier.image = img;
	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barier.subresourceRange.baseMipLevel = 0;
	barier.subresourceRange.levelCount = 1;
	barier.subresourceRange.baseArrayLayer = 0;
	barier.subresourceRange.layerCount = 1;
	barier.srcAccessMask = 0;
	barier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barier.oldLayout = layout;
	barier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	vkCmdPipelineBarrier( Global::vk().cur_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
	layout = barier.newLayout;
}

void Image::to_read_optimal() {
	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barier.image = img;
	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barier.subresourceRange.baseMipLevel = 0;
	barier.subresourceRange.levelCount = 1;
	barier.subresourceRange.baseArrayLayer = 0;
	barier.subresourceRange.layerCount = 1;
	barier.srcAccessMask = 0;
	barier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barier.oldLayout = layout;
	barier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier( Global::vk().cur_cmd_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
	layout = barier.newLayout;
}

void Image::to_transfer_dst() {
	// Transition the image to TRANSFER_DST to receive image
	auto vk = Global::vk();

	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barier.srcAccessMask = 0;
	barier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barier.image = img;
	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barier.subresourceRange.baseMipLevel = 0;
	barier.subresourceRange.levelCount = mip_levels;
	barier.subresourceRange.baseArrayLayer = 0;
	barier.subresourceRange.layerCount = 1;
	barier.srcQueueFamilyIndex = vk.graphics_queue;
	barier.dstQueueFamilyIndex = vk.graphics_queue;
	vkCmdPipelineBarrier( vk.cur_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
}


void FrameRenderBuffer::init(int width_, int height_) {
	width = width_;
	height = height_;
	auto vk = Global::vk();

	img.init(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	depth_stencil.init(width, height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

	int msaa_sample_count(1);
	// Create a renderpass
	uint32_t n_attach = 2;
	VkAttachmentDescription att_desc[ 2 ];
	VkAttachmentReference att_ref[ 2 ];
	att_ref[ 0 ].attachment = 0;
	att_ref[ 0 ].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	att_ref[ 1 ].attachment = 1;
	att_ref[ 1 ].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	att_desc[ 0 ].format = VK_FORMAT_R8G8B8A8_SRGB;
	att_desc[ 0 ].samples = (VkSampleCountFlagBits) msaa_sample_count;
	att_desc[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	att_desc[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	att_desc[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att_desc[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	att_desc[ 0 ].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	att_desc[ 0 ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	att_desc[ 0 ].flags = 0;

	att_desc[ 1 ].format = VK_FORMAT_D32_SFLOAT;
	att_desc[ 1 ].samples = (VkSampleCountFlagBits) msaa_sample_count;
	att_desc[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	att_desc[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	att_desc[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att_desc[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	att_desc[ 1 ].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	att_desc[ 1 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	att_desc[ 1 ].flags = 0;

	VkSubpassDescription subpassci = { };
	subpassci.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassci.flags = 0;
	subpassci.inputAttachmentCount = 0;
	subpassci.pInputAttachments = NULL;
	subpassci.colorAttachmentCount = 1;
	subpassci.pColorAttachments = &att_ref[ 0 ];
	subpassci.pResolveAttachments = NULL;
	subpassci.pDepthStencilAttachment = &att_ref[ 1 ];
	subpassci.preserveAttachmentCount = 0;
	subpassci.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderpass_ci = { };
	renderpass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpass_ci.flags = 0;
	renderpass_ci.attachmentCount = 2;
	renderpass_ci.pAttachments = &att_desc[ 0 ];
	renderpass_ci.subpassCount = 1;
	renderpass_ci.pSubpasses = &subpassci;
	renderpass_ci.dependencyCount = 0;
	renderpass_ci.pDependencies = NULL;

	check( vkCreateRenderPass( vk.dev, &renderpass_ci, NULL, &render_pass ), "vkCreateRenderPass");

	// Create the framebuffer
	VkImageView attachments[ 2 ] = { img.view, depth_stencil.view };
	VkFramebufferCreateInfo fb_ci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	fb_ci.renderPass = render_pass;
	fb_ci.attachmentCount = 2;
	fb_ci.pAttachments = &attachments[ 0 ];
	fb_ci.width = width;
	fb_ci.height = height;
	fb_ci.layers = 1;
	check( vkCreateFramebuffer( vk.dev, &fb_ci, NULL, &framebuffer), "vkCreateFramebuffer");

	img.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_stencil.layout = VK_IMAGE_LAYOUT_UNDEFINED;

}


void FrameRenderBuffer::start_render_pass() {
	// Start the renderpass
	VkRenderPassBeginInfo renderpassci = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderpassci.renderPass = render_pass;
	renderpassci.framebuffer = framebuffer;
	renderpassci.renderArea.offset.x = 0;
	renderpassci.renderArea.offset.y = 0;
	renderpassci.renderArea.extent.width = width;
	renderpassci.renderArea.extent.height = height;
	renderpassci.clearValueCount = 2;
	VkClearValue cv[ 2 ];
	cv[ 0 ].color.float32[ 0 ] = 0.0f;
	cv[ 0 ].color.float32[ 1 ] = 0.0f;
	cv[ 0 ].color.float32[ 2 ] = 0.0f;
	cv[ 0 ].color.float32[ 3 ] = 1.0f;
	cv[ 1 ].depthStencil.depth = 1.0f;
	cv[ 1 ].depthStencil.stencil = 0;
	renderpassci.pClearValues = &cv[ 0 ];

	vkCmdBeginRenderPass( Global::vk().cur_cmd_buffer, &renderpassci, VK_SUBPASS_CONTENTS_INLINE );
}

void FrameRenderBuffer::end_render_pass() {
	vkCmdEndRenderPass( Global::vk().cur_cmd_buffer );
}
