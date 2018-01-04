#include "flywheel.h"

using namespace std;

static Image* ImageFlywheel::image(std::string name) {
	if (!wheel.exists(name)) {
		string path("/home/marijnfs/img/");
		path += name;
		
		wheel[name] = new Image(path, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	}
}

