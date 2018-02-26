#include "scene.h"


std::vector<float> Object::get_pos() {

}



void Object::set_pos(std::vector<float> pos) {

}

void Scene::init() {
	
}

void Scene::process_triggers() {
  for (auto t : triggers) {
    if (t->check())
      t->triggered();
  }
}

void Scene::draw() {
  for (auto o : objects)
  	o->draw();
}

void Scene::add_plane(std::string name) {
	auto go = new GraphicsObject();
	go->init_screen();

	objects.push_back(new Object{go});
}

void Scene::set_pos(std::string, std::vector<float> pos) {

}
