#ifndef __SCENE_H__
#define __SCENE_H__

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
    if (d < .1) {
      triggered();
    }
  }
};

struct Scene {
	std::vector<GraphicsObject*> objects;
  std::vector<Trigger*> triggers;
  

  void set_rotation(vector<float> &rot);

  void process_triggers(); //step through triggers
  
  
};

#endif
