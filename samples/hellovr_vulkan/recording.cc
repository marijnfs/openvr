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

void Recording::serialise(Bytes *bytes) {
  ::capnp::MallocMessageBuilder cap_message;
  auto builder = cap_message.initRoot<cap::Recording>();
  
  auto object_builder = builder.initObjects(objects.size());
  //for (int i(0); i < scene.objects.size(); ++i)
  {
    int i(0);
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

void Recording::deserialise(Bytes &bytes, Scene *scene) {
  ::capnp::FlatArrayMessageReader reader(bytes.kjwp());
  variables.clear();
  objects.clear();
  triggers.clear();
  snaps.clear();
  
  auto rec = reader.getRoot<cap::Recording>();

  auto rec_names = rec.getNames();
  auto rec_objects = rec.getObjects();
  auto rec_variables = rec.getVariables();
  auto rec_triggers = rec.getTriggers();
  auto rec_snaps = rec.getSnaps();

  for (auto n : rec_names)
    scene->names.push_back(n);

  for (auto v : rec_variables)
    variables.push_back(read_variable(v));
  
  for (auto t : rec_triggers)
    triggers.push_back(read_trigger(t));
  
  for (auto ob : rec_objects)
    objects.push_back(read_object(ob));

  for (auto rsnap : rec_snaps) {
    auto snap = new Snap();
    snap->deserialise(rsnap);
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
