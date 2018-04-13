#include "scene.h"
#include "vrsystem.h"
#include "global.h"

using namespace std;

void Controller::update() {
  if (tracked) {
    auto &vr = Global::vr();
    if (right) {
      from_mat4(vr.right_controller.t);
      if (vr.right_controller.pressed && !pressed)
        clicked = true;
      pressed = vr.right_controller.pressed;
    } else {
      from_mat4(vr.left_controller.t);
      if (vr.left_controller.pressed && !pressed)
        clicked = true;
      pressed = vr.left_controller.pressed;
    }
    
    
  }

  //if (acted) { //somehow get it from actions
  // }
}

void HMD::update() {
  if (tracked) {
    auto &vr = Global::vr();
    from_mat4(vr.hmd_pose);
    
    //vr.hmd_pose = glm_to_mat4(to_mat4()); ///TODO, we need this for replaying

    cout << "HMD: [" << p[0] << " " << p[1] << " " << p[2] << "] [" <<
      quat[0] << " "  << quat[1] << " "  << quat[2] << " "  << quat[3] << "]" << endl;
  }
}

void Scene::add_trigger(Trigger *t, std::string funcname) {
  int id = register_name(funcname);
  t->function_nameid = id;
  triggers.push_back(t);
  
}


void Scene::step() {
  ++time;
  
  for (auto &kv : objects) {
    kv.second->update();
  }
  
  for (auto &kv : variables) {
    kv.second->update(*this);
  }
  
  for (auto &t : triggers) {
    if (t->check(*this)) {
      auto nameid = t->function_nameid;
      auto name = names[t->function_nameid];
      cout << nameid << " " << name << endl;
      function_map[name]();
    }  
  }
}

void Scene::snap(Recording *rec) {
  Snap *snap_ptr = new Snap();
  Snap &snap(*snap_ptr);
  snap.time = time;
  snap.reward = reward;
    
  for (auto &kv : objects) {
    auto &object(*kv.second);
    if (object.changed)//store a copy
      snap.object_ids.push_back(rec->add_object(object.copy()));
    else //refer to stored copy
      snap.object_ids.push_back(rec->index_map[(void*)&object]);
  }

  for (auto &kv : variables) {
    auto &variable(*kv.second);
    if (variable.changed)
      snap.variable_ids.push_back(rec->add_variable(variable.copy()));
    else
      snap.variable_ids.push_back(rec->index_map[(void*)&variable]);
  }


  for (auto &trigger : triggers) {
    if (trigger->changed)
      snap.trigger_ids.push_back(rec->add_trigger(trigger->copy()));
    else
      snap.trigger_ids.push_back(rec->index_map[(void*)trigger]);
  }

  rec->snaps.push_back(snap_ptr);
  
  //store snap in recording
}

bool InBoxTrigger::check(Scene &scene) {
  cout << scene.names[box_id] << " " << scene.names[target_id] << endl;

  cout << scene.names << endl;
  for (auto kv : scene.objects)
    cout << kv.first << " ";
  auto &box = scene.find<Box>(box_id);
  
  auto &target = scene.find(target_id);
  std::cout << box_id << " " << target_id << std::endl;
  std::cout << scene.objects.size() << std::endl;
  //todo: account for rotation, now we assume unrotated boxes
  bool in = target.p[0] > box.p[0] - box.width / 2 &&
    target.p[0] < box.p[0] + box.width / 2 &&
                  target.p[1] > box.p[1] - box.height / 2 &&
    target.p[1] < box.p[1] + box.height / 2 &&
                  target.p[2] > box.p[2] - box.depth / 2 &&
    target.p[2] < box.p[2] + box.depth / 2;
  
  return in;
}

Object *read_object(cap::Object::Reader reader) {
  switch (reader.which()) {
  case cap::Object::HMD: {
    auto o = new HMD();
    o->deserialise(reader);
    return o;
  }
  case cap::Object::CONTROLLER: {
    auto o = new Controller();
    o->deserialise(reader);
    return o;
  }
  case cap::Object::POINT: {
    auto o = new Point();
    o->deserialise(reader);
    return o;
  }
  case cap::Object::CANVAS: {
    auto o = new Canvas();
    o->deserialise(reader);
    return o;
  }
  case cap::Object::BOX: {
    auto o = new Box();
    o->deserialise(reader);
    return o;
  }
    //hmd @3 : Void;
    //  controller @4 : Controller;
    //  point @5 : Void;
    //  canvas @6 : Text;
    //   box @7 : Box;

  }
}

Variable *read_variable(cap::Variable::Reader reader) {
  switch (reader.which()) {
  case cap::Variable::DISTANCE: {
    auto v = new DistanceVariable();
    v->deserialise(reader);
    return v;
  }
  case cap::Variable::FREE: {
    auto v = new FreeVariable();
    v->deserialise(reader);
    return v;
  }
  //  distance @2 : NamePair;
  //     free @3 : Float32;
  }
}

Trigger *read_trigger(cap::Trigger::Reader reader) {
  switch (reader.which()) {
  case cap::Trigger::LIMIT: {
    auto t = new LimitTrigger();
    t->deserialise(reader);
    return t;
  }    
  case cap::Trigger::CLICK: {
    auto t = new ClickTrigger();
    t->deserialise(reader);

     return t;
  }
  case cap::Trigger::IN_BOX: {
    auto t = new InBoxTrigger();
    t->deserialise(reader);
    return t;
  }
  }
  //limit @2 : NameLimit;
  //     click @3 : Void;
  //     inBox @4 : NamePair;

}
