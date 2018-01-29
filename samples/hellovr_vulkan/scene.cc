#include "scene.h"

std::vector<float> Object::get_pos() {
  
}

void Object::render() {
  go.render(t);
}

void Scene::init() {
	
}

void Scene::process_triggers() {
  for (auto t : triggers) {
    if (t->check())
      t->triggered();
  }
}

void Scene::render() {
  for (auto &o : objects)
    o.second->render();
}

