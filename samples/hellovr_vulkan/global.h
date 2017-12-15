#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "vulkansystem.h"
#include "vrsystem.h"
#include "windowsystem.h"


struct Global {
	Global();

	static Global &inst() {
	  static Global *g = new Global();
	  return *g;
	}

	static VulkanSystem &vk() {
		std::cout << "initializing Vulkan System" << std::endl;
		if (!inst().vk_ptr) {
		  inst().vk_ptr = new VulkanSystem();
		  inst().vk_ptr->init();
		}


		return *(inst().vk_ptr);
	}

	static VRSystem &vr() {
		std::cout << "initializing VR System" << std::endl;
		if (!inst().vr_ptr) {
			inst().vr_ptr = new VRSystem();
			inst().vr_ptr->init();
		}

	  return *(inst().vr_ptr);
	}

	static WindowSystem &ws() {
		std::cout << "initializing Window System" << std::endl;
		if (!inst().ws_ptr) {
		  inst().ws_ptr = new WindowSystem();
		  inst().ws_ptr->init();
		}

	  return *(inst().ws_ptr);
	}

	static void init() {
		inst();
	}

	VulkanSystem *vk_ptr;
	VRSystem *vr_ptr;
	WindowSystem *ws_ptr;
};

#endif
