//========= Copyright Valve Corporation ============//

#if defined( _WIN32 )
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define SDL_VIDEO_DRIVER_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <inttypes.h>
#include <openvr.h>
#include <deque>
#include <iostream>
#include <fstream>

#include "shared/lodepng.h"
#include "shared/Matrices.h"
#include "shared/pathtools.h"

#include "util.h"


#if defined(POSIX)
#include "unistd.h"
#endif

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

void ThreadSleep( unsigned long nMilliseconds )
{
#if defined(_WIN32)
  ::Sleep( nMilliseconds );
#elif defined(POSIX)
  usleep( nMilliseconds * 1000 );
#endif
}

template <typename T>
void check(VkResult res, std::string str, T other = "") {
  if (res != VK_SUCCESS) {
    cerr << str << other << " error: " << res << endl;
    throw "err";
  }
}

void check(vr::EVRInitError err) {
  if ( eError != vr::VRInitError_None ) {
    cerr << "Unable to init vr: " << vr::VR_GetVRInitErrorAsEnglishDescription(err) << endl;
    throw "";
  }
}

void sdl_check(int err) {
  if (err < 0) {
    cerr << "SDL error: " << SDL_GetError() << endl;
    throw "";
  }
}

Matrix4 vrmat_to_mat4( const vr::HmdMatrix34_t &matPose )
{
  Matrix4 matrixObj(
    matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
    matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
    matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
    matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
    );
  return matrixObj;
}

struct FencedCommandBuffer
{
  VkCommandBuffer cmd_buffer;
  VkFence fence;

  void begin() {
	VkCommandBufferBeginInfo cmbbi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer( Global::vk().cmd_buffer, &cmbbi );
  }

  void end() {
	vkEndCommandBuffer( Global::vk().cmd_buffer );
  }

  bool finished() {
	return vkGetFenceStatus( Global::vk().device, m_commandBuffers.back().m_pFence ) == VK_SUCCESS;	
  }

  void reset() {
  	vkResetCommandBuffer( cmd_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
	vkResetFences( Global::vk().device, 1, &fence );
  }


};


struct Pos3Tex2
{
	Vector3 pos;
	Vector2 texpos;
};

struct Pos2Tex2
{
	Vector2 pos;
	Vector2 texpos;
};

enum PSO //shader files
{
	PSO_SCENE = 0,
	PSO_AXES,
	PSO_RENDERMODEL,
	PSO_COMPANION,
	PSO_COUNT
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

  VulkanSystem() {
    init_instance();
    init_dev();
		
  }

  void submit(FencedCommandBuffer fcb) {
  	VkSubmitInfo submiti = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submiti.commandBufferCount = 1;
	submiti.pCommandBuffers = &fcb.cmd_buffer;
	vkQueueSubmit( queue, 1, &submiti, fcb.fence );
  }

  void wait_queue() {
  	vkQueueWaitIdle( queue );
  }

