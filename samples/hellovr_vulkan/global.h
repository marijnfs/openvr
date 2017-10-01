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
	  return *(inst().vk_ptr);
	}

	static VRSystem &vr() {
	  return *(inst().vr_ptr);
	}

	static WindowSystem &ws() {
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
