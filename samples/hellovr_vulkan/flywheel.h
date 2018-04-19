#ifndef __FLYWHEEL__
#define __FLYWHEEL__

#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "buffer.h"


struct ImageFlywheel {
  static std::map<std::string, Image*> wheel;
  
  ImageFlywheel();


  static void preload() {
    std::cout << "Preloading images" << std::endl;
    for (auto f : walk("/home/marijnfs/img", ".png")) {
      cout << f << endl;
      int pos = f.rfind("/");
      auto name = f.substr(pos+1);
      ImageFlywheel::image(name);
    }
  }
  
  static Image* image(std::string name) {
    if (!ImageFlywheel::wheel.count(name)) {
      std::string path("/home/marijnfs/img/");
      path += name;

      std::cout << "loading: " << path << std::endl;
      ImageFlywheel::wheel[name] = new Image(path, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    return ImageFlywheel::wheel[name];
  }


  static void destroy() {
    for (auto &kv : ImageFlywheel::wheel)
      delete kv.second;
    ImageFlywheel::wheel.clear();
  }
};

#endif
