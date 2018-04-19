#include "scene.h"
#include "vrsystem.h"
#include "global.h"

using namespace std;

void Controller::update() {
  clicked = false;
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
    changed = true;
  }
}

void HMD::update() {
  if (tracked) {
    auto &vr = Global::vr();
    
    from_mat4(vr.hmd_pose);
    changed = true;
    //vr.hmd_pose = glm_to_mat4(to_mat4()); ///TODO, we need this for replaying

  }
}

void Scene::add_trigger(Trigger *t, std::string funcname) {
  int id = register_name(funcname);
  t->function_nameid = id;
  triggers.push_back(t);
  
}


void Scene::step() {
  ++time;
  
  for (auto &kv : objects)
    kv.second->update();
  
  for (auto &kv : variables)
    kv.second->update(*this);

  auto triggers_cpy = triggers; //triggers might adjust itself
  for (auto &t : triggers_cpy) {
    if (t->check(*this)) {
      auto nameid = t->function_nameid;
      auto name = names[t->function_nameid];
      function_map[name]();
    }  
  }
}

void Scene::snap(Recording *rec) {
  if (!record)
    return;
  Snap *snap_ptr = new Snap();
  Snap &snap(*snap_ptr);
  snap.time = time;
  snap.reward = reward;
    
  for (auto &kv : objects) {
    auto &object(*kv.second);
    if (object.changed) {//store a copy
      auto obj_cpy = object.copy();
      auto id = rec->add_object(obj_cpy);
      snap.object_ids.push_back(id);
      rec->index_map[(void*)&object] = id;
      object.changed = false;
    } else //refer to stored copy
      snap.object_ids.push_back(rec->index_map[(void*)&object]);
  }

  for (auto &kv : variables) {
    auto &variable(*kv.second);
    if (variable.changed) {
      auto var_cpy = variable.copy();
      auto id = rec->add_variable(var_cpy);
      snap.variable_ids.push_back(id);
      rec->index_map[(void*)&variable] = id;
      variable.changed = false;
    } else
      snap.variable_ids.push_back(rec->index_map[(void*)&variable]);
  }


  for (auto &trigger_ptr : triggers) {
    Trigger &trigger(*trigger_ptr);
    if (trigger.changed) {
      auto var_cpy = trigger.copy();
      auto id = rec->add_trigger(var_cpy);
      snap.trigger_ids.push_back(id);
      rec->index_map[(void*)&trigger] = id;
      trigger.changed = false;
    } else
      snap.trigger_ids.push_back(rec->index_map[(void*)&trigger]);
  }

  rec->snaps.push_back(snap_ptr);
  
  //store snap in recording
}

bool InBoxTrigger::check(Scene &scene) {
  auto &box = scene.find<Box>(box_id);
  
  auto &target = scene.find(target_id);
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
  default:
    throw StringException("unknown variable");
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
  case cap::Variable::MARK: {
    auto v = new MarkVariable();
    v->deserialise(reader);
    return v;
  }
  default:
    throw StringException("unknown variable");
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
  case cap::Trigger::NEXT: {
    auto t = new NextTrigger();
    t->deserialise(reader);
    return t;
  }
  default:
    throw StringException("unknown variable");
  }
}

Pose::Pose(Scene &scene) {
  base = scene.find<HMD>("hmd").p;
  baseq = scene.find<HMD>("hmd").quat;

  auto c_pos = scene.find<HMD>("controller").p;
  auto v = c_pos - base;
  arm_length = l2Norm(v);

  v /= arm_length;
  
}

Action::Action(Snap &cur, Snap &next) {
  
}
