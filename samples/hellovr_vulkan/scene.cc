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
    //auto vr = Global::vr();
    
    //grab position from vrsystem if tracked
  }

  //if (acted) { //somehow get it from actions
  // }
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
    if (t->check())
      function_map[names[t->function_nameid]]();
  }
  
  if (record) {
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
    auto &variable = kv.second;
    if (variable->changed) {
      uint idx = rec->variables.size(); //will be the index of newly added variable
      rec->variables.push_back(variable->copy());
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
      rec->index_map[(void*)&trigger] = idx; //to find the index to trigger later for referals
    } else {
      snap.trigger_ids.push_back(rec->index_map[(void*)&trigger]);
    }
  }

  rec->snaps.push_back(snap_ptr);
  
  //store snap in recording
}
