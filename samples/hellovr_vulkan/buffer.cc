#include "buffer.h"



Buffer::Buffer() {}

Buffer::Buffer(size_T size, VkBufferUsageFlags usage) {
	init(size, usage);
}

template <typename T>
void Buffer::init(size_T size, VkBufferUsageFlags usage, Location loc, std::vector<T> &init_data) {
	init(size, usage, loc);

	auto vk = Global::vk();
	void *data(0);
	check( vkMapMemory( vk.device, memory, 0, VK_WHOLE_SIZE, 0, &data ), "vkMapMemory");

	memcpy( data, &init_data[0], sizeof(T) * init_data.size() );

	vkUnmapMemory(vk.device , memory);

	VkMappedMemoryRange mem_range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
	mem_range.memory = memory;
	mem_range.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges( vk.device(), 1, &mem_range );
}

void Buffer::init(size_T size, VkBufferUsageFlags usage, Location loc) {
	auto vk = Global::vk();
// Create the vertex buffer and fill with data
	VkBufferCreateInfo bci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bci.size = size;
	bci.usage = usage;
	check( vkCreateBuffer( vk.dev, &bci, nullptr, &buffer ), "vkCreateBuffer");
	
	VkMemoryRequirements memreq = {};
	vkGetBufferMemoryRequirements( vk.deve, *buffer, &memreq );

	VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	alloc_info.memoryTypeIndex = get_mem_type( memreq.memoryTypeBits, 
		(loc == HOST) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 
		(loc == HOST_COHERENT) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
	alloc_info.allocationSize = memreq.size;

	check( vkAllocateMemory( vk.dev, &alloc_info, nullptr, &memory ), "vkCreateBuffer" );
	
	check( vkBindBufferMemory( vk.dev, &buffer, &memory, 0 ), "vkBindBufferMemory" );
};


Image::Image() {}

Image::Image(int width, int height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
	init(width, height, format, usage);
}

void Image::init(int width, int height, VkFormat format, VkImageUsageFlags usage, Vk) {
	auto vk = Global::vk();

	int msaa_sample_count(1);
	VkImageCreateInfo imgci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imgci.imageType = VK_IMAGE_TYPE_2D;
	imgci.extent.width = width;
	imgci.extent.height = height;
	imgci.extent.depth = 1;
	imgci.mipLevels = 1;
	imgci.arrayLayers = 1;
	imgci.format = VK_FORMAT_R8G8B8A8_SRGB;
	imgci.tiling = VK_IMAGE_TILING_OPTIMAL;
	imgci.samples = ( VkSampleCountFlagBits ) msaa_sample_count;
	imgci.usage = ( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT );
	imgci.flags = 0;	

	check( vkCreateImage( vk.device, &imgci, nullptr, &img ), "vkCreateImage");

	VkMemoryRequirements mem_req = {};
	vkGetImageMemoryRequirements( vk.device, img, &mem_req );

	VkMemoryAllocateInfo mem_alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	mem_alloc_info.allocationSize = mem_req.size;
	mem_alloc_info.memoryTypeIndex = get_mem_type( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	check( vkAllocateMemory( vk.device, &mem_alloc_info, nullptr, &mem), "vkAllocateMemory");
	check( vkBindImageMemory( vk.device, img, mem, 0 ), "vkBindImageMemory");

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
	check( vkCreateImageView( device, &img_view_ci, nullptr, &view ));
}

void FrameRenderBuffer::init(int width_, int height_) {
	width = width_;
	height = height_;
	auto vk = Global::vk();

	image.init(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
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
	att_desc[ 0 ].samples = msaa_sample_count;
	att_desc[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	att_desc[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	att_desc[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att_desc[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	att_desc[ 0 ].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	att_desc[ 0 ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	att_desc[ 0 ].flags = 0;

	att_desc[ 1 ].format = VK_FORMAT_D32_SFLOAT;
	att_desc[ 1 ].samples = msaa_sample_count;
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

	check( vkCreateRenderPass( vk.device, &renderpass_ci, NULL, &render_pass ), "vkCreateRenderPass");

	// Create the framebuffer
	VkImageView attachments[ 2 ] = { image.view, depth_stencil.view };
	VkFramebufferCreateInfo fb_ci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	fb_ci.renderPass = render_pass;
	fb_ci.attachmentCount = 2;
	fb_ci.pAttachments = &attachments[ 0 ];
	fb_ci.width = width;
	fb_ci.height = height;
	fb_ci.layers = 1;
	check( vkCreateFramebuffer( vk.device, &fb_ci, NULL, &framebuffer), "vkCreateFramebuffer");

	framebufferDesc.m_nImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	framebufferDesc.m_nDepthStencilImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

}

void FrameRenderBuffer::to_colour_optimal() {
	auto vk = Global::vk();
	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
	barier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barier.oldLayout = image.layout;
	barier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barier.image = image.img;
	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barier.subresourceRange.baseMipLevel = 0;
	barier.subresourceRange.levelCount = 1;
	barier.subresourceRange.baseArrayLayer = 0;
	barier.subresourceRange.layerCount = 1;
	barier.srcQueueFamilyIndex = vk.graphics_queue;
	barier.dstQueueFamilyIndex = vk.graphics_queue;
	vkCmdPipelineBarrier( vk.cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
	image.layout = barier.newLayout;
}


void FrameRenderBuffer::to_depth_optimal() {
	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barier.image = depth_stencil.img;
	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barier.subresourceRange.baseMipLevel = 0;
	barier.subresourceRange.levelCount = 1;
	barier.subresourceRange.baseArrayLayer = 0;
	barier.subresourceRange.layerCount = 1;
	barier.srcAccessMask = 0;
	barier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barier.oldLayout = depth_stencil.layout;
	barier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	vkCmdPipelineBarrier( Global::vk().cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
	depth_stencil.layout = barier.newLayout;
}

void FrameRenderBuffer::to_read_optimal() {
	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barier.image = image.img;
	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barier.subresourceRange.baseMipLevel = 0;
	barier.subresourceRange.levelCount = 1;
	barier.subresourceRange.baseArrayLayer = 0;
	barier.subresourceRange.layerCount = 1;
	barier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barier.oldLayout = m_leftEyeDesc.m_nImageLayout;
	barier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier( Global::vk().cmd_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
	image.layout = barier.newLayout;
}


void FrameRenderBuffer::start_render_pass() {
	// Start the renderpass
	VkRenderPassBeginInfo renderpassci = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderpassci.renderPass = renderpass;
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

	vkCmdBeginRenderPass( Global::vk().cmd_buffer, &renderpassci, VK_SUBPASS_CONTENTS_INLINE );
}

void FrameRenderBuffer::end_render_pass() {
	vkCmdEndRenderPass( Global::vk().cmd_buffer );
}