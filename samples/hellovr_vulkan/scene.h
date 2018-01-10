#ifndef __SCENE_H__
#define __SCENE_H__

#include <map>
#include <string> 
#include <vector>

#include "vrsystem.h"
#include "vulkansystem.h"

//triggers to set up events
struct Trigger {
  
  Object *obj;
  Controller *con;
  
  void check() {
    auto pos = obj->get_pos();
    auto con_pos = con->get_pos();
    float d = dist(pos, con_pos);
    return d < .1;
  }

  void triggered() {}
};

struct Scene {
  std::map<std::string, GraphicsObject*> objects;
  std::vector<Trigger*> triggers;
  
  void init();
  
  void add_plane(std::string name);
  
  void set_pos(std::string, std::vector<float> pos);
  
	

  

  void set_rotation(vector<float> &rot);

  void process_triggers(); //step through triggers
  
  
};

#endif
