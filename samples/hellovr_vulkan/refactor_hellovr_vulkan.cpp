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


struct FencedCommandBuffer
{
  VkCommandBuffer cmd_buffer;
  VkFence fence;
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

  
  uint32_t n_swap;
  uint32_t frame_idx;
  uint32_t current_frame;
  std::vector< VkImage > swapchain_img;
  std::vector< VkImageView > swapchain_view;
  std::vector< VkFramebuffer > swapchain_framebuffers;
  std::vector< VkSemaphore > swapchain_semaphores;
  VkRenderPass m_pSwapchainRenderPass;

  VkCommandPool cmd_pool;
  VkDescriptorPool desc_pool;
  VkDescriptorSet desc_sets[ NUM_DESCRIPTOR_SETS ];

  std::deque< FencedCommandBuffer > cmd_buffers;;
  
  
  VkSampler sampler;

  VkDebugReportCallbackEXT debug_callback;

  VulkanSystem() {
    init_instance();
    init_dev();
   
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

  void init_dev() {
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

    int graphics_queue(-1);
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
    dqci.queueFamilyIndex = m_nQueueFamilyIndex;
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
  
};


struct VertexBuffer {
};

struct IndexBuffer {
};

struct VRSystem {
  vr::IVRSystem *hmd;
  vr::IVRRenderModels *render_models;
  std::string driver_str, display_str;
  vr::TrackedDevicePose_t tracked_pose[ vr::k_unMaxTrackedDeviceCount ];
  Matrix4 device_pose[ vr::k_unMaxTrackedDeviceCount ];


  VRSystem() {
    vr::EVRInitError err = vr::VRInitError_None;
    hmd = vr::VR_Init( &err, vr::VRApplication_Scene );
    check(err);

    render_models = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &err );

    driver_str = query_str(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
    display_str = query_str(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

    cout << "driver: " << driver_str << " display: " << display_str << endl;
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
};

struct WindowSystem {
  SDL_Window *window;
  uint32_t width, height;

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


struct Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;

  Buffer(size_T size, VkBufferUsageFlags usage) {
    auto vk = Global::vk();
    // Create the vertex buffer and fill with data
    VkBufferCreateInfo bci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bci.size = size;
    bci.usage = usage;
    check( vkCreateBuffer( vk.dev, &bci, nullptr, &buffer ), "vkCreateBuffer");
    
    VkMemoryRequirements memreq = {};
    vkGetBufferMemoryRequirements( vk.deve, *buffer, &memreq );

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.memoryTypeIndex = get_mem_type( memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
    alloc_info.allocationSize = memreq.size;

    check( vkAllocateMemory( vk.dev, &alloc_info, nullptr, &memory ), "vkCreateBuffer" );
    
    check( vkBindBufferMemory( vk.dev, &buffer, &memory, 0 ), "vkBindBufferMemory" );

    /*
    if ( pBufferData != nullptr )
      {
		void *pData;
		nResult = vkMapMemory( pDevice, *ppDeviceMemoryOut, 0, VK_WHOLE_SIZE, 0, &pData );
		if ( nResult != VK_SUCCESS )
		{
			dprintf( "%s - vkMapMemory returned error %d\n", __FUNCTION__, nResult );
			return false;
		}
		memcpy( pData, pBufferData, nSize );
		vkUnmapMemory( pDevice, *ppDeviceMemoryOut );

		VkMappedMemoryRange memoryRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		memoryRange.memory = *ppDeviceMemoryOut;
		memoryRange.size = VK_WHOLE_SIZE;
		vkFlushMappedMemoryRanges( pDevice, 1, &memoryRange );

	}
	return true;
	}*/
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

  Image() {
    auto vk = Global::vk();
    
  }
};

struct FrameBuffer {
  Image image, depth_stencil;
  VkRenderPass render_pass;
  VkFramebuffer framebuffer;
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
