#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <iostream>
#include <fstream>
#include <vector>

#include "vulkansystem.h"
#include "vrsystem.h"
#include "windowsystem.h"
#include "scene.h"
#include "script.h"

struct Global {
	Global();

	static Global &inst() {
	  static Global *g = new Global();
	  return *g;
	}

  static VulkanSystem &vk() {
    //std::cout << "getting vk ptr" << std::endl;
      //std::cout << "initializing Vulkan System" << std::endl;
      if (!inst().vk_ptr) {
        inst().vk_ptr = new VulkanSystem();
        inst().vk_ptr->init();
      }
      
      
      return *(inst().vk_ptr);
	}

	static VRSystem &vr() {
      //std::cout << "getting vr ptr" << std::endl;
      if (!inst().vr_ptr) {
			inst().vr_ptr = new VRSystem();
			inst().vr_ptr->init();
		}

	  return *(inst().vr_ptr);
	}

	static WindowSystem &ws() {
      //std::cout << "initializing Window System" << std::endl;
		if (!inst().ws_ptr) {
		  inst().ws_ptr = new WindowSystem();
		  inst().ws_ptr->init();
		}

	  return *(inst().ws_ptr);
	}


	static Scene &scene() {
		if (!inst().scene_ptr) {
			inst().scene_ptr = new Scene();
        }

		return *(inst().scene_ptr);
	}

  static Script &script() {
    if (!inst().script_ptr) {
      inst().script_ptr = new Script();
    }
    
    return *(inst().script_ptr);
    
  }
  
	static void init() {
		inst();
	}

  static void shutdown() {
    vk().wait_idle(); //wait for VR to finish

    //Destroy stuff
    delete inst().scene_ptr;
    delete inst().vr_ptr;

    ImageFlywheel::destroy();
    ws().destroy_buffers();
    
    delete inst().vk_ptr;
    delete inst().ws_ptr;
  }
  
  VulkanSystem *vk_ptr = 0;
  VRSystem *vr_ptr = 0;
  WindowSystem *ws_ptr = 0;
  Scene *scene_ptr = 0;
  Script *script_ptr = 0;
};

template <typename T>
void write_vec(std::vector<T> &in, std::string filename) {
  std::ofstream outfile(filename.c_str(), std::ios::binary);
  uint64_t s = in.size();
  outfile.write((char*)&s, sizeof(s));
  outfile.write((char*)&in[0], sizeof(T) * in.size());
}
                                 
template <typename T>
std::vector<T> read_vec(std::string filename) {
  std::ifstream infile(filename.c_str(), std::ios::binary);
  uint64_t s(0);
  infile.read((char*)&s, sizeof(s));
  std::vector<T> v(s);
  
  infile.read((char*)&v[0], sizeof(T) * v.size());
  return v;
}

static int msaa = 4;
static int VIVE_HEIGHT = 1680;
static int VIVE_WIDTH = 1512;

#endif
