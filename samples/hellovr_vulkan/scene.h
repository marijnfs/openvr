#ifndef __SCENE_H__
#define __SCENE_H__

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>
#include <fstream>

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

  std::vector<char>  result(pos);
  
  ifs.seekg(0, std::ios::beg);
  ifs.read(&result[0], pos);

  return result;
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
  //Buffer pos; 


  bool changed = true;
  int nameid = -1;
  
  Pos p;
  fquat quat;
  
  Object() : p(3) {}
  
  virtual Object *copy() {return new Object(*this); }
  
  void set_pos(Pos pos) { p = pos; changed = true; }
  void from_axis(float x, float y, float z, float angle) { //angle in degrees
    quat = glm::angleAxis<float>(angle, glm::vec3(x, y, z));
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
  
};

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
  bool clicked = false;
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

struct Trigger {
  bool changed = true;
  int nameid = -1;
  int function_nameid = -1;
  
  bool check() { return false; }

  virtual Trigger *copy() {
    return new Trigger(*this);
  }

  virtual void serialise(cap::Trigger::Builder builder) {
    builder.setNameId(nameid);
    builder.setFunctionNameId(function_nameid);
  }
};

struct Snap {
  //state
  // all objects, with their states
  // world state vars
  // reward

  uint t = 0;
  float reward = 0;
  std::vector<uint> object_ids;
  std::vector<uint> trigger_ids;
  std::vector<uint> variable_ids;

  void serialise(cap::Snap::Builder builder) {
    builder.setTimestamp(t);
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
  
};

struct Scene;
struct Recording {
  std::vector<Snap*> snaps;
  std::vector<Object*> objects;
  std::vector<Variable*> variables;
  std::vector<Trigger*> triggers;
  
  std::map<void*, int> index_map; //temporary data

  void deserialise(Bytes &b, Scene *scene);
  void load(std::string filename, Scene *scene);

  void serialise(Bytes *b);
  void save(std::string filename);
  
  void update();
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

};

struct FreeVariable : public Variable {
  void serialise(cap::Variable::Builder builder) {
    Variable::serialise(builder);
    builder.setFree(val);
  }

  void set_value(float val_) {
    val = val_;
    changed = true;
  }
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
    clear_objects();
    clear_triggers();
  }

  void set_tracked(bool tracked) {
   for (auto kv : objects) {
      //skip hmd and controller
     try {dynamic_cast<HMD*>(kv.second)->tracked = tracked;} catch(...) {}
      try {dynamic_cast<Controller*>(kv.second)->tracked = tracked;} catch(...) {}
      
    }}
  
  void clear_objects() {
    for (auto kv : objects) {
      //skip hmd and controller
      try {dynamic_cast<HMD*>(kv.second);} catch(...) {continue;}
      try {dynamic_cast<Controller*>(kv.second);} catch(...) {continue;}
      
      delete kv.second;
      objects.erase(kv.first);
    }
  }

  void clear_triggers() {
    for (auto t : triggers)
      delete t;
    triggers.clear();
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
    name_map[name] = name_map.size();
    names.push_back(name);
  }  
  
  void set_reward(float r_) {
    reward = r_;
  }

  void register_function(std::string name, void_function func) {
    function_map[name] = func;
  }
  
  void add_trigger(Trigger *t, std::string funcname) {
    int id = register_name(funcname);
    
    t->function_nameid = id;
    triggers.push_back(t);
  }

  float dist(Pos p1, Pos p2) {
    float d(0);
    //Object &o = find(name);

    for (int i(0); i < 3; ++i)
      d += (p1[i] - p2[i]) * (p1[i] - p2[i]);
    return sqrt(d);
  }

  void visit(ObjectVisitor &visitor) {
    std::cout << "visitor " << std::endl;
    visitor.i = 0;
    for (auto &kv : objects) {
      std::cout << "visiting object " << visitor.i << std::endl;
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
};

struct InBoxTrigger : public Trigger {
  bool changed = false;
  int target_id = -1, box_id = -1;

  InBoxTrigger(){}
 InBoxTrigger(int tid, int bid) : target_id(tid), box_id(bid) {}

  virtual void serialise(cap::Trigger::Builder builder) {
    Trigger::serialise(builder);
    auto l = builder.initInBox();
    l.setNameId1(target_id);
    l.setNameId2(box_id);    
  }
  
  bool check() {
    
  }
  
};

struct LimitTrigger : public Trigger {
  bool changed = false;
  int nameid = -1; //var name
  float limit = 0;
  
  LimitTrigger(){}
  
 LimitTrigger(int id, float limit_) : nameid(id), limit(limit_) {}

  virtual void serialise(cap::Trigger::Builder builder) {
    Trigger::serialise(builder);
    auto l = builder.initLimit();
    l.setNameId(nameid);
    l.setLimit(limit);    
  }
  
  bool check() {
  }
};

struct ClickTrigger : public Trigger {
  int oid = -1;

  ClickTrigger(){}
 ClickTrigger(uint oid_) : oid(oid_) {}
  
  bool check() {
    return rand() % 100 == 0;
  }

  void serialise(cap::Trigger::Builder builder) {
    Trigger::serialise(builder);
    builder.setClick();
  }
  
};



#endif
