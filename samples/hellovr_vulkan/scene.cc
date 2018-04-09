#include "scene.h"
#include "vrsystem.h"
#include "global.h"

using namespace std;

void Controller::update() {
  if (tracked) {
    auto &vr = Global::vr();
    if (right) {
      from_mat4(vr.right_controller.t);
    } else {
      from_mat4(vr.left_controller.t);
    }
    //auto &vr = Global::vr();
    
    //grab position from vrsystem if tracked
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
  for (auto name : names)
    std::cout << "n " << name << std::endl;
  int id = register_name(funcname);
  cout << id << endl;
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
      cout << names.size() << endl;
      cout << "func: " << nameid << " " << name << endl;
      function_map[names[t->function_nameid]]();
    }  
  }
}

void Scene::snap(Recording *rec) {
  Snap *snap_ptr = new Snap();
  Snap &snap(*snap_ptr);
  snap.t = time;
  snap.reward = reward;
    
  for (auto &kv : objects) {
    auto &object(*kv.second);
    if (object.changed) { //store a copy
      uint idx = rec->objects.size(); //will be the index of newly added object
      rec->objects.push_back(object.copy());
      snap.object_ids.push_back(idx);
      rec->index_map[(void*)&object] = idx; //to find the index to object later for referals
    } else { //refer to stored copy
      snap.object_ids.push_back(rec->index_map[(void*)&object]);
    }
  }

  for (auto &kv : variables) {
    auto &variable(*kv.second);
    if (variable.changed) {
      uint idx = rec->variables.size(); //will be the index of newly added variable
      rec->variables.push_back(variable.copy());
      snap.variable_ids.push_back(idx);
      rec->index_map[(void*)&variable] = idx; //to find the index to variable later for referals
    } else {
      snap.variable_ids.push_back(rec->index_map[(void*)&variable]);
    }
  }


  for (auto &trigger : triggers) {
    if (trigger->changed) {
      uint idx = rec->triggers.size(); //will be the index of newly added trigger
      rec->triggers.push_back(trigger->copy());
      snap.trigger_ids.push_back(idx);
      rec->index_map[(void*)trigger] = idx; //to find the index to trigger later for referals
    } else {
      snap.trigger_ids.push_back(rec->index_map[(void*)trigger]);
    }
  }

  rec->snaps.push_back(snap_ptr);
  
  //store snap in recording
}

bool InBoxTrigger::check(Scene &scene) {
  auto &box = scene.find<Box>(box_id);
  auto &target = scene.find(target_id);
  std::cout << box_id << " " << target_id << std::endl;
  //todo: account for rotation, now we assume unrotated boxes
  bool in = target.p[0] > box.p[0] - box.width / 2 &&
    target.p[0] < box.p[0] + box.width / 2 &&
                  target.p[1] > box.p[1] - box.height / 2 &&
    target.p[1] < box.p[1] + box.height / 2 &&
                  target.p[2] > box.p[2] - box.depth / 2 &&
    target.p[2] < box.p[2] + box.depth / 2;
  
  std::cout << "checking " << target.p[0] << " " << box.p[0] << " " << box.width << endl <<
    "checking " << target.p[1] << " " << box.p[1] <<" " << box.height << endl <<
    "checking " << target.p[2] << " " << box.p[2] << " " << box.depth << endl << endl;
                                                     if (in)
      throw "";
                                                     return in;
}
