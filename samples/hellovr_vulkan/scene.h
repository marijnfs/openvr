#ifndef __SCENE_H__
#define __SCENE_H__

#include <map>
#include <string> 
#include <vector>

#include "vrsystem.h"
#include "vulkansystem.h"

// place to create buffers for scene and store objects
// instead of bloating the vulkan system class

//Scene Object


struct Scene {
	std::map<std::string, GraphicsObject*> objects;

	void init();

	void add_plane(std::string name);

	void set_pos(std::string, std::vector<float> pos);

	

};

#endif