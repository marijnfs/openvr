#ifndef __SCENE_H__
#define __SCENE_H__

#include <map>
#include <string> 
#include <vector>

#include "vrsystem.h"
#include "vulkansystem.h"
#include "util.h"


struct Object {
  std::vector<float> p, o;
  Matrix4 t;
  GraphicsObject go;

  Object();
  void set_pos(std::vector<float> p);
  void set_pos_orientation(std::vector<float> p, std::vector<float> o);
  void set_t(Matrix4 t);

  void render();

  std::vector<float> get_pos();
};

//triggers to set up events
struct Trigger {
  Object *obj;
  Controller *con;
  
  bool check() {
    auto pos = obj->get_pos();
    auto con_pos = con->get_pos();
    float d = dist(pos, con_pos);
    return d < .1;
  }

  void triggered() {}
};

struct Scene {
  std::map<std::string, Object*> objects;
  std::vector<Trigger*> triggers;
  
  void init();
  
  void add_plane(std::string name);
  
  void set_pos(std::string, std::vector<float> pos);
  void set_rotation(std::vector<float> &rot);

  void process_triggers(); //step through triggers

  void render();
};

#endif
