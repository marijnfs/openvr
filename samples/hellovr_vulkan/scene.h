#ifndef __SCENE_H__
#define __SCENE_H__

#include "vrsystem.h"
#include "vulkansystem.h"

// place to create buffers for scene and store objects
// instead of bloating the vulkan system class

//Scene Object


struct Scene {
	std::vector<GraphicsObject*> objects;


};

#endif