#include "scene.h"

std::vector<float> Object::get_pos() {
  return std::vector<float>{t[12], t[13], t[14]};
}

void Object::set_pos(std::vector<float> p) {
  t[12] = p[0];
  t[13] = p[1];
  t[14] = p[2];
}

void Object::set_pos_orientation(std::vector<float> p, std::vector<float> o) {
  t.identity();
  t.translate(p[0], p[1], p[2]);
  t.rotateX(o[0]);
  t.rotateY(o[1]);
  t.rotateZ(o[2]);
}

void Object::set_t(Matrix4 t_) {
  t = t_;
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

