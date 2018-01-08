#ifndef __FLYWHEEL__
#define __FLYWHEEL__

#include <string>
#include <vector>


#include "buffer.h"


struct ImageFlywheel {

	ImageFlywheel();

	static std::map<std::string, Image*> wheel;

	static Image* image(std::string name);
};

#endif
