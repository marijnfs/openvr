#include "vulkansystem.h"
#include "global.h"
#include "util.h"
#include "shared/lodepng.h"

using namespace std;

//  ==== FENCED BUFFER ====
void FencedCommandBuffer::begin() {
	VkCommandBufferBeginInfo cmbbi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer( cmd_buffer, &cmbbi );
}

void FencedCommandBuffer::end() {
	vkEndCommandBuffer( cmd_buffer );
}

bool FencedCommandBuffer::finished() {
	return vkGetFenceStatus( Global::vk().dev, fence ) == VK_SUCCESS;	
}

void FencedCommandBuffer::reset() {
	vkResetCommandBuffer( cmd_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
	vkResetFences( Global::vk().dev, 1, &fence );
}

void FencedCommandBuffer::init() {
	auto &vk = Global::vk();
	VkCommandBufferAllocateInfo cmd_buffer_alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmd_buffer_alloc_info.commandBufferCount = 1;
	cmd_buffer_alloc_info.commandPool = vk.cmd_pool;
	cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers( vk.dev, &cmd_buffer_alloc_info, &cmd_buffer );

	VkFenceCreateInfo fenceci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	vkCreateFence( vk.dev, &fenceci, nullptr, &fence );
}

// ==== Render Model ====

void RenderModel::init() {
	int vert_count(0);
	int idx_count(0);

	vertex_buf.init(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof( vr::RenderModel_Vertex_t ) * vert_count, HOST);
	index_buf.init(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(uint16_t) * idx_count, HOST);

}



// ==== Graphics Object ====
void GraphicsObject::draw() {
		//TODO fix
	auto vk = Global::vk();
	vkCmdBindPipeline( vk.cur_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipelines[ PSO_SCENE ] );

		// Update the persistently mapped pointer to the CB data with the latest matrix, TODO: SET THIS SOMEWHERE
	//memcpy( m_pSceneConstantBufferData[ nEye ], GetCurrentViewProjectionMatrix( nEye ).get(), sizeof( Matrix4 ) );

	vkCmdBindDescriptorSets( vk.cur_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, 0, 1, &desc.desc, 0, nullptr );

		// Draw
	VkDeviceSize nOffsets[ 1 ] = { 0 };
	vkCmdBindVertexBuffers( vk.cur_cmd_buffer, 0, 1, &vertex_buf.buffer, &nOffsets[ 0 ] );
	vkCmdDraw( vk.cur_cmd_buffer, n_vertex, 1, 0, 0 );
}

void GraphicsObject::init_cube(Matrix4 pos) {
	Vector4 A = pos * Vector4( 0, 0, 0, 1 );
	Vector4 B = pos * Vector4( 1, 0, 0, 1 );
	Vector4 C = pos * Vector4( 1, 1, 0, 1 );
	Vector4 D = pos * Vector4( 0, 1, 0, 1 );
	Vector4 E = pos * Vector4( 0, 0, 1, 1 );
	Vector4 F = pos * Vector4( 1, 0, 1, 1 );
	Vector4 G = pos * Vector4( 1, 1, 1, 1 );
	Vector4 H = pos * Vector4( 0, 1, 1, 1 );

	// triangles instead of quads
	add_vertex( E.x, E.y, E.z, 0, 1); //Front
	add_vertex( F.x, F.y, F.z, 1, 1);
	add_vertex( G.x, G.y, G.z, 1, 0);
	add_vertex( G.x, G.y, G.z, 1, 0);
	add_vertex( H.x, H.y, H.z, 0, 0);
	add_vertex( E.x, E.y, E.z, 0, 1);

add_vertex( B.x, B.y, B.z, 0, 1); //Back
add_vertex( A.x, A.y, A.z, 1, 1);
add_vertex( D.x, D.y, D.z, 1, 0);
add_vertex( D.x, D.y, D.z, 1, 0);
add_vertex( C.x, C.y, C.z, 0, 0);
add_vertex( B.x, B.y, B.z, 0, 1);

add_vertex( H.x, H.y, H.z, 0, 1); //Top
add_vertex( G.x, G.y, G.z, 1, 1);
add_vertex( C.x, C.y, C.z, 1, 0);
add_vertex( C.x, C.y, C.z, 1, 0);
add_vertex( D.x, D.y, D.z, 0, 0);
add_vertex( H.x, H.y, H.z, 0, 1);

add_vertex( A.x, A.y, A.z, 0, 1); //Bottom
add_vertex( B.x, B.y, B.z, 1, 1);
add_vertex( F.x, F.y, F.z, 1, 0);
add_vertex( F.x, F.y, F.z, 1, 0);
add_vertex( E.x, E.y, E.z, 0, 0);
add_vertex( A.x, A.y, A.z, 0, 1);

add_vertex( A.x, A.y, A.z, 0, 1); //Left
add_vertex( E.x, E.y, E.z, 1, 1);
add_vertex( H.x, H.y, H.z, 1, 0);
add_vertex( H.x, H.y, H.z, 1, 0);
add_vertex( D.x, D.y, D.z, 0, 0);
add_vertex( A.x, A.y, A.z, 0, 1);

add_vertex( F.x, F.y, F.z, 0, 1); //Right
add_vertex( B.x, B.y, B.z, 1, 1);
add_vertex( C.x, C.y, C.z, 1, 0);
add_vertex( C.x, C.y, C.z, 1, 0);
add_vertex( G.x, G.y, G.z, 0, 0);
add_vertex( F.x, F.y, F.z, 0, 1);
}

void GraphicsObject::add_vertex(float fl0, float fl1, float fl2, float fl3, float fl4) {
	v.push_back( fl0 );
	v.push_back( fl1 );
	v.push_back( fl2 );
	v.push_back( fl3 );
	v.push_back( fl4 );
}

// ===== SwapChain =======
void Swapchain::init() {
	auto window = Global::ws();
	auto vk = Global::vk();

	SDL_SysWMinfo wm_info;
	SDL_VERSION( &wm_info.version );
	SDL_GetWindowWMInfo( window.window, &wm_info );

#ifdef VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
	win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32SurfaceCreateInfo.pNext = NULL;
	win32SurfaceCreateInfo.flags = 0;
	win32SurfaceCreateInfo.hinstance = GetModuleHandle( NULL );
	win32SurfaceCreateInfo.hwnd = ( HWND ) wm_info.info.win.window;
	check( vkCreateWin32SurfaceKHR( inst, &win32SurfaceCreateInfo, nullptr, &surface ), "vkCreateWin32SurfaceKHR");
#else
	VkXlibSurfaceCreateInfoKHR xlib_ci = { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
	xlib_ci.flags = 0;
	xlib_ci.dpy = wm_info.info.x11.display;
	xlib_ci.window = wm_info.info.x11.window;
	check( vkCreateXlibSurfaceKHR( vk.inst, &xlib_ci, nullptr, &surface ), "vkCreateXlibSurfaceKHR" );
#endif

	VkBool32 supports_present = VK_FALSE;
	check( vkGetPhysicalDeviceSurfaceSupportKHR( vk.phys_dev, vk.graphics_queue, surface, &supports_present), "vkGetPhysicalDeviceSurfaceSupportKHR");
	if (supports_present == VK_FALSE) {
		cerr << "support not present, vkGetPhysicalDeviceSurfaceSupportKHR" << endl;
		throw StringException("support not present, vkGetPhysicalDeviceSurfaceSupportKHR");
	}

//query supported formats
	VkFormat swap_format;
	uint32_t format_index(0);
	uint32_t n_swap_format(0);
	VkColorSpaceKHR color_space;

	check( vkGetPhysicalDeviceSurfaceFormatsKHR( vk.phys_dev, surface, &n_swap_format, NULL), "vkGetPhysicalDeviceSurfaceFormatsKHR");
	vector<VkSurfaceFormatKHR> swap_formats(n_swap_format);
	check( vkGetPhysicalDeviceSurfaceFormatsKHR( vk.phys_dev, surface, &n_swap_format, &swap_formats[0]), "vkGetPhysicalDeviceSurfaceFormatsKHR");

	for (int i(0); i < n_swap_format; ++i) {
		if (swap_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || swap_formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
			format_index = i;
			break;
		}
	}
	swap_format = swap_formats[format_index].format;
	color_space = swap_formats[format_index].colorSpace;


//check capabilities
	VkSurfaceCapabilitiesKHR surface_caps = {};
	check( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk.phys_dev, surface, &surface_caps ), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

	uint32_t n_present_modes(0);
	check( vkGetPhysicalDeviceSurfacePresentModesKHR( vk.phys_dev, surface, &n_present_modes, NULL ), "vkGetPhysicalDeviceSurfacePresentModesKHR");
	vector<VkPresentModeKHR> present_modes(n_present_modes);
	check( vkGetPhysicalDeviceSurfacePresentModesKHR( vk.phys_dev, surface, &n_present_modes, &present_modes[0] ), "vkGetPhysicalDeviceSurfacePresentModesKHR");

//create extent
	VkExtent2D swapchain_extent;
	if ( surface_caps.currentExtent.width == -1 ) {
	// If the surface size is undefined, the size is set to the size of the images requested.
		swapchain_extent.width = window.width;
		swapchain_extent.height = window.height;
	} else {
	// If the surface size is defined, the swap chain size must match
		swapchain_extent = surface_caps.currentExtent;
	}


//find best present mode
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (auto mode : present_modes) {
		if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			present_mode = mode;
			break;
		}
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			present_mode = mode;
		if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR && mode != VK_PRESENT_MODE_MAILBOX_KHR)
			present_mode = mode;	
	}

	n_swap = surface_caps.minImageCount;
	if (n_swap < 2) n_swap = 2;
	if (surface_caps.maxImageCount > 0 && n_swap > surface_caps.maxImageCount)
		n_swap = surface_caps.maxImageCount;

	VkSurfaceTransformFlagsKHR pre_transform;
	if (surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	else
		pre_transform = surface_caps.currentTransform;

	VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if ( surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
		image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	else
		cerr << "Vulkan swapchain does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT. Some operations may not be supported.\n" << endl;


	VkSwapchainCreateInfoKHR scci = {};
	scci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	scci.pNext = NULL;
	scci.surface = surface;
	scci.minImageCount = n_swap;
	scci.imageFormat = swap_format;
	scci.imageColorSpace = color_space;
	scci.imageExtent = swapchain_extent;
	scci.imageUsage = image_usage;
	scci.preTransform = ( VkSurfaceTransformFlagBitsKHR ) pre_transform;
	scci.imageArrayLayers = 1;
	scci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	scci.queueFamilyIndexCount = 0;
	scci.pQueueFamilyIndices = NULL;
	scci.presentMode = present_mode;
	scci.clipped = VK_TRUE;


	if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		scci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	else if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
		scci.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	else
		cerr << "Unexpected value for VkSurfaceCapabilitiesKHR.compositeAlpha:" << surface_caps.supportedCompositeAlpha << endl;

	check( vkCreateSwapchainKHR( vk.dev, &scci, NULL, &swapchain), "vkCreateSwapchainKHR");

	check( vkGetSwapchainImagesKHR(vk.dev, swapchain, &n_swap, NULL), "vkGetSwapchainImagesKHR");
	images.resize(n_swap);
	check( vkGetSwapchainImagesKHR(vk.dev, swapchain, &n_swap, &images[0]), "vkGetSwapchainImagesKHR");


// Create a renderpass
	uint32_t n_att = 1;
	VkAttachmentDescription att_desc;
	VkAttachmentReference att_ref;
	att_ref.attachment = 0;
	att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	att_desc.format = swap_format;
	att_desc.samples = VK_SAMPLE_COUNT_1_BIT;
	att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	att_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	att_desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	att_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	att_desc.flags = 0;

	VkSubpassDescription subpassci = { };
	subpassci.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassci.flags = 0;
	subpassci.inputAttachmentCount = 0;
	subpassci.pInputAttachments = NULL;
	subpassci.colorAttachmentCount = 1;
	subpassci.pColorAttachments = &att_ref;
	subpassci.pResolveAttachments = NULL;
	subpassci.pDepthStencilAttachment = NULL;
	subpassci.preserveAttachmentCount = 0;
	subpassci.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderpassci = { };
	renderpassci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassci.flags = 0;
	renderpassci.attachmentCount = 1;
	renderpassci.pAttachments = &att_desc;
	renderpassci.subpassCount = 1;
	renderpassci.pSubpasses = &subpassci;
	renderpassci.dependencyCount = 0;
	renderpassci.pDependencies = NULL;

	check( vkCreateRenderPass( vk.dev, &renderpassci, NULL, &renderpass), "vkCreateRenderPass");

	for (int i(0); i < images.size(); ++i) {
		VkImageViewCreateInfo viewci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewci.flags = 0;
		viewci.image = images[ i ];
		viewci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewci.format = swap_format;
		viewci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		viewci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewci.subresourceRange.baseMipLevel = 0;
		viewci.subresourceRange.levelCount = 1;
		viewci.subresourceRange.baseArrayLayer = 0;
		viewci.subresourceRange.layerCount = 1;
		VkImageView image_view = VK_NULL_HANDLE;
		vkCreateImageView( vk.dev, &viewci, nullptr, &image_view );
		
		views.push_back( image_view );

		VkImageView attachments[ 1 ] = { image_view };
		VkFramebufferCreateInfo fbci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbci.renderPass = renderpass;
		fbci.attachmentCount = 1;
		fbci.pAttachments = &attachments[ 0 ];
		fbci.width = window.width;
		fbci.height = window.height;
		fbci.layers = 1;
		VkFramebuffer framebuffer;
		check( vkCreateFramebuffer( vk.dev, &fbci, NULL, &framebuffer ), "vkCreateFramebuffer");
		framebuffers.push_back( framebuffer );

		VkSemaphoreCreateInfo semci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkSemaphore semaphore = VK_NULL_HANDLE;
		vkCreateSemaphore( vk.dev, &semci, nullptr, &semaphore );
		semaphores.push_back( semaphore );
	}
}

// ==== Vulkan System====


VulkanSystem::VulkanSystem() {
	init();

}

void VulkanSystem::submit(FencedCommandBuffer fcb) {
	VkSubmitInfo submiti = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submiti.commandBufferCount = 1;
	submiti.pCommandBuffers = &fcb.cmd_buffer;
	vkQueueSubmit( queue, 1, &submiti, fcb.fence );
}

void VulkanSystem::wait_queue() {
	vkQueueWaitIdle( queue );
}

void VulkanSystem::init_instance() {


	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.pApplicationName = "hellovr_vulkan";
	app_info.applicationVersion = 1;
	app_info.pEngineName = nullptr;
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_MAKE_VERSION( 1, 0, 0 );


	int layer_count(0);
	auto inst_req = Global::vr().get_inst_ext_required_verified();
	char *inst_req_charp[inst_req.size()];
	for (int i(0); i < inst_req.size(); ++i)
		inst_req_charp[i] = (char*)inst_req[i].c_str();

	VkInstanceCreateInfo ici = {};
	ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	ici.pNext = NULL;
	ici.pApplicationInfo = &app_info;
	ici.enabledExtensionCount = inst_req.size();
	ici.ppEnabledExtensionNames = inst_req_charp;
	ici.enabledLayerCount = layer_count;
	ici.ppEnabledLayerNames = 0; //might need validation layers later

	check( vkCreateInstance( &ici, nullptr, &inst), "Create Instance");
}

void VulkanSystem::init_device() {
	auto vr = Global::vr();

	uint32_t n_dev(0);
	check( vkEnumeratePhysicalDevices( inst, &n_dev, NULL ), "vkEnumeratePhysicalDevices");
	vector<VkPhysicalDevice> devices(n_dev);
	check( vkEnumeratePhysicalDevices( inst, &n_dev, &devices[0] ), "vkEnumeratePhysicalDevices");

	VkPhysicalDevice chosen_dev = devices[0]; //select first, could be altered

	vkGetPhysicalDeviceProperties( chosen_dev, &prop);
	vkGetPhysicalDeviceMemoryProperties( chosen_dev, &mem_prop );
	vkGetPhysicalDeviceFeatures( chosen_dev, &features );


	uint32_t n_queue(0);
	vkGetPhysicalDeviceQueueFamilyProperties( chosen_dev, &n_queue, 0);
	vector<VkQueueFamilyProperties> queue_family(n_queue);
	vkGetPhysicalDeviceQueueFamilyProperties( chosen_dev, &n_queue, &queue_family[0]);
	if (n_queue == 0) {
		cerr << "Failed to get queue properties.\n" << endl;
		throw "";
	}

	graphics_queue = -1;
	for (int i(0); i < queue_family.size(); ++i) {
		if (queue_family[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphics_queue = i;
			break;
		}
	}

	if (graphics_queue < 0) {
		cerr << "no graphics queue" << endl;
		throw "";
	}

	auto dev_ext = vr.get_dev_ext_required_verified();
	// Add additional required extensions
	dev_ext.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );

	vector<char *> pp_dev_ext(dev_ext.size());
	for (int i(0); i < dev_ext.size(); ++i)
		pp_dev_ext[i] = (char*) dev_ext[i].c_str();

	//create device
	// Create the device
	VkDeviceQueueCreateInfo dqci = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	dqci.queueFamilyIndex = graphics_queue;
	dqci.queueCount = 1;
	float fQueuePriority = 1.0f;
	dqci.pQueuePriorities = &fQueuePriority;

	VkDeviceCreateInfo dci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &dqci;
	dci.enabledExtensionCount = dev_ext.size();
	dci.ppEnabledExtensionNames = &pp_dev_ext[0];
	dci.pEnabledFeatures = &features;

	check( vkCreateDevice( chosen_dev, &dci, nullptr, &dev ), "vkCreateDevice");

	vkGetDeviceQueue( dev, graphics_queue, 0, &queue );
}


void Descriptor::init() {
	auto vk = Global::vk();
	//idx = vk.desc_sets.size();
	//vk.desc_sets.push_back(Descriptor());

	VkDescriptorSetAllocateInfo desc_inf = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	desc_inf.descriptorPool = vk.desc_pool;
	desc_inf.descriptorSetCount = 1;
	desc_inf.pSetLayouts = &vk.desc_set_layout;
	vkAllocateDescriptorSets( vk.dev, &desc_inf, &desc );
}

void Descriptor::register_texture(VkImageView view) {
	auto vk = Global::vk();

	VkDescriptorImageInfo img_i = {};
	img_i.imageView = view;
	img_i.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet write_desc[ 1 ] = { };
	write_desc[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc[ 0 ].dstSet = desc;
	write_desc[ 0 ].dstBinding = 1;
	write_desc[ 0 ].descriptorCount = 1;
	write_desc[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write_desc[ 0 ].pImageInfo = &img_i;
	vkUpdateDescriptorSets( vk.dev, _countof( write_desc ), write_desc, 0, nullptr );

	img_i.imageView = view;
	write_desc[ 0 ].dstSet = desc;
	vkUpdateDescriptorSets( vk.dev, _countof( write_desc ), write_desc, 0, nullptr );
}

void Descriptor::register_model_texture(VkBuffer buf, VkImageView view, VkSampler sampler) {
	auto vk = Global::vk();

	VkDescriptorBufferInfo buf_inf = {};
	buf_inf.buffer = buf;
	buf_inf.offset = 0;
	buf_inf.range = VK_WHOLE_SIZE;

	VkDescriptorImageInfo img_inf = {};
	img_inf.imageView = view;
	img_inf.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo sample_info = {};
	sample_info.sampler = sampler;

	VkWriteDescriptorSet write_desc_set[ 3 ] = { };
	write_desc_set[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc_set[ 0 ].dstSet = desc;
	write_desc_set[ 0 ].dstBinding = 0;
	write_desc_set[ 0 ].descriptorCount = 1;
	write_desc_set[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_desc_set[ 0 ].pBufferInfo = &buf_inf;
	write_desc_set[ 1 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc_set[ 1 ].dstSet = desc;
	write_desc_set[ 1 ].dstBinding = 1;
	write_desc_set[ 1 ].descriptorCount = 1;
	write_desc_set[ 1 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write_desc_set[ 1 ].pImageInfo = &img_inf;
	write_desc_set[ 2 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc_set[ 2 ].dstSet = desc;
	write_desc_set[ 2 ].dstBinding = 2;
	write_desc_set[ 2 ].descriptorCount = 1;
	write_desc_set[ 2 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	write_desc_set[ 2 ].pImageInfo = &sample_info;

	vkUpdateDescriptorSets( vk.dev, _countof( write_desc_set ), write_desc_set, 0, nullptr );
}

void Descriptor::bind() {	
	auto vk = Global::vk();
	vkCmdBindDescriptorSets( vk.cur_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, 0, 1, &desc, 0, nullptr );



	// vkCmdBindPipeline( m_currentCommandBuffer.m_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelines[ PSO_SCENE ] );
	
	// // Update the persistently mapped pointer to the CB data with the latest matrix
	// memcpy( m_pSceneConstantBufferData[ nEye ], GetCurrentViewProjectionMatrix( nEye ).get(), sizeof( Matrix4 ) );

	// vkCmdBindDescriptorSets( m_currentCommandBuffer.m_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSets[ DESCRIPTOR_SET_LEFT_EYE_SCENE + nEye ], 0, nullptr );

	// // Draw
	// VkDeviceSize nOffsets[ 1 ] = { 0 };
	// vkCmdBindVertexBuffers( m_currentCommandBuffer.m_pCommandBuffer, 0, 1, &m_pSceneVertexBuffer, &nOffsets[ 0 ] );
}


void VulkanSystem::init() {
	init_instance();
	init_device();
	
	init_descriptor_sets();
	init_shaders();
}

void VulkanSystem::init_descriptor_sets() {
	//Create pool
	size_t NUM_DESCRIPTOR_SETS(256);

	VkDescriptorPoolSize pool_sizes[ 3 ];
	pool_sizes[ 0 ].descriptorCount = NUM_DESCRIPTOR_SETS;
	pool_sizes[ 0 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[ 1 ].descriptorCount = NUM_DESCRIPTOR_SETS;
	pool_sizes[ 1 ].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	pool_sizes[ 2 ].descriptorCount = NUM_DESCRIPTOR_SETS;
	pool_sizes[ 2 ].type = VK_DESCRIPTOR_TYPE_SAMPLER;

	VkDescriptorPoolCreateInfo descpool_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descpool_ci.flags = 0;
	descpool_ci.maxSets = NUM_DESCRIPTOR_SETS;
	descpool_ci.poolSizeCount = _countof( pool_sizes );
	descpool_ci.pPoolSizes = &pool_sizes[ 0 ];
	vkCreateDescriptorPool( dev, &descpool_ci, nullptr, &desc_pool );


	//Create Layout
	VkDescriptorSetLayoutBinding lb[3] = {};
	lb[0].binding = 0;
	lb[0].descriptorCount = 1;
	lb[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lb[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	lb[1].binding = 1;
	lb[1].descriptorCount = 1;
	lb[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	lb[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	lb[2].binding = 2;
	lb[2].descriptorCount = 1;
	lb[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	lb[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo desc_set_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	desc_set_ci.bindingCount = 3;
	desc_set_ci.pBindings = &lb[ 0 ];
	check( vkCreateDescriptorSetLayout( dev, &desc_set_ci, nullptr, &desc_set_layout ), "vkCreateDescriptorSetLayout");


}



void VulkanSystem::init_shaders() {
	//Create Shaders, probably most involved part
	//Init desc sets first
	vector<string> shader_names = {
		"scene",
		"axes",
		"rendermodel",
		"companion"
	};
	vector<string> stages = {
		"vs",
		"ps"
	};

	int i(0);
	for (auto shader_name : shader_names) {
		for (auto stage : stages) {
			string path = "../shaders/" + shader_name + "_" + stage + ".spv";
			string code = read_all(path);

			VkShaderModuleCreateInfo shader_ci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
			shader_ci.codeSize = code.size();
			shader_ci.pCode = ( const uint32_t *) &code[0];
			check(vkCreateShaderModule( dev, &shader_ci, nullptr, (stage == "vs") ? &shader_modules_vs[i] : &shader_modules_ps[i] ), "vkCreateShaderModule");
		}
		++i;
	}


//Create pipelines, first layout
	VkPipelineLayoutCreateInfo pipeline_ci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_ci.pNext = NULL;
	pipeline_ci.setLayoutCount = 1;
	pipeline_ci.pSetLayouts = &desc_set_layout;
	pipeline_ci.pushConstantRangeCount = 0;
	pipeline_ci.pPushConstantRanges = NULL;
	check( vkCreatePipelineLayout( dev, &pipeline_ci, nullptr, &pipeline_layout ), "vkCreatePipelineLayout");

// Create pipeline cache
	VkPipelineCacheCreateInfo pipeline_cache_ci = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	vkCreatePipelineCache( dev, &pipeline_cache_ci, NULL, &pipeline_cache );

	VkRenderPass render_passes[ PSO_COUNT ] =
	{
		swapchain.renderpass,
		swapchain.renderpass,
		swapchain.renderpass,
	 	swapchain.companion_renderpass
	};

//define strides for data used in shaders
	size_t strides[ PSO_COUNT ] =
	{
		sizeof( Pos3Tex2 ),			// PSO_SCENE
		sizeof( float ) * 6,				// PSO_AXES
		sizeof( vr::RenderModel_Vertex_t ),	// PSO_RENDERMODEL
		sizeof( Pos2Tex2 )			// PSO_COMPANION
    };

	VkVertexInputAttributeDescription attr_desc[ PSO_COUNT * 3 ]
	{
		// PSO_SCENE
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT,	0 },
		{ 1, 0, VK_FORMAT_R32G32_SFLOAT,	offsetof( Pos3Tex2, texpos ) },
		{ 0, 0, VK_FORMAT_UNDEFINED,		0 },
		// PSO_AXES
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT,	0 },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT,	sizeof( float ) * 3 },
		{ 0, 0, VK_FORMAT_UNDEFINED,		0 },
		// PSO_RENDERMODEL
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT,	0 },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT,	offsetof( vr::RenderModel_Vertex_t, vNormal ) },
		{ 2, 0, VK_FORMAT_R32G32_SFLOAT,	offsetof( vr::RenderModel_Vertex_t, rfTextureCoord ) },
		// PSO_COMPANION
		{ 0, 0, VK_FORMAT_R32G32_SFLOAT,	0 },
		{ 1, 0, VK_FORMAT_R32G32_SFLOAT,	sizeof( float ) * 2 },
		{ 0, 0, VK_FORMAT_UNDEFINED,		0 },
	};

	// Create the PSOs
	for ( uint32_t pso = 0; pso < PSO_COUNT; pso++ )
	{
		VkGraphicsPipelineCreateInfo pipeline_ci = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		
		// VkPipelineVertexInputStateCreateInfo
		VkVertexInputBindingDescription binding_ci;
		binding_ci.binding = 0;
		binding_ci.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		binding_ci.stride = strides[ pso ];
		
		VkPipelineVertexInputStateCreateInfo vertexi_ci = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		for ( uint32_t attr = 0; attr < 3; attr++ )
		{
			if ( attr_desc[ pso * 3 + attr ].format != VK_FORMAT_UNDEFINED )
			{
				vertexi_ci.vertexAttributeDescriptionCount++;
			}
		}
		vertexi_ci.pVertexAttributeDescriptions = &attr_desc[ pso * 3 ];
		vertexi_ci.vertexBindingDescriptionCount = 1;
		vertexi_ci.pVertexBindingDescriptions = &binding_ci;

		// VkPipelineDepthStencilStateCreateInfo
		VkPipelineDepthStencilStateCreateInfo depth_ci = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depth_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_ci.depthTestEnable = ( pso != PSO_COMPANION ) ? VK_TRUE : VK_FALSE;
		depth_ci.depthWriteEnable = ( pso != PSO_COMPANION ) ? VK_TRUE : VK_FALSE;
		depth_ci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depth_ci.depthBoundsTestEnable = VK_FALSE;
		depth_ci.stencilTestEnable = VK_FALSE;
		depth_ci.minDepthBounds = 0.0f;
		depth_ci.maxDepthBounds = 0.0f;

		// VkPipelineColorBlendStateCreateInfo
		VkPipelineColorBlendStateCreateInfo colorblend_ci = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorblend_ci.logicOpEnable = VK_FALSE;
		colorblend_ci.logicOp = VK_LOGIC_OP_COPY;
		VkPipelineColorBlendAttachmentState color_attach_state = {};
		color_attach_state.blendEnable = VK_FALSE;
		color_attach_state.colorWriteMask = 0xf;
		colorblend_ci.attachmentCount = 1;
		colorblend_ci.pAttachments = &color_attach_state;

		// VkPipelineColorBlendStateCreateInfo
		VkPipelineRasterizationStateCreateInfo raster_ci = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		raster_ci.polygonMode = VK_POLYGON_MODE_FILL;
		raster_ci.cullMode = VK_CULL_MODE_BACK_BIT;
		raster_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		raster_ci.lineWidth = 1.0f;

		// VkPipelineInputAssemblyStateCreateInfo
		VkPipelineInputAssemblyStateCreateInfo ia_state_ci = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		ia_state_ci.topology = ( pso == PSO_AXES ) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		ia_state_ci.primitiveRestartEnable = VK_FALSE;

		// VkPipelineMultisampleStateCreateInfo
		VkPipelineMultisampleStateCreateInfo multisample_ci = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisample_ci.rasterizationSamples = ( pso == PSO_COMPANION ) ? VK_SAMPLE_COUNT_1_BIT : ( VkSampleCountFlagBits ) msaa;
		multisample_ci.minSampleShading = 0.0f;
		uint32_t sample_mask = 0xFFFFFFFF;
		multisample_ci.pSampleMask = &sample_mask;

		// VkPipelineViewportStateCreateInfo
		VkPipelineViewportStateCreateInfo viewport_ci = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewport_ci.viewportCount = 1;
		viewport_ci.scissorCount = 1;

		VkPipelineShaderStageCreateInfo shader_stages[ 2 ] = { };
		shader_stages[ 0 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[ 0 ].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[ 0 ].module = shader_modules_vs[ pso ];
		shader_stages[ 0 ].pName = "VSMain";
		
		shader_stages[ 1 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[ 1 ].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[ 1 ].module = shader_modules_ps[ pso ];
		shader_stages[ 1 ].pName = "PSMain";

		pipeline_ci.layout = pipeline_layout;

		// Set pipeline states
		pipeline_ci.pVertexInputState = &vertexi_ci;
		pipeline_ci.pInputAssemblyState = &ia_state_ci;
		pipeline_ci.pViewportState = &viewport_ci;
		pipeline_ci.pRasterizationState = &raster_ci;
		pipeline_ci.pMultisampleState = &multisample_ci;
		pipeline_ci.pDepthStencilState = &depth_ci;
		pipeline_ci.pColorBlendState = &colorblend_ci;
		pipeline_ci.stageCount = 2;
		pipeline_ci.pStages = &shader_stages[ 0 ];
		pipeline_ci.renderPass = render_passes[ pso ];

		static VkDynamicState dynamic_states[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		static VkPipelineDynamicStateCreateInfo dynamic_state_ci = {};
		dynamic_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_ci.pNext = NULL;
		dynamic_state_ci.dynamicStateCount = _countof( dynamic_states );
		dynamic_state_ci.pDynamicStates = &dynamic_states[ 0 ];
		pipeline_ci.pDynamicState = &dynamic_state_ci;


		// Create the pipeline
		check( vkCreateGraphicsPipelines( dev, pipeline_cache, 1, &pipeline_ci, NULL, &pipelines[ pso ] ), "vkCreateGraphicsPipelines");
	}
}
/*
void VulkanSystem::init_texture_maps() {
	string tex_path = "../cube_texture.png";

	std::vector< unsigned char > img_rgba;
	unsigned width, height;
	unsigned nError = lodepng::decode( img_rgba, width, height, tex_path.c_str() );

	if ( nError != 0 )
		throw StringException("lodepng: couldn't open texture");

	VkDeviceSize buf_size = 0;
	uint8_t *buf = new uint8_t[ nImageWidth * nImageHeight * 4 * 2 ];
	uint8_t *prev_buf = buf;
	uint8_t *cur_buf = buf;
	memcpy( cur_buf, &img_rgba[0], sizeof( uint8_t ) * width * height * 4 );
	cur_buf += sizeof( uint8_t ) * width * height * 4;

//make several copies for different scales
	std::vector< VkBufferImageCopy > buf_img_copies;
	VkBufferImageCopy buf_img_copy = {};
	buf_img_copy.bufferOffset = 0;
	buf_img_copy.bufferRowLength = 0;
	buf_img_copy.bufferImageHeight = 0;
	buf_img_copy.imageSubresource.baseArrayLayer = 0;
	buf_img_copy.imageSubresource.layerCount = 1;
	buf_img_copy.imageSubresource.mipLevel = 0;
	buf_img_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	buf_img_copy.imageOffset.x = 0;
	buf_img_copy.imageOffset.y = 0;
	buf_img_copy.imageOffset.z = 0;
	buf_img_copy.imageExtent.width = width;
	buf_img_copy.imageExtent.height = height;
	buf_img_copy.imageExtent.depth = 1;
	buf_img_copies.push_back( buf_img_copy );

	int mip_width = width;
	int mip_height = height;

	while( mip_width > 1 && mip_height > 1 )
	{
		gen_mipmap_rgba( prev_buf, cur_buf, mip_width, mip_height, &mip_width, &mip_height );
		buf_img_copy.bufferOffset = cur_buf - buf;
		buf_img_copy.imageSubresource.mipLevel++;
		buf_img_copy.imageExtent.width = mip_width;
		buf_img_copy.imageExtent.height = mip_height;
		buf_img_copies.push_back( buf_img_copy );
		pPrevBuffer = cur_buf;
		cur_buf += ( mip_width * mip_height * 4 * sizeof( uint8_t ) );
	}
	nBufferSize = cur_buf - buf;

//TODO, put to image class
// Create the image
	VkImageCreateInfo img_ci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	img_ci.imageType = VK_IMAGE_TYPE_2D;
	img_ci.extent.width = width;
	img_ci.extent.height = height;
	img_ci.extent.depth = 1;
	img_ci.mipLevels = ( uint32_t ) buf_img_copies.size();
	img_ci.arrayLayers = 1;
	img_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
	img_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_ci.samples = VK_SAMPLE_COUNT_1_BIT;
	img_ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	img_ci.flags = 0;
	vkCreateImage( device, &img_ci, nullptr, &scene_img );

	VkMemoryRequirements mem_req = {};
	vkGetImageMemoryRequirements( device, scene_img, &mem_req );

	VkMemoryAllocateInfo mem_alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	mem_alloc_info.allocationSize = mem_req.size;
	mem_alloc_info.memoryTypeIndex = get_mem_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory( device, &mem_alloc_info, nullptr, &scene_img_mem );
	vkBindImageMemory( device, scene_img, scene_img_mem, 0 );

	VkImageViewCreateInfo view_ci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	view_ci.flags = 0;
	view_ci.image = scene_img;
	view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_ci.format = img_ci.format;
	view_ci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_ci.subresourceRange.baseMipLevel = 0;
	view_ci.subresourceRange.levelCount = img_ci.mipLevels;
	view_ci.subresourceRange.baseArrayLayer = 0;
	view_ci.subresourceRange.layerCount = 1;
	vkCreateImageView( device, &view_ci, nullptr, &scene_img_view );

	vector<uint8_t> buf(buffer, buffer + buf_size);

	scene_staging_buffer.init(buf_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	get_pixel_buffer.init(buf_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);

// Create a staging buffer
	if ( !CreateVulkanBuffer( device, mem_prop, pBuffer, nBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &m_pSceneStagingBuffer, &m_pSceneStagingBufferMemory ) )
	{
		return false;
	}

// Create a read pixel buffer
	if ( !CreateVulkanBuffer( m_pDevice, mem_prop, pBuffer, nBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, &getPixelBuffer, &getPixelBufferMemory ) )
	{
		return false;
	}
}
*/

void Swapchain::to_present(int i) {
	auto vk = Global::vk();
	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barier.srcAccessMask = 0;
	barier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barier.image = images[i];
	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barier.subresourceRange.baseMipLevel = 0;
	barier.subresourceRange.levelCount = 1;
	barier.subresourceRange.baseArrayLayer = 0;
	barier.subresourceRange.layerCount = 1;
	barier.srcQueueFamilyIndex = vk.graphics_queue;
	barier.dstQueueFamilyIndex = vk.graphics_queue;
	vkCmdPipelineBarrier( vk.cur_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
}

void Swapchain::get_image() {
	auto vk = Global::vk();
	check( vkAcquireNextImageKHR( vk.dev, swapchain, UINT64_MAX, semaphores[ frame_idx ], VK_NULL_HANDLE, &current_swapchain_image ), "vkAcquireNextImageKHR");


}

void VulkanSystem::init_vulkan() {
	init_instance();
	init_device();
	swapchain.init();
	
	// Create the command pool
	{
		VkCommandPoolCreateInfo cmdpoolci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		cmdpoolci.queueFamilyIndex = graphics_queue;
		cmdpoolci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		check( vkCreateCommandPool( dev, &cmdpoolci, nullptr, &cmd_pool ), "vkCreateCommandPool");
	}
}


VkCommandBuffer VulkanSystem::cmd_buffer() {
	for (auto &buf : cmd_buffers) {
		if (buf.finished()) {
			buf.reset();
			cur_cmd_buffer = buf.cmd_buffer;
			cur_fence = buf.fence;
			return cur_cmd_buffer;
		}
	}
	cmd_buffers.push_back(FencedCommandBuffer());
	cmd_buffers.back().init();
	cur_cmd_buffer = cmd_buffers.back().cmd_buffer;
	cur_fence = cmd_buffers.back().fence;

	return cur_cmd_buffer;
}

int get_mem_type( uint32_t mem_bits, VkMemoryPropertyFlags mem_prop )
{
  auto vk = Global::vk();
  for ( uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++ )
    {
      if ( ( mem_bits & 1 ) == 1)
  {
    // Type is available, does it match user properties?
    if ( ( vk.mem_prop.memoryTypes[i].propertyFlags & mem_prop ) == mem_prop )
      return i;
  }
      mem_bits >>= 1;
    }

  // No memory types matched, return failure
  throw "err";
}


void VulkanSystem::start_cmd_buffer() {
	// Start the command buffer
	VkCommandBufferBeginInfo cmd_buf_bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmd_buf_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer( cur_cmd_buffer, &cmd_buf_bi );
}

void VulkanSystem::end_cmd_buffer() {
	vkEndCommandBuffer( cur_cmd_buffer );
}

void VulkanSystem::submit_cmd_buffer() {

// Submit the command buffer
	VkPipelineStageFlags mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	si.commandBufferCount = 1;
	si.pCommandBuffers = &cur_cmd_buffer;
	si.waitSemaphoreCount = 1;
	si.pWaitSemaphores = &swapchain.semaphores[ swapchain.frame_idx ];
	si.pWaitDstStageMask = &mask;

	vkQueueSubmit( queue, 1, &si, cur_fence );

}
