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


struct VulkanSystem {

};

struct VRSystem {

};

struct WindowSystem {

};

struct Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;

}

struct ViewedBuffer {
	Buffer buffer;
	VkBufferView buffer_view;
}

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