#include "scene.h"
#include "bytes.h"
#include "serialise.h"
#include "gzstream.h"

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <capnp/serialize-packed.h>

#include <algorithm>
#include <iterator>

using namespace std;

void Recording::load_scene(int i, Scene *scene) {
  scene->clear(false);
  auto &snap = *snaps[i];

  //todo
  t = snap.t;
  reward = snap.reward;
  
  for (auto o : snap.object_ids) {
    string name = scene.names[objects[o]->name_id];
    scene.objects[name] = objects[o];
  }

  for (auto v : snap.variable_ids) {
    string name = scene.names[variables[v]->name_id];
    scene.variables[name] = variables[v];
  }

  for (auto t : snap.trigger_ids)
    scene.triggers.push_back(triggers[t]);
  
}

void Recording::serialise(Bytes *bytes) {
  ::capnp::MallocMessageBuilder cap_message;
  auto builder = cap_message.initRoot<cap::Recording>();
  

  {
    int i(0);
    auto name_builder = builder.initNames(names.size());
    for (auto n : names)
      name_builder.set(i++, n);
  }
  //for (int i(0); i < scene.objects.size(); ++i)
  {
    int i(0);
    auto object_builder = builder.initObjects(objects.size());
    for (auto o : objects)
      o->serialise(object_builder[i++]);
  }
  
  {
    int i(0);
    auto var_builder = builder.initVariables(variables.size());
    for (auto v : variables)
      v->serialise(var_builder[i++]);
  }

  {
    int i(0);
    auto trig_builder = builder.initTriggers(triggers.size());
    for (auto &t : triggers)
      t->serialise(trig_builder[i++]);
  }

  {
    int i(0);
    auto snap_builder = builder.initSnaps(snaps.size());
    for (auto &s : snaps)
      s->serialise(snap_builder[i++]);
  }

  auto cap_data = messageToFlatArray(cap_message);
  auto cap_bytes = cap_data.asBytes();
  bytes->resize(cap_bytes.size());
  memcpy(&(*bytes)[0], &cap_bytes[0], cap_bytes.size());
  //copy(cap_bytes.begin(), cap_bytes.end(), &bytes[0]);
}

int Recording::add_object(Object *o) {
  int idx = objects.size();
  objects.push_back(o);
  index_map[(void*)o] = idx;
  return idx;
}

int Recording::add_variable(Variable *v) {
  int idx = variables.size();
  variables.push_back(v);
  index_map[(void*)v] = idx;
  return idx;
}

int Recording::add_trigger(Trigger *t) {
  int idx = triggers.size();
  triggers.push_back(t);
  index_map[(void*)t] = idx;
  return idx;
}

void Recording::deserialise(Bytes &bytes, Scene *scene) {
  ::capnp::FlatArrayMessageReader reader(bytes.kjwp());
  variables.clear();
  objects.clear();
  triggers.clear();
  snaps.clear();
  index_map.clear();
  
  auto rec = reader.getRoot<cap::Recording>();

  auto rec_names = rec.getNames();
  auto rec_objects = rec.getObjects();
  auto rec_variables = rec.getVariables();
  auto rec_triggers = rec.getTriggers();
  auto rec_snaps = rec.getSnaps();

  for (auto n : rec_names)
    scene->register_name(n);

  for (auto v : rec_variables)
    add_variable(read_variable(v));

  for (auto t : rec_triggers)
    add_trigger(read_trigger(t));
  
  for (auto ob : rec_objects)
    add_object(read_object(ob));

  for (auto rsnap : rec_snaps) {
    auto snap = new Snap();
    snap->deserialise(rsnap);
    snaps.push_back(snap);
  }

}

void Recording::load(std::string filename, Scene *scene) {
  igzstream ifs(filename.c_str());

  Bytes b;
  ifs.seekg(0, std::ios::end);
  b.reserve(ifs.tellg());
  ifs.seekg(0, std::ios::beg);
      
  copy(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), b.begin());
  deserialise(b, scene);
}

void Recording::save(std::string filename) {
  Bytes b;
  serialise(&b);

  ogzstream of(filename.c_str());
  of.write((char*)&b[0], b.size());
}
