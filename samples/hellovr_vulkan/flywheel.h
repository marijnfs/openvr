#ifndef __FLYWHEEL__
#define __FLYWHEEL__

#include <string>
#include <vector>
#include <map>

#include "buffer.h"


struct ImageFlywheel {

  static std::map<std::string, Image*> wheel;
  
  ImageFlywheel();


  static Image* image(std::string name) {
    if (!ImageFlywheel::wheel.count(name)) {
      std::string path("/home/marijnfs/img/");
      path += name;
      
      ImageFlywheel::wheel[name] = new Image(path, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    }
  }

};

#endif