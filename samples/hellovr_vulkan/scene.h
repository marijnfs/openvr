#ifndef __SCENE_H__
#define __SCENE_H__

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>
#include <fstream>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

typedef glm::fquat fquat;
//typedef typename std::vector<float> Pos;
typedef typename glm::fvec3 Pos;

#include "bytes.h"
#include "snap.capnp.h"


struct Variable;
struct Trigger;

typedef typename std::function<void(void)> void_function;

inline std::string concat(std::string name, int id) {
  std::ostringstream oss;
  oss << name << id;
  return oss.str();
}

inline void write_all_bytes(std::string filename, std::vector<char> &bytes) {
  std::ofstream ofs(filename.c_str(), std::ios::binary | std::ios::out);

  ofs.write(&bytes[0], bytes.size());
}

inline std::vector<char> read_all_bytes(std::string filename)
{
  std::ifstream ifs(filename.c_str(), std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();
  std::vector<char> data(pos);
  
  ifs.seekg(0, std::ios::beg);
  ifs.read(&data[0], pos);

  return data;
}

struct Canvas;
struct Controller;
struct Point;
struct HMD;
struct Box;

struct ObjectVisitor {
  int i = 0;

  virtual void visit(Canvas &canvas) {};
  virtual void visit(Controller &controller) {};
  virtual void visit(Point &point) {};
  virtual void visit(HMD &hmd) {};
  virtual void visit(Box &box) {};
};

struct Object {
  bool changed = true;
  int nameid = -1;
  
  Pos p;
  fquat quat;
  
  Object() : p(3) {}
  
  virtual Object *copy() {return new Object(*this); }
  
  void set_pos(Pos pos) { p = pos; changed = true; }
  void from_axis(float x, float y, float z, float angle) { //angle in degrees
    quat = glm::angleAxis<float>(angle, glm::vec3(x, y, z));
    changed = true;
  }

  void look_at(Pos to, Pos up) {
    quat = glm::quat_cast(glm::lookAt(p, to, up));
  }
  
  float angle() {
    return glm::angle(quat);
  }

  glm::mat4 to_mat4() {
    auto m = glm::toMat4(quat);
    m[3][0] = p[0];
    m[3][1] = p[1];
    m[3][2] = p[2];
    m[3][3] = 1.0;
    return m;
  }

  void from_mat4(glm::fmat4 m) {
    auto ptr = (float*)glm::value_ptr(m);
    quat = glm::quat_cast(m);
    p[0] = ptr[12];
    p[1] = ptr[13];
    p[2] = ptr[14];
  }

  virtual void update() {
  }

  virtual void visit(ObjectVisitor &visitor) {
  }

  
  
  virtual void serialise(cap::Object::Builder builder) {
    auto q = builder.initQuat();
    q.setA(quat[0]);
    q.setB(quat[1]);
    q.setC(quat[2]);
    q.setD(quat[3]);

    auto p_ = builder.initPos();
    p_.setX(p[0]);
    p_.setY(p[1]);
    p_.setZ(p[2]);

    builder.setNameId(nameid);
  }

  virtual void deserialise(cap::Object::Reader reader) {
    nameid = reader.getNameId();

    auto q = reader.getQuat();
    quat[0] = q.getA();
    quat[1] = q.getB();
    quat[2] = q.getC();
    quat[3] = q.getD();

    auto p_ = reader.getPos();
    p[0] = p_.getX();
    p[1] = p_.getY();
    p[2] = p_.getZ();    
  }
};

Object *read_object(cap::Object::Reader reader);

struct Box : public Object {
  float width = 1, height = 1, depth = 1;
  std::string tex_name = "stub.png";
  
  void serialise(cap::Object::Builder builder) {
    Object::serialise(builder);
    auto b = builder.initBox();
    b.setW(width);
    b.setH(height);
    b.setD(depth);
    b.setTexture(tex_name);
  }

  void deserialise(cap::Object::Reader reader) {
    Object::deserialise(reader);
    auto b = reader.getBox();

    width = b.getW();
    height = b.getH();
    depth = b.getD();
    tex_name = b.getTexture();
  }
  
  void set_dim(float w, float h, float d) {
    width = w;
    height = h;
    depth = d;
    changed = true;
  }

  void set_texture(std::string name) {
    changed = true;
    tex_name = name;
  }

  Object *copy() {
    return new Box(*this);
  }
  
  void visit(ObjectVisitor &visitor) {
    visitor.visit(*this);
  }

};

struct Point : public Object {
  void serialise(cap::Object::Builder builder) {
    Object::serialise(builder);
    builder.setPoint();
  }

  Object *copy() {
    return new Point(*this);
  }
  
  void visit(ObjectVisitor &visitor) {
    visitor.visit(*this);
  }

};

struct Canvas : public Object {
  std::string tex_name = "stub.png";

 Canvas() {}
 Canvas(std::string tex_name_) : tex_name(tex_name_) {}

  Object *copy() {
    return new Canvas(*this);
  }

  void serialise(cap::Object::Builder builder) {
    Object::serialise(builder);
    builder.setCanvas(tex_name);
  }

  void deserialise(cap::Object::Reader reader) {
    Object::deserialise(reader);
    tex_name = reader.getCanvas();
  }

  void set_texture(std::string name) {
    changed = true;
    tex_name = name;
  }

  void visit(ObjectVisitor &visitor) {
    visitor.visit(*this);
  }

};

struct HMD : public Object {
  bool tracked = true;
  
  void update();

  void serialise(cap::Object::Builder builder) {
    Object::serialise(builder);
    builder.setHmd();
  }

  Object *copy() {
    return new HMD(*this);
  }

  void visit(ObjectVisitor &visitor) {
    visitor.visit(*this);
  }

};

struct Controller : public Object {
  bool right = true;
  bool pressed = false; //true if pressed
  bool clicked = false; //true if pressed and previously unpressed (so only one frame)
  bool tracked = true;

  Controller(){}
 Controller(bool right_) : right(right_) {
  }

  Object *copy() {
    return new Controller(*this);
  }

  void update();

  void visit(ObjectVisitor &visitor) {
    visitor.visit(*this);
  }

  void serialise(cap::Object::Builder builder) {
    Object::serialise(builder);
    auto c = builder.initController();
    c.setRight(right);
    c.setClicked(clicked);
    c.setPressed(pressed);
  }

  void deserialise(cap::Object::Reader reader) {
    Object::deserialise(reader);
    
    auto c = reader.getController();
    right = c.getRight();
    clicked = c.getClicked();
    pressed = c.getPressed();
  }

};

struct PrintVisitor : public ObjectVisitor {
  void visit(Canvas &canvas) {
    std::cout << "a canvas" << std::endl;
  }

  void visit(Controller &controller) {
    std::cout << "a controller" << std::endl;
  }

  void visit(Point &point) {
    std::cout << "a point" << std::endl;
  }

  void visit(HMD &hmd) {
    std::cout << "an HMD" << std::endl;
  }
};

struct Scene;
struct Trigger {
  bool changed = true;
  //int nameid = -1;
  int function_nameid = -1;
  
  virtual bool check(Scene &scene) { return false; }

  virtual Trigger *copy() {
    return new Trigger(*this);
  }

  virtual void serialise(cap::Trigger::Builder builder) {
    //builder.setNameId(nameid);
    builder.setFunctionNameId(function_nameid);
  }
  
  virtual void deserialise(cap::Trigger::Reader reader) {
    //nameid = reader.getNameId();
    function_nameid = reader.getFunctionNameId();
  }
};

Trigger *read_trigger(cap::Trigger::Reader reader);
  
struct Snap {
  //state
  // all objects, with their states
  // world state vars
  // reward

  uint time = 0;
  float reward = 0;
  std::vector<uint> object_ids;
  std::vector<uint> trigger_ids;
  std::vector<uint> variable_ids;

  void serialise(cap::Snap::Builder builder) {
    builder.setTimestamp(time);
    builder.setReward(reward);

    {
      auto l = builder.initObjectIds(object_ids.size());
      for (size_t i(0); i < object_ids.size(); ++i)
        l.set(i, object_ids[i]);
    }
    {
      auto l = builder.initTriggerIds(trigger_ids.size());
      for (size_t i(0); i < trigger_ids.size(); ++i)
        l.set(i, trigger_ids[i]);
    }
    {
      auto l = builder.initVariableIds(variable_ids.size());
      for (size_t i(0); i < variable_ids.size(); ++i)
        l.set(i, variable_ids[i]);
    }    
  }
  
  void deserialise(cap::Snap::Reader reader) {
    time = reader.getTimestamp();
    reward = reader.getReward();
    object_ids.reserve(reader.getObjectIds().size());
    trigger_ids.reserve(reader.getTriggerIds().size());
    variable_ids.reserve(reader.getVariableIds().size());

    for (auto n : reader.getObjectIds())
      object_ids.push_back(n);
    
    for (auto n : reader.getTriggerIds())
      trigger_ids.push_back(n);
    
    for (auto n : reader.getVariableIds())
      variable_ids.push_back(n);    
  }

};

struct Recording {
  std::vector<Snap*> snaps;
  std::vector<Object*> objects;
  std::vector<Variable*> variables;
  std::vector<Trigger*> triggers;
  
  std::map<void*, int> index_map; //temporary data

  void deserialise(Bytes &b, Scene *scene);
  void load(std::string filename, Scene *scene);

  void serialise(Bytes *b, Scene &scene);
  void save(std::string filename, Scene &scene);

  int add_object(Object *o);
  int add_variable(Variable *v);
  int add_trigger(Trigger *t);

  int size() { return snaps.size(); }
  void load_scene(int i, Scene *scene);
  
  //void update();
};

//struct Scene;
struct Variable {
  bool changed = true;
  int nameid = -1;
  float val = 0;
  
  virtual void update(Scene &scene) { }
  virtual float value() {return val;}
  virtual Variable *copy() {return new Variable(*this);}

  virtual void serialise(cap::Variable::Builder builder) {
    builder.setNameId(nameid);
  }
  
  virtual void deserialise(cap::Variable::Reader reader) {
    nameid = reader.getNameId();
  }
};

Variable *read_variable(cap::Variable::Reader reader);

struct FreeVariable : public Variable {

  void set_value(float val_) {
    val = val_;
    changed = true;
  }
  
  void serialise(cap::Variable::Builder builder) {
    Variable::serialise(builder);
    builder.setFree(val);
  }

  virtual void deserialise(cap::Variable::Reader reader) {
    Variable::deserialise(reader);
    val = reader.getFree();
  }

  Variable *copy() {return new FreeVariable(*this);}
};

struct Scene {
  uint time = 0;
  float reward = 0;
  bool record = false;
  
  std::map<std::string, Object*> objects;
  std::map<std::string, Variable*> variables;
  std::vector<Trigger*> triggers;

  std::map<std::string, void_function> function_map;
  std::map<std::string, int> name_map;
  std::vector<std::string> names;
  
  int operator()(std::string name) {
    return register_name(name);
  }
  
  Object &find(int nameid) {
    return *objects[names[nameid]];
  }

  template <typename T>
  T &find(int name_id) {
    return *reinterpret_cast<T*>(objects[names[name_id]]);
  }

  template <typename T>
  T &find(std::string name) {
    return *reinterpret_cast<T*>(objects[name]);
  }
  
  Object &find(std::string name) {
    if (!objects.count(name))
      throw "no such object";
    return *objects[name];
  }

  template <typename T>
  T &variable(std::string name) {
    return *reinterpret_cast<T*>(variables[name]);
  }
  
  void clear() {
    clear_objects(false);
    clear_triggers();
    clear_variables();
  }

  
  void set_tracked(bool tracked) {
   for (auto kv : objects) {
      //skip hmd and controller
     try {dynamic_cast<HMD*>(kv.second)->tracked = tracked;} catch(...) {}
      try {dynamic_cast<Controller*>(kv.second)->tracked = tracked;} catch(...) {}
      
    }}
  
  void clear_objects(bool filter_hmd_controller = true) {
    for (auto kv : objects) {
      //skip hmd and controller
      if (filter_hmd_controller) {
        if (dynamic_cast<HMD*>(kv.second) != NULL) continue;
        if (dynamic_cast<Controller*>(kv.second) != NULL) continue;
      }

      delete kv.second;
      objects.erase(kv.first);
    }
  }

  void clear_triggers() {
    for (auto t : triggers)
      delete t;
    triggers.clear();
  }

  void clear_variables() {
    for (auto kv : variables)
      delete kv.second;
    variables.clear();
  }
  
  void step();
  void snap(Recording *rec);

  void add_object(std::string name, Object *o) {
    int nameid = register_name(name);
    o->nameid = nameid;
    if (objects.count(name))
      delete objects[name];
    objects[name] = o;
  }

  void add_hmd() {
    add_object("hmd", new HMD());
  }
  
  void add_canvas(std::string name) {
    add_object(name, new Canvas());
  }
  
  void add_point(std::string name) {
    add_object(name, new Point());
  }

  void add_box(std::string name) {
    add_object(name, new Box());
  }
  
  
  void add_variable(std::string name, Variable *v) {
    int nameid = register_name(name);
    v->nameid = nameid;
    variables[name] = v;
  }


  
  void set_pos(std::string name, Pos pos) {
    Object &o = find(name);
    o.set_pos(pos);
  }

  Pos get_pos(std::string name) {
    Object &o = find(name);
    return o.p;
  }

  void set_texture(std::string name, std::string tex) {
    reinterpret_cast<Canvas&>(find(name)).set_texture(tex);
  }

  int register_name(std::string name) {
    if (name_map.count(name)) return name_map[name];
    names.push_back(name);
    return name_map[name] = name_map.size();
  }
  
  void set_reward(float r_) {
    reward = r_;
  }

  void register_function(std::string name, void_function func) {
    function_map[name] = func;
  }
  
  void add_trigger(Trigger *t, std::string funcname);

  float dist(Pos p1, Pos p2) {
    float d(0);
    //Object &o = find(name);

    for (int i(0); i < 3; ++i)
      d += (p1[i] - p2[i]) * (p1[i] - p2[i]);
    return sqrt(d);
  }

  void visit(ObjectVisitor &visitor) {
    visitor.i = 0;
    for (auto &kv : objects) {
      //std::cout << "visiting object " << visitor.i << std::endl;
      kv.second->visit(visitor);
      ++visitor.i;
    }
  }
  
  void start_recording() {
    //create unique id
    //grab current time
    record = true;
  }

  void end_recording() {
    //finalise, save last reward

    record = false;
  }
  
};

struct DistanceVariable : public Variable {
  int oid1 = -1, oid2 = -1;

  DistanceVariable() {}
  
 DistanceVariable(int oid1_, int oid2_) :
    oid1(oid1_), oid2(oid2_)
  {}

  void update(Scene &scene) {
    val = scene.dist(scene.find(oid1).p, scene.find(oid2).p);
  }

  void serialise(cap::Variable::Builder builder) {
    Variable::serialise(builder);
    auto b = builder.initDistance();
    b.setNameId1(oid1);
    b.setNameId2(oid2);
  }

  void deserialise(cap::Variable::Reader reader) {
    Variable::deserialise(reader);
    auto d = reader.getDistance();
    oid1 = d.getNameId1();
    oid2 = d.getNameId2();
  }

  Variable *copy() {return new DistanceVariable(*this);}
};

struct InBoxTrigger : public Trigger {
  bool changed = false;
  int target_id = -1, box_id = -1;

  InBoxTrigger(){}
 InBoxTrigger(int bid, int tid) : target_id(tid), box_id(bid) {}

  virtual void serialise(cap::Trigger::Builder builder) {
    Trigger::serialise(builder);
    auto l = builder.initInBox();
    l.setNameId1(target_id);
    l.setNameId2(box_id);
  }

  virtual void deserialise(cap::Trigger::Reader reader) {
    Trigger::deserialise(reader);
    auto l = reader.getInBox();
    target_id = l.getNameId1();
    box_id = l.getNameId2();
  }

  Trigger *copy() { return new InBoxTrigger(*this); }
  
  bool check(Scene &scene);  
};

struct LimitTrigger : public Trigger {
  bool changed = false;
  int nameid = -1; //var name
  float limit = 0;
  
  LimitTrigger(){}
  
 LimitTrigger(int id, float limit_) : nameid(id), limit(limit_) {}

  virtual void serialise(cap::Trigger::Builder builder) {
    Trigger::deserialise(builder);
    auto l = builder.initLimit();
    l.setNameId(nameid);
    l.setLimit(limit);    
  }
  
  bool check(Scene &scene) {
    return scene.variables[scene.names[nameid]]->val > limit;
  }

  Trigger *copy() { return new LimitTrigger(*this); }
};

struct ClickTrigger : public Trigger {
  int oid = -1;

  ClickTrigger(){}
 ClickTrigger(uint oid_) : oid(oid_) {}
  
  bool check(Scene &scene) {
    return scene.find<Controller>(oid).clicked;
  }

  void serialise(cap::Trigger::Builder builder) {
    Trigger::serialise(builder);
    //builder.setClick();
  }

  void deserialise(cap::Trigger::Reader reader) {
    Trigger::deserialise(reader);
    //reader.getClick();
  }

  Trigger *copy() { return new ClickTrigger(*this); }
};



#endif