  FencedCommandBuffer get_cmd_buffer() {
  	FencedCommandBuffer buf;
    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmd_buffer_alloc_info.commandBufferCount = 1;
	cmd_buffer_alloc_info.commandPool = cmd_pool;
	cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkAllocateCommandBuffers( device, &cmd_buffer_alloc_info, &buf.cmd_buffer );

	VkFenceCreateInfo fenceCci= { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	vkCreateFence( device, &fenceci, nullptr, &buf.fence );
	return buf;
  }

  void init_instance() {
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
      inst_req_charp[i] = inst_req[i].c_str();
		
    VkInstanceCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pNext = NULL;
    ici.pApplicationInfo = &appInfo;
    ici.enabledExtensionCount = inst_req.size();
    ici.ppEnabledExtensionNames = inst_req_charp;
    ici.enabledLayerCount = layer_count;
    ici.ppEnabledLayerNames = 0; //might need validation layers later

    check( vkCreateInstance( &ici, nullptr, &inst), "Create Instance");
  }

  void init_device() {
    uint32_t n_dev(0);
    check( vkEnumeratePhysicalDevices( instance, &n_dev, NULL ), "vkEnumeratePhysicalDevices");
    vector<VkPhysicalDevice> devices(n_dev);
    check( vkEnumeratePhysicalDevices( instance, &n_dev, &devices[0] ), "vkEnumeratePhysicalDevices");
	
    VkPhysicalDevice chosen_dev = devices[0]; //select first, could be altered

    vkGetPhysicalDeviceProperties( chosen_dev, &prop);
    vkGetPhysicalDeviceMemoryProperties( chosen_dev, &mem_prop );
    vkGetPhysicalDeviceFeatures( chosen_dev, &features );

    std::vector< std::string > requiredDeviceExtensions;
    GetVulkanDeviceExtensionsRequired( chosen_dev, requiredDeviceExtensions );

    auto dev_ext = get_dev_ext_required();
    // Add additional required extensions
    dev_ext.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    

    int n_queue(0);
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

    auto dev_ext = get_dev_ext_required_verified();
    vector<char *> pp_dev_ext(dev_ext.size());
    for (int i(0); i < dev_ext.size(); ++i)
      pp_dev_ext[i] = dev_ext[i].c_str();
    
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

    check( vkCreateDevice( chosen_dev, &dci, nullptr, &device ), "vkCreateDevice");

    vkGetDeviceQueue( device, graphics_queue, 0, &queue );
  }

  void init_descriptor_sets() {
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
	vkCreateDescriptorPool( device, &descpool_ci, nullptr, &desc_pool );

	for ( int i = 0; i < NUM_DESCRIPTOR_SETS; i++ )
	{
		VkDescriptorSetAllocateInfo desc_inf = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		desc_inf.descriptorPool = m_pDescriptorPool;
		desc_inf.descriptorSetCount = 1;
		desc_inf.pSetLayouts = &m_pDescriptorSetLayout;
		vkAllocateDescriptorSets( device, &desc_inf, &m_pDescriptorSets[ i ] );
	}

	for ( uint32_t eye = 0; eye < 2; eye++ )
	{
		VkDescriptorBufferInfo buf_inf = {};
		buf_inf.buffer = scene_constant_buffer[ eye ].buffer;
		buf_inf.offset = 0;
		buf_inf.range = VK_WHOLE_SIZE;
		
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageView = scene_img_view;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		VkDescriptorImageInfo sample_info = {};
		sample_info.sampler = scene_sampler;

		VkWriteDescriptorSet write_desc_set[ 3 ] = { };
		write_desc_set[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc_set[ 0 ].dstSet = desc_sets[ DESCRIPTOR_SET_LEFT_EYE_SCENE + eye ];
		write_desc_set[ 0 ].dstBinding = 0;
		write_desc_set[ 0 ].descriptorCount = 1;
		write_desc_set[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_desc_set[ 0 ].pBufferInfo = &buf_inf;
		write_desc_set[ 1 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc_set[ 1 ].dstSet = desc_sets[ DESCRIPTOR_SET_LEFT_EYE_SCENE + eye ];
		write_desc_set[ 1 ].dstBinding = 1;
		write_desc_set[ 1 ].descriptorCount = 1;
		write_desc_set[ 1 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write_desc_set[ 1 ].pImageInfo = &imageInfo;
		write_desc_set[ 2 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc_set[ 2 ].dstSet = desc_sets[ DESCRIPTOR_SET_LEFT_EYE_SCENE + eye ];
		write_desc_set[ 2 ].dstBinding = 2;
		write_desc_set[ 2 ].descriptorCount = 1;
		write_desc_set[ 2 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write_desc_set[ 2 ].pImageInfo = &sample_info;
		
		vkUpdateDescriptorSets( device, _countof( write_desc_set ), write_desc_set, 0, nullptr );
	}

	// Companion window descriptor sets
	{
		VkDescriptorImageInfo img_i = {};
		img_i.imageView = left_eye_fb.view;
		img_i.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write_desc[ 1 ] = { };
		write_desc[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc[ 0 ].dstSet = desc_sets[ DESCRIPTOR_SET_COMPANION_LEFT_TEXTURE ];
		write_desc[ 0 ].dstBinding = 1;
		write_desc[ 0 ].descriptorCount = 1;
		write_desc[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write_desc[ 0 ].pImageInfo = &img_i;
		vkUpdateDescriptorSets( device, _countof( write_desc ), write_desc, 0, nullptr );

		img_i.imageView = right_eye_fb.view;
		write_desc[ 0 ].dstSet = desc_sets[ DESCRIPTOR_SET_COMPANION_RIGHT_TEXTURE ];
		vkUpdateDescriptorSets( device, _countof( write_desc ), write_desc, 0, nullptr );
	}
  }

  void init_swapchain() {
    auto window = Global::window();
	
    SDL_SysWMinfo wm_info;
    SDL_VERSION( &wm_info.version );
    SDL_GetWindowWMInfo( window.window, &wm_info );
    VkResult nResult;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.pNext = NULL;
    win32SurfaceCreateInfo.flags = 0;
    win32SurfaceCreateInfo.hinstance = GetModuleHandle( NULL );
    win32SurfaceCreateInfo.hwnd = ( HWND ) wm_info.info.win.window;
    check( vkCreateWin32SurfaceKHR( instance, &win32SurfaceCreateInfo, nullptr, &surface ), "vkCreateWin32SurfaceKHR");
#else
    VkXlibSurfaceCreateInfoKHR xlibSurfaceCreateInfo = { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
    xlibSurfaceCreateInfo.flags = 0;
    xlibSurfaceCreateInfo.dpy = wm_info.info.x11.display;
    xlibSurfaceCreateInfo.window = wm_info.info.x11.window;
    check( vkCreateXlibSurfaceKHR( instance, &xlibSurfaceCreateInfo, nullptr, &surface ), "vkCreateXlibSurfaceKHR" );
#endif

    VkBool32 supports_present = VK_FALSE;
    check( vkGetPhysicalDeviceSurfaceSupportKHR( phys_dev, graphics_queue, surface, &supports_present), "vkGetPhysicalDeviceSurfaceSupportKHR");
    if (supports_present == VK_FALSE) {
      cerr << "support not present, vkGetPhysicalDeviceSurfaceSupportKHR" << endl;
      throw "";
    }

    //query supported formats
    VkFormat swap_format;
    uint32_t format_index(0);
    uint32_t n_swap_format(0);
    VkColorSpaceKHR color_space;
	
    check( vkGetPhysicalDeviceSurfaceFormatsKHR( phys_dev, surface, &n_swap_format, NULL), "vkGetPhysicalDeviceSurfaceFormatsKHR");
    vector<VkSurfaceFormatKHR> swap_formats(n_swap_format);
    check( vkGetPhysicalDeviceSurfaceFormatsKHR( phys_dev, surface, &n_swap_format, &swap_formats[0]), "vkGetPhysicalDeviceSurfaceFormatsKHR");

    for (int i(0); i < n_swap_format; ++i) {
      if (swap_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || swap_formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
	format_index = i;
	break;
      }
    }
    swap_formats = swap_formats[format_index].format;
    color_space = swap_formats[format_index].colorSpace;


    //check capabilities
    VkSurfaceCapabilitiesKHR surface_caps = {};
    check( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( phys_dev, surface, &surface_caps ), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	
    int n_present_modes(0);
    check( vkGetPhysicalDeviceSurfacePresentModesKHR( phys_dev, surface, &n_present_modes, NULL ), "vkGetPhysicalDeviceSurfacePresentModesKHR");
    vector<VkPresentModeKHR> present_modes(n_present_modes);
    check( vkGetPhysicalDeviceSurfacePresentModesKHR( phys_dev, surface, &n_present_modes, &present_modes[0] ), "vkGetPhysicalDeviceSurfacePresentModesKHR");

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
    else if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
      scci.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    else
      cerr << "Unexpected value for VkSurfaceCapabilitiesKHR.compositeAlpha:" << surfaceCaps.supportedCompositeAlpha << endl;

    check( vkCreateSwapchainKHR( device, &scci, NULL, &swapchain), "vkCreateSwapchainKHR");

    check( vkGetSwapchainImagesKHR(device, swapchain, &n_swap, NULL) );
    swapchain_img.resize(n_swap);
    check( vkGetSwapchainImagesKHR(device, swapchain, &n_swap, &swapchain_img[0]) );


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

    check( vkCreateRenderPass( device, &renderpassci, NULL, &swapchain_renderpass), "vkCreateRenderPass");

    for (int i(0); i < swapchain_img.size(); ++i) {
      VkImageViewCreateInfo viewci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
      viewci.flags = 0;
      viewci.image = swapchain_img[ i ];
      viewci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewci.format = swap_format;
      viewci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
      viewci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewci.subresourceRange.baseMipLevel = 0;
      viewci.subresourceRange.levelCount = 1;
      viewci.subresourceRange.baseArrayLayer = 0;
      viewci.subresourceRange.layerCount = 1;
      VkImageView image_view = VK_NULL_HANDLE;
      vkCreateImageView( device, &viewci, nullptr, &image_view );
      m_pSwapchainImageViews.push_back( image_view );
		
      VkImageView attachments[ 1 ] = { image_view };
      VkFramebufferCreateInfo fbci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
      fbci.renderPass = swapchain_renderpass;
      fbci.attachmentCount = 1;
      fbci.pAttachments = &attachments[ 0 ];
      fbci.width = window.width;
      fbci.height = window.height;
      fbci.layers = 1;
      VkFramebuffer framebuffer;
      check( vkCreateFramebuffer( device, &fbci, NULL, &framebuffer ), "vkCreateFramebuffer");
      swapchain_framebuffers.push_back( framebuffer );
		
      VkSemaphoreCreateInfo semci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
      VkSemaphore semaphore = VK_NULL_HANDLE;
      vkCreateSemaphore( device, &semci, nullptr, &semaphore );
      swapchain_semaphores.push_back( semaphore );
    }
  }

  void init_shaders() {
  	//Create Shaders, probably most involved part
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
			nResult = vkCreateShaderModule( device, &shader_ci, nullptr, (stage == "vs") ? &shader_modules_vs[i] : &shader_modules_ps[i] );
  		}
  		++i;
	}


	//Create Descriptor Layout
	VkDescriptorSetLayoutBinding layout_bind[3] = {};
	layout_bind[0].binding = 0;
	layout_bind[0].descriptorCount = 1;
	layout_bind[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_bind[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	layout_bind[1].binding = 1;
	layout_bind[1].descriptorCount = 1;
	layout_bind[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	layout_bind[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	layout_bind[2].binding = 2;
	layout_bind[2].descriptorCount = 1;
	layout_bind[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	layout_bind[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo desc_layout_ci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	desc_layout_ci.bindingCount = 3;
	desc_layout_ci.pBindings = &layout_bind[ 0 ];
	check( vkCreateDescriptorSetLayout( device(), &desc_layout_ci, nullptr, &desc_layout ), "vkCreateDescriptorSetLayout");


	//Create pipelines, first layout
	VkPipelineLayoutCreateInfo pipeline_ci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_ci.pNext = NULL;
	pipeline_ci.setLayoutCount = 1;
	pipeline_ci.pSetLayouts = &desc_layout_ci;
	pipeline_ci.pushConstantRangeCount = 0;
	pipeline_ci.pPushConstantRanges = NULL;
	check( vkCreatePipelineLayout( device, &pipeline_ci, nullptr, &pipeline_layout ), "vkCreatePipelineLayout");

	// Create pipeline cache
	VkPipelineCacheCreateInfo pipeline_cache_ci = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	vkCreatePipelineCache( device, &pipeline_cache_ci, NULL, &pipeline_cache );

	VkRenderPass render_passes[ PSO_COUNT ] =
	{
		left_eye_fb.render_pass,
		left_eye_fb.render_pass,
		left_eye_fb.render_pass,
		swapchain_renderpass
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
		multisample_ci.rasterizationSamples = ( pso == PSO_COMPANION ) ? VK_SAMPLE_COUNT_1_BIT : ( VkSampleCountFlagBits ) msaa_sample_count;
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
		pipeline_ci.renderPass = pRenderPasses[ pso ];

		static VkDynamicState dynamic_states[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		static VkPipelineDynamicStateCreateInfo dynamic_state_ci = {};
		dynamic_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_ci.pNext = NULL;
		dynamic_state_ci.dynamicStateCount = _countof( dynamic_states );
		dynamic_state_ci.pdynamic_states = &dynamic_states[ 0 ];
		pipeline_ci.pDynamicState = &dynamic_state_ci;


		// Create the pipeline
		check( vkCreateGraphicsPipelines( device, pipeline_cache, 1, &pipeline_ci, NULL, &pipelines[ pso ] ), "vkCreateGraphicsPipelines");
	}
  }

  void init_texture_maps() {
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
	MemoryTypeFromProperties( mem_prop, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc_info.memoryTypeIndex );
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

  void swapchain_to_present(int i) {
  	VkImageMemoryBarrier barier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
  	barier.srcAccessMask = 0;
  	barier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  	barier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  	barier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  	barier.image = swapchain_img[i];
  	barier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  	barier.subresourceRange.baseMipLevel = 0;
  	barier.subresourceRange.levelCount = 1;
  	barier.subresourceRange.baseArrayLayer = 0;
  	barier.subresourceRange.layerCount = 1;
  	barier.srcQueueFamilyIndex = graphics_queue;
  	barier.dstQueueFamilyIndex = graphics_queue;
  	vkCmdPipelineBarrier( cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barier );
  }

  void init_vulkan() {
  	init_instance();
  	init_device();
  	init_swapchain();

  	// Create the command pool
	{
		VkCommandPoolCreateInfo cmdpoolci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		cmdpoolci.queueFamilyIndex = graphics_queue;
		cmdpoolci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		check( vkCreateCommandPool( device, &cmdpoolci, nullptr, &cmd_pool ), "vkCreateCommandPool");
	}
  }
};

struct RenderModel {
	Buffer mat_buffer;
	Buffer vertex_buf, index_buf;

	VkDescriptorSet desc_set;

	void init() {
		int vert_count(0);
		int idx_count(0);

		vertex_buf.init(sizeof( vr::RenderModel_Vertex_t ) * vert_count);
		index_buf.init(sizeof(uint16_t) * idx_count);

	}
};

struct VRSystem {
  vr::IVRSystem *hmd;
  vr::IVRRenderModels *render_models;
  std::string driver_str, display_str;

  vr::TrackedDevicePose_t tracked_pose[ vr::k_unMaxTrackedDeviceCount ];
  Matrix4 tracked_pose_mat4[ vr::k_unMaxTrackedDeviceCount ];
  TrackedDeviceClass device_class[ vr::k_unMaxTrackedDeviceCount ];

  Matrix4 hmd_pose;
  Matrix4 eye_pos_left, eye_pos_right, eye_pose_center;
  Matrix4 projection_left, projection_right;

  FrameRenderBuffer left_eye_fb, right_eye_fb;

  int render_width, render_height;
  float near_clip, far_clip;

  VRSystem() {
  	render_width = 0;
  	render_height = 0;

  	near_clip = 0.1f;
  	far_clip = 30.0f;

    vr::EVRInitError err = vr::VRInitError_None;
    hmd = vr::VR_Init( &err, vr::VRApplication_Scene );
    check(err);

    render_models = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &err );

    driver_str = query_str(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
    display_str = query_str(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

    cout << "driver: " << driver_str << " display: " << display_str << endl;

  	if ( !vr::VRCompositor() ) {
  		cerr << "Couldn't create VRCompositor" << endl;
  		throw "";
  	}
  }

  void render_stereo_targets() {
  	VkViewport viewport = { 0.0f, 0.0f, (float ) render_width, ( float ) render_height, 0.0f, 1.0f };
  	vkCmdSetViewport( cmd_buffer, 0, 1, &viewport );
  	VkRect2D scissor = { 0, 0, render_width, render_height};
  	vkCmdSetScissor( cmd_buffer, 0, 1, &scissor );

  	left_eye_fb.to_colour_optimal();
  	if (left_eye_fb.depth_stencil.layout == VK_IMAGE_LAYOUT_UNDEFINED)
  	 	left_eye_fb.to_depth_optimal();
  	left_eye_fb.start_render_pass();
  	//render stuff
  	left_eye_fb.end_render_pass();
  	left_eye_fb.to_read_optimal();


  	right_eye_fb.to_colour_optimal();
  	if (right_eye_fb.depth_stencil.layout == VK_IMAGE_LAYOUT_UNDEFINED)
  		right_eye_fb.to_depth_optimal();
  	right_eye_fb.start_render_pass();
  	//render stuff
  	right_eye_fb.end_render_pass();
  	right_eye_fb.to_read_optimal();

  }

  Matrix4 get_eye_transform( vr::Hmd_Eye eye )
  {
    vr::HmdMatrix34_t mat_eye = m_pHMD->GetEyeToHeadTransform( eye );
    Matrix4 mat(
      mat_eye.m[0][0], mat_eye.m[1][0], mat_eye.m[2][0], 0.0, 
      mat_eye.m[0][1], mat_eye.m[1][1], mat_eye.m[2][1], 0.0,
      mat_eye.m[0][2], mat_eye.m[1][2], mat_eye.m[2][2], 0.0,
      mat_eye.m[0][3], mat_eye.m[1][3], mat_eye.m[2][3], 1.0f
    );

    return mat.invert();
  }

  Matrix4 get_hmd_projection( vr::Hmd_Eye eye )
  {
    vr::HmdMatrix44_t mat = hmd->GetProjectionMatrix( eye, near_clip, far_clip );

    return Matrix4(
      mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
      mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], 
      mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2], 
      mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
    );
  }


  Matrix4 get_view_projection( vr::Hmd_Eye eye ) {
    if( eye == vr::Eye_Left )
      return projection_left * eye_pos_left * hmd_pose;
    else if( eye == vr::Eye_Right )
      return projection_right * eye_pos_right * hmd_pose;
    throw StringException("not valid eye");
  }

  void render_scene() {

  }

  void setup_render_models()
  {
    for( uint32_t d = vr::k_unTrackedDeviceIndex_Hmd + 1; d < vr::k_unMaxTrackedDeviceCount; d++ )
    {
      if( !hmd->IsTrackedDeviceConnected( d ) )
        continue;

      SetupRenderModelForTrackedDevice( d );
    }

  }

  void setup_render_model_for_device(int d) {

  }

  void update_track_pose() {
    vr::VRCompositor()->WaitGetPoses(tracked_pose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );

    for ( int d = 0; d < vr::k_unMaxTrackedDeviceCount; ++d )
    {
      if ( tracked_pose[d].bPoseIsValid )
      {
        tracked_pose_mat4[d] = ConvertSteamVRMatrixToMatrix4( tracked_pose[d].mDeviceToAbsoluteTracking );
        device_class[d] = hmd->GetTrackedDeviceClass(d);
      }
    }

    if ( tracked_pose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
    {
      hmd_pose = tracked_pose_mat4[vr::k_unTrackedDeviceIndex_Hmd];
      hmd_pose.invert();
    }
  }

  std::string query_str(vr::TrackedDeviceIndex_t devidx, vr::TrackedDeviceProperty prop) {
    vr::TrackedPropertyError *err = NULL;
    uint32_t buflen = hmd->GetStringTrackedDeviceProperty( devidx, prop, NULL, 0, err );
    if( buflen == 0)
      return "";

    std::string buf(' ', buflen);
    buflen = hmd->GetStringTrackedDeviceProperty( devidx, prop, buf.c_str(), buflen, err );
    return buf;      
  }

  vector<string> get_inst_ext_required() {
    uint32_t buf_size = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( nullptr, 0 );
    if (!buf_size)
      return;

    std::string buf(' ', buf_size);
    vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( buf.c_str(), buf_size );

    // Break up the space separated list into entries on the CUtlStringList
    vector<string> ext_list;
    std::string cur_ext;
    uint32_t idx = 0;
    while ( idx < buf_size ) {
      if ( buf[ idx ] == ' ' ) {
	ext_list.push_back( cur_ext );
	cur_ext.clear();
      } else {
	cur_ext += buf[ idx ];
      }
      ++idx;
    }
    if ( cur_ext.size() > 0 ) {
      ext_list.push_back( cur_ext );
    }

    return ext_list;
  }

  vector<string> get_dev_ext_required() {
    uint32_t buf_size = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( nullptr, 0 );
    if (!buf_size)
      return;

    std::string buf(' ', buf_size);
    vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( buf.c_str(), buf_size );

    // Break up the space separated list into entries on the CUtlStringList
    vector<string> ext_list;
    std::string cur_ext;
    uint32_t idx = 0;
    while ( idx < buf_size ) {
      if ( buf[ idx ] == ' ' ) {
	ext_list.push_back( cur_ext );
	cur_ext.clear();
      } else {
	cur_ext += buf[ idx ];
      }
      ++idx;
    }
    if ( cur_ext.size() > 0 ) {
      ext_list.push_back( cur_ext );
    }

    return ext_list;
  }

  vector<string> get_inst_ext_required_verified() {
    auto instance_ext_req = get_inst_ext_required();
    instance_ext_req.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

#if defined ( _WIN32 )
    instance_ext_req.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#else
    instance_ext_req.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#endif


    uint32_t n_instance_ext(0);
    check( vkEnumerateInstanceExtensionProperties( NULL, &n_instance_ext, NULL ), "vkEnumerateInstanceExtensionProperties");

    vector<VkExtensionProperties> ext_prop(n_instance_ext);

    check( vkEnumerateInstanceExtensionProperties( NULL, &n_instance_ext, &ext_prop[0]), "vkEnumerateInstanceExtensionProperties" );

    for (auto req_inst : instance_ext_req) {
      bool found(false);
      for (auto prop : ext_prop)
	if (req_inst == string(prop.extensionName))
	  found = true;
      if (!found) {
	cerr << "couldn't find extension" << endl;
	throw "";
      }
    }
			
    return instance_ext_req;
  }

  vector<string> get_dev_ext_required_verified() {
    auto dev_ext_req = get_dev_ext_required();
    dev_ext_req.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

#if defined ( _WIN32 )
    dev_ext_req.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#else
    dev_ext_req.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#endif


    uint32_t n_dev_ext(0);
    check( vkEnumerateDeviceExtensionProperties( NULL, &n_dev_ext, NULL ), "vkEnumerateDeviceExtensionProperties");

    vector<VkExtensionProperties> ext_prop(n_dev_ext);

    check( vkEnumerateDeviceExtensionProperties( NULL, &n_dev_ext, &ext_prop[0]), "vkEnumerateDeviceExtensionProperties" );

    for (auto req_dev : dev_ext_req) {
      bool found(false);
      for (auto prop : ext_prop)
	if (req_dev == string(prop.extensionName))
	  found = true;
      if (!found) {
	cerr << "couldn't find extension" << endl;
	throw "";
      }
    }
				
    return dev_ext_req;
  }

  ~VRSystem() {
    vr::VR_Shutdown();
  }

  void setup_render_targets() {
  	hmd->GetRecommendedRenderTargetSize( &render_width, &render_height );

	CreateFrameBuffer( render_width, render_height, left_eye_fb );
	CreateFrameBuffer( render_width, render_height, right_eye_fb );
  }
};

struct GraphicsObject {
	vector<float> v;


	void draw() {
		//TODO fix
		vkCmdBindPipeline( m_currentCommandBuffer.m_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelines[ PSO_SCENE ] );
		
		// Update the persistently mapped pointer to the CB data with the latest matrix
		memcpy( m_pSceneConstantBufferData[ nEye ], GetCurrentViewProjectionMatrix( nEye ).get(), sizeof( Matrix4 ) );

		vkCmdBindDescriptorSets( m_currentCommandBuffer.m_pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &desc_sets[ DESCRIPTOR_SET_LEFT_EYE_SCENE + nEye ], 0, nullptr );

		// Draw
		VkDeviceSize nOffsets[ 1 ] = { 0 };
		vkCmdBindVertexBuffers( m_currentCommandBuffer.m_pCommandBuffer, 0, 1, &m_pSceneVertexBuffer, &nOffsets[ 0 ] );
		vkCmdDraw( m_currentCommandBuffer.m_pCommandBuffer, m_uiVertcount, 1, 0, 0 );
	}

	void init_cube(Matrix4 pos) {
		Vector4 A = pos * Vector4( 0, 0, 0, 1 );
		Vector4 B = pos * Vector4( 1, 0, 0, 1 );
		Vector4 C = pos * Vector4( 1, 1, 0, 1 );
		Vector4 D = pos * Vector4( 0, 1, 0, 1 );
		Vector4 E = pos * Vector4( 0, 0, 1, 1 );
		Vector4 F = pos * Vector4( 1, 0, 1, 1 );
		Vector4 G = pos * Vector4( 1, 1, 1, 1 );
		Vector4 H = pos * Vector4( 0, 1, 1, 1 );

		// triangles instead of quads
		add_vertex( E.x, E.y, E.z, 0, 1, ); //Front
		add_vertex( F.x, F.y, F.z, 1, 1, );
		add_vertex( G.x, G.y, G.z, 1, 0, );
		add_vertex( G.x, G.y, G.z, 1, 0, );
		add_vertex( H.x, H.y, H.z, 0, 0, );
		add_vertex( E.x, E.y, E.z, 0, 1, );
						 
		add_vertex( B.x, B.y, B.z, 0, 1, ); //Back
		add_vertex( A.x, A.y, A.z, 1, 1, );
		add_vertex( D.x, D.y, D.z, 1, 0, );
		add_vertex( D.x, D.y, D.z, 1, 0, );
		add_vertex( C.x, C.y, C.z, 0, 0, );
		add_vertex( B.x, B.y, B.z, 0, 1, );
						
		add_vertex( H.x, H.y, H.z, 0, 1, ); //Top
		add_vertex( G.x, G.y, G.z, 1, 1, );
		add_vertex( C.x, C.y, C.z, 1, 0, );
		add_vertex( C.x, C.y, C.z, 1, 0, );
		add_vertex( D.x, D.y, D.z, 0, 0, );
		add_vertex( H.x, H.y, H.z, 0, 1, );
					
		add_vertex( A.x, A.y, A.z, 0, 1, ); //Bottom
		add_vertex( B.x, B.y, B.z, 1, 1, );
		add_vertex( F.x, F.y, F.z, 1, 0, );
		add_vertex( F.x, F.y, F.z, 1, 0, );
		add_vertex( E.x, E.y, E.z, 0, 0, );
		add_vertex( A.x, A.y, A.z, 0, 1, );
						
		add_vertex( A.x, A.y, A.z, 0, 1, ); //Left
		add_vertex( E.x, E.y, E.z, 1, 1, );
		add_vertex( H.x, H.y, H.z, 1, 0, );
		add_vertex( H.x, H.y, H.z, 1, 0, );
		add_vertex( D.x, D.y, D.z, 0, 0, );
		add_vertex( A.x, A.y, A.z, 0, 1, );

		add_vertex( F.x, F.y, F.z, 0, 1, ); //Right
		add_vertex( B.x, B.y, B.z, 1, 1, );
		add_vertex( C.x, C.y, C.z, 1, 0, );
		add_vertex( C.x, C.y, C.z, 1, 0, );
		add_vertex( G.x, G.y, G.z, 0, 0, );
		add_vertex( F.x, F.y, F.z, 0, 1, );
	}

	void add_vertex(t fl0, float fl1, float fl2, float fl3, float fl4) {
		v.push_back( fl0 );
		v.push_back( fl1 );
		v.push_back( fl2 );
		v.push_back( fl3 );
		v.push_back( fl4 );
	}

};

struct WindowSystem {
  SDL_Window *window;
  uint32_t width, height;
  Buffer vertex_buf, index_buf;

  WindowSystem() : width(800), height(800) {
    sdl_check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER));

    int pox_x = 700;
    int pos_y = 100;
    Uint32 wflags = SDL_WINDOW_SHOWN;

    window = SDL_CreateWindow( "hellovr [Vulkan]", pos_x, pos_y, width, height, wflags );
    if (!window) {
      cerr << "SDL Window problem: " << SDL_GetError() << endl;
      throw "";
    }
  }

  void setup_window() {
  	vector<Pos2Tex2> verts;

  	//left eye verts
  	verts.push_back( Pos2Tex2( Vector2(-1, -1), Vector2(0, 1)) );
	verts.push_back( Pos2Tex2( Vector2(0, -1), Vector2(1, 1)) );
	verts.push_back( Pos2Tex2( Vector2(-1, 1), Vector2(0, 0)) );
	verts.push_back( Pos2Tex2( Vector2(0, 1), Vector2(1, 0)) );

	// right eye verts
	verts.push_back( Pos2Tex2( Vector2(0, -1), Vector2(0, 1)) );
	verts.push_back( Pos2Tex2( Vector2(1, -1), Vector2(1, 1)) );
	verts.push_back( Pos2Tex2( Vector2(0, 1), Vector2(0, 0)) );
	verts.push_back( Pos2Tex2( Vector2(1, 1), Vector2(1, 0)) );

	uint16_t indices[] = { 0, 1, 3,   0, 3, 2,   4, 5, 7,   4, 7, 6};


	//aTODO: add initialisation
	vertex_buf.init(sizeof(Pos2Tex2) * verts.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	index_buf.init(sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	for (int i(0); i < swapchain_img.size(); ++i)	
		swapchain_to_present(i);
  }

};


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

enum Location {
	HOST,
	DEVICE,
	HOST_COHERENT
};

struct Buffer {
  VkBuffer buffer;
  VkDeviceMemory memory;

  Buffer() {}

  Buffer(size_T size, VkBufferUsageFlags usage) {
  	init(size, usage);
  }

  template <typename T>
  void init(size_T size, VkBufferUsageFlags usage, Location loc, std::vector<T> &init_data) {
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

  void init(size_T size, VkBufferUsageFlags usage, Location loc) {
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

  struct ViewedBuffer {
    Buffer buffer;
    VkBufferView buffer_view;
  };

  struct Image {
    VkImage img;
    VkDeviceMemory mem;
    VkImageLayout layout;
    VkImageView view;

    Image() {}

    Image(int width, int height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
    	init(width, height, format, usage);
	}

    void init(int width, int height, VkFormat format, VkImageUsageFlags usage, Vk) {
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
  };

  struct FrameRenderBuffer {
    Image image, depth_stencil;
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
    int width, height;

    void init(int width_, int height_) {
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

    void to_colour_optimal() {
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


	void to_depth_optimal() {
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

	void to_read_optimal() {
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


	void start_render_pass() {
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

	void end_render_pass() {
		vkCmdEndRenderPass( Global::vk().cmd_buffer );
	}
  };

  struct Global {
    Global() {
      vk_ptr = new VulkanSystem();
      vr_ptr = new VRSystem();
      ws_ptr = new WindowSystem();
    }

    static Global &inst() {
      static Global *g = new Global();
      return g;
    }

    static VulkanSystem &vk() {
      return *(inst().vk_ptr);
    }

    static VRSystem &vr() {
      return *(inst().vr_ptr);
    }

    static WindowSystem &ws() {
      return *(inst().ws_ptr);
    }

    VulkanSystem *vk_ptr;
    VRSystem *vr_ptr;
    WindowSystem *ws_ptr;
  };

  int main() {
    Global::inst();

				
  }
