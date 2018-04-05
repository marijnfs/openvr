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
  

  auto rec = reader.getRoot<cap::Recording>();

  auto rec_names = rec.getNames();
  auto rec_objects = rec.getObjects();
  auto rec_variables = rec.getVariables();
  auto rec_triggers = rec.getTriggers();
  auto rec_snaps = rec.getSnaps();

  for (auto n : rec_names)
    scene->names.push_back(n);

  for (auto v : rec_variables) {
    Variable *new_var(0);
    switch (v.which()) {
    case cap::Variable::DISTANCE: {
      auto n1 = v.getDistance().getNameId1();
      auto n2 = v.getDistance().getNameId2();
      new_var = new DistanceVariable(n1, n2);
            
      break;
    }
      case cap::Variable::FREE:
        new_var = new FreeVariable();
        break;
    }
    new_var->nameid = v.getNameId();
  }

  
  for (auto t : rec_triggers) {
    Trigger *new_t(0);
    switch (t.which()) {
    case cap::Trigger::LIMIT: {
      auto n = t.getLimit().getNameId();
      new_t = new LimitTrigger(n, t.getLimit().getLimit());
      break;
    }
    case cap::Trigger::CLICK:
      new_t = new ClickTrigger();
      break;
      
    }

    new_t->function_nameid = t.getFunctionNameId();
    triggers.push_back(new_t);
  }
  
  for (auto ob : rec_objects) {
    Object *new_ob(0);
    switch (ob.which()) {
    case cap::Object::HMD:
      new_ob = new HMD();
      break;
    case cap::Object::CONTROLLER:
      new_ob = new Controller();
      
      break;
    case cap::Object::POINT:
      new_ob = new Point();
      break;
    case cap::Object::CANVAS:
      new_ob = new Canvas(ob.getCanvas());
      
      break;
    }
    
    new_ob->quat[0] = ob.getQuat().getA();
    new_ob->quat[1] = ob.getQuat().getB();
    new_ob->quat[2] = ob.getQuat().getC();
    new_ob->quat[3] = ob.getQuat().getD();

    new_ob->p[0] = ob.getPos().getX();
    new_ob->p[1] = ob.getPos().getY();
    new_ob->p[2] = ob.getPos().getZ();

    new_ob->nameid = ob.getNameId();
    
    objects.push_back(new_ob);
  }  

  for (auto rec_snap : rec_snaps) {
    auto snap = new Snap();
    snap->t = rec_snap.getTimestamp();
    snap->reward = rec_snap.getReward();
    snap->object_ids.reserve(rec_snap.getObjectIds().size());
    snap->trigger_ids.reserve(rec_snap.getTriggerIds().size());
    snap->variable_ids.reserve(rec_snap.getVariableIds().size());

    for (auto n : rec_snap.getObjectIds())
      snap->object_ids.push_back(n);
    
    for (auto n : rec_snap.getTriggerIds())
      snap->trigger_ids.push_back(n);
    
    for (auto n : rec_snap.getVariableIds())
      snap->variable_ids.push_back(n);

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
