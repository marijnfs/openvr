#include "scene.h"

void Scene::init() {
	
}

void Scene::process_triggers() {
  for (auto t : triggers) {
    if (t->check())
      t->triggered();
  }
}

void Scene::draw() {
  
}
