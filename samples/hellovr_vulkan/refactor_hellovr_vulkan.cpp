//========= Copyright Valve Corporation ============//

#include <string>
#include <cstdlib>
#include <inttypes.h>
#include <openvr.h>
#include <deque>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "shared/lodepng.h"
#include "shared/Matrices.h"
#include "shared/pathtools.h"

#include "util.h"
#include "utilvr.h"
#include "global.h"
#include "vulkansystem.h"
#include "vrsystem.h"
#include "windowsystem.h"
#include "flywheel.h"

#include "learningsystem.h"

using namespace std;

enum Orientation {
  Horizontal = 0,
  Vertical = 1,
  Laying = 2
};

struct ExperimentStep {
  Orientation orientation = Horizontal;
  float xdir = 0;
  float ydir = 0;
  float zdir = 0;
  int n_clicks = 0;

  float long_side = 0;
  float short_side = 0;

  ExperimentStep(){}
  ExperimentStep(Orientation o, float xdir_, float ydir_, float zdir_, int n_clicks_, float long_side_, float short_side_) :
    orientation(o), xdir(xdir_), ydir(ydir_), zdir(zdir_), n_clicks(n_clicks_),
    long_side(long_side_), short_side(short_side_)
  {}
};

struct FittsWorld {
  Scene &scene;
  int click = 0;
  int step = 0;
  int choice = 0;
  
  vector<ExperimentStep> steps;
  
  

  FittsWorld(Scene &scene_) : scene(scene_) {
    srand(123123);
    init_experiments();
    init();
  }
  
  void init_experiments() {
    //ExperimentStep(Orientation o, float xdir_, float ydir_, float zdir_, int n_clicks_, float long_side_, float short_side_) :
    vector<float> multipliers = {1.0, 2.0, 4.0};
    int n_clicks(10);
    float long_side(.2);
    float short_side(.01);
    float dist(.02);
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Vertical, dist * m + short_side * m2, 0, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Vertical, 0., 0, dist * m + short_side * m2, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Vertical, dist * m + short_side * m2, 0, dist * m + short_side * m2, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Vertical, dist * m + short_side * m2, 0, -dist * m - short_side * m2, n_clicks, long_side, short_side * m2));
    
    ////Horizontal
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Horizontal, 0, dist * m + short_side * m2, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Horizontal, 0., 0, dist * m + short_side * m2, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Horizontal, 0, dist * m + short_side * m2, dist * m + short_side * m2, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
      steps.push_back(ExperimentStep(Horizontal, 0, dist * m + short_side * m2, -dist * m - short_side * m2, n_clicks, long_side, short_side * m2));
    
    //Laying
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Laying, dist * m + short_side * m2, 0, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Laying, 0., dist * m + short_side * m2, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Laying, dist * m + short_side * m2, dist * m + short_side * m2, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
      steps.push_back(ExperimentStep(Laying, dist * m + short_side * m2, -dist * m - short_side * m2, 0, n_clicks, long_side, short_side * m2));
    random_shuffle(steps.begin(), steps.end());
    
    steps.resize(steps.size() / 2);///TODO
  }
    
  void init() {
    cout << "Fitts World INIT" << endl;
    //scene.add_canvas("test");
    scene.add_hmd();
    scene.add_object("controller", new Controller(true));
    //scene.set_pos("test", Pos(1, 1, 1));

    scene.add_variable("step", new FreeVariable());
    scene.add_variable("click", new FreeVariable());

    scene.add_variable("orientation", new FreeVariable());
    scene.add_variable("xdir", new FreeVariable());
    scene.add_variable("ydir", new FreeVariable());
    scene.add_variable("zdir", new FreeVariable());

    scene.add_variable("long_side", new FreeVariable());
    scene.add_variable("short_side", new FreeVariable());
    
    scene.add_variable("choice", new FreeVariable());

    scene.add_variable("start", new MarkVariable());
    scene.add_variable("end", new MarkVariable());
    
    scene.register_function("on_in_box", std::bind(&FittsWorld::on_in_box, *this));
    scene.register_function("on_start", std::bind(&FittsWorld::on_start, *this));
    scene.add_trigger(new ClickTrigger(scene("controller")), "on_start");

    scene.add_box("startbox");
    scene.set_pos("startbox", Pos(0, .9, -.05));
    scene.find<Box>("startbox").set_dim(.05, .05, .05);
    scene.find<Box>("startbox").set_texture("white-checker.png");

    cout << "Fitts World Done INIT" << endl;
  }

  void on_in_box() {
    //cout << "ON IN BOX" << endl;

    if (scene.find<Controller>("controller").clicked == true) {
      scene.clear_scene();
      scene.set_reward(1);

      scene.add_trigger(new NextTrigger(), "on_start");
      scene.variable<MarkVariable>("end").set_value(1);
      //scene.end_recording();
      //scene.clear_objects();
      //scene.clear_triggers();
      //scene.add_trigger(new ClickTrigger(), "on_start");
    }
  }
  
  void on_start() {
    click++;
    if (click >= steps[step].n_clicks) {
      step++;
      cout << step << "/" << steps.size() << endl;
      click = 0;
    }
    if (step >= steps.size()) {
      scene.stop = true;
      return;
    }

    ExperimentStep &cur_step(steps[step]);

    int new_choice = rand() % 3;
    while (new_choice == choice)
      new_choice = rand() % 3;
    choice = new_choice;

    //store experiment vars;
    scene.variable<FreeVariable>("step").set_value(step);
    scene.variable<FreeVariable>("click").set_value(click);
    scene.variable<FreeVariable>("orientation").set_value(cur_step.orientation);

    scene.variable<FreeVariable>("long_side").set_value(cur_step.long_side);
    scene.variable<FreeVariable>("short_side").set_value(cur_step.short_side);

    scene.variable<FreeVariable>("xdir").set_value(cur_step.xdir);
    scene.variable<FreeVariable>("ydir").set_value(cur_step.ydir);
    scene.variable<FreeVariable>("zdir").set_value(cur_step.zdir);

    scene.variable<FreeVariable>("choice").set_value(choice);
    scene.variable<MarkVariable>("start").set_value(1);
    //scene.variable<FreeVariable>("start").set_value(1);
        
    scene.set_reward(0);
    scene.clear_objects();
    scene.clear_triggers();

    vector<string> boxes = {"box1", "box2", "box3"};

    float long_side(cur_step.long_side), short_side(cur_step.short_side);
    
    float x(0);
    float y(.9);
    float z(-.05);
    
    float box_width(cur_step.orientation == Horizontal ? long_side : short_side);
    float box_height(cur_step.orientation == Vertical ? long_side : short_side);
    float box_depth(cur_step.orientation == Laying ? long_side : short_side);
    
    scene.add_box("box1");
    scene.set_pos("box1", Pos(x - cur_step.xdir, y - cur_step.ydir, z - cur_step.zdir));
    scene.find<Box>("box1").set_dim(box_width, box_height, box_depth);
    scene.find<Box>("box1").set_texture("white-checker.png");
    
    scene.add_box("box2");
    scene.set_pos("box2", Pos(x, y, z));
    scene.find<Box>("box2").set_dim(box_width, box_height, box_depth);
    scene.find<Box>("box2").set_texture("white-checker.png");
    
    scene.add_box("box3");
    scene.set_pos("box3", Pos(x + cur_step.xdir, y + cur_step.ydir, z + cur_step.zdir));
    scene.find<Box>("box3").set_dim(box_width, box_height, box_depth);
    scene.find<Box>("box3").set_texture("white-checker.png");

    scene.find<Box>(boxes[choice]).set_texture("blue-checker.png");        
    scene.add_trigger(new InBoxTrigger(scene(boxes[choice]), scene("controller")), "on_in_box");
    
    scene.set_reward(0);
    scene.start_recording();
    
  }
};

void test() {
  Global::scene().clear();
}

int record(string filename) {
  if (exists(filename))
    throw StringException("file already exists");

  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  
  
  //preloading images
  ImageFlywheel::preload();
  //ImageFlywheel::image("stub.png");
  //ImageFlywheel::image("gray.png");
  //ImageFlywheel::image("blue.png");
  //ImageFlywheel::image("red.png");

  auto &scene = Global::scene();
  FittsWorld world(scene);
  vk.end_submit_cmd();
  
  
  Timer a_timer(1./90);
  uint i(0);
  Recording recording;
  while (!scene.stop) {
    //cout << i << endl;
    vr.update_track_pose();
    scene.step();
    scene.snap(&recording);

    vr.render(scene);
    vr.wait_frame();

    //cout << "elapsed: " << a_timer.elapsed() << endl;
    //if (a_timer.elapsed() > 60.) {
      // recording.save(filename, scene);
    // a_timer.start();
    // }
    //vr.request_poses();
    //a_timer.wait();
  }

  cout << "writing: " << endl;
  recording.save(filename, scene);
  cout << "done: " << endl;
  recording.release();
  Global::shutdown();
}

int replay(string filename) {
  if (!exists(filename))
    throw StringException("file doesnt exist");
      
  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  //preloading images
  ImageFlywheel::preload();
  //ImageFlywheel::image("stub.png");
  //ImageFlywheel::image("gray.png");
  //ImageFlywheel::image("blue.png");
  //ImageFlywheel::image("red.png");
  //ImageFlywheel::image("white-checker.png");
  //ImageFlywheel::image("blue-checker.png");
  //ImageFlywheel::image("red-checker.png");

  auto &scene = Global::scene();
  FittsWorld world(scene);
  vk.end_submit_cmd();
  

  
  Timer a_timer(1./90);
  uint i(0);
  Recording recording;
  recording.load(filename, &scene);
  cout << "recording size: " << recording.size() << endl;
  /*
  for (auto o : scene.objects)
    cout << o.first << " " << scene.names[o.second->nameid] << endl;

  for (auto v : scene.variables)
    cout << v.first << " " << v.second->val << " " << scene.names[v.second->nameid] << endl;
  for (auto t : scene.triggers)
    cout << scene.names[t->function_nameid] << endl;
  */                                           
  while (i < recording.size()) {
    //cout << i << endl;
    //vr.update_track_pose();
    //scene.step();
    //scene.snap(&recording);

    recording.load_scene(i, &scene);
    vr.hmd_pose = Matrix4(scene.find<HMD>("hmd").to_mat4());
    cout << "scene " << i << " items: " << scene.objects.size() << endl;
    vr.render(scene);
    vr.wait_frame();
    ++i;
    //vr.request_poses();
    //a_timer.wait();
  }

  Global::shutdown();
}

int learn(string filename) {
  if (!exists(filename))
    throw StringException("file doesnt exist");

  
  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  //preloading images
  ImageFlywheel::preload();
  //ImageFlywheel::image("stub.png");
  //ImageFlywheel::image("gray.png");
  //ImageFlywheel::image("blue.png");
  //ImageFlywheel::image("red.png");
  //ImageFlywheel::image("white-checker.png");
  //ImageFlywheel::image("blue-checker.png");
  //ImageFlywheel::image("red-checker.png");

  auto &scene = Global::scene();
  FittsWorld world(scene);
  vk.end_submit_cmd();
  
  Timer a_timer(1./90);
  Recording recording;
  recording.load(filename, &scene);
  cout << "recording size: " << recording.size() << endl;

  int n = 100;
  int c = 3 * 2; //stereo rgb
  int h = 32;
  VolumeShape img_input{n, c, width, height};
  VolumeShape network_output{n, h, 1, 1};

  VolumeNetwork net(img_input);
  net.add_pool(2, 2);
  net.add_univlstm(7, 7, 16);
  net.add_pool(2, 2);
  net.add_univlstm(7, 7, 16);
  //output should be {z, c, 1, 1}

  while (true) {
    int b = rand() % (recording.size() - n + 1);
    int e = b + n;
    for (int i(b); i < e; ++i) {
      recording.load_scene(i, &scene);
      vr.hmd_pose = Matrix4(scene.find<HMD>("hmd").to_mat4());
      cout << "scene " << i << " items: " << scene.objects.size() << endl;
      vr.render(scene);
      vr.copy_image_to_cpu();
      vr.wait_frame();
    }
  }
  

  Global::shutdown();
}

int analyse(string filename) {
  if (!exists(filename))
    throw StringException("file doesnt exist");
      
  auto &scene = Global::scene();
  FittsWorld world(scene);
  
  Timer a_timer(1./90);
  uint i(0);
  Recording recording;
  recording.load(filename, &scene);
  cout << "recording size: " << recording.size() << endl;
  /*
  for (auto o : scene.objects)
    cout << o.first << " " << scene.names[o.second->nameid] << endl;

  for (auto v : scene.variables)
    cout << v.first << " " << v.second->val << " " << scene.names[v.second->nameid] << endl;
  for (auto t : scene.triggers)
    cout << scene.names[t->function_nameid] << endl;
  */                                           
  int clicked(0);
  Pos start_pos;
  int start_frame(0);
  int start_clicks(0);

  ofstream datafile(filename + ".data");
  datafile << "ORIENTATION WIDTH XDIR YDIR ZDIR STARTX STARTY STARTZ ENDX ENDY ENDZ NFRAMES" << endl;
  while (i < recording.size()) {
    //cout << i << endl;
    //vr.update_track_pose();
    //scene.step();
    //scene.snap(&recording);

    
    recording.load_scene(i, &scene);
    
    try {
      if (scene.find<Controller>("controller").clicked)
        clicked++;

      if (scene.variable<MarkVariable>("start").val > 0) {
        start_frame = i;
        start_clicks = clicked;
        start_pos = scene.find<Controller>("controller").p;
      }
      
      if (scene.variable<MarkVariable>("end").val > 0) {
        Pos cur_pos = scene.find<Controller>("controller").p;
        datafile << scene.variable<FreeVariable>("orientation").val << " "
             << scene.variable<FreeVariable>("short_side").val << " "
             << scene.variable<FreeVariable>("xdir").val << " "
             << scene.variable<FreeVariable>("ydir").val << " " 
             << scene.variable<FreeVariable>("zdir").val << " "
             << start_pos.x << " "
             << start_pos.y << " "
             << start_pos.z << " "
             << cur_pos.x << " "
             << cur_pos.y << " "
             << cur_pos.z << " "
             << (i - start_frame) << endl;
      }
    } catch(...) {}
    
    //cout << "scene " << i << " items: " << scene.objects.size() << endl;
    ++i;
    //vr.request_poses();
    //a_timer.wait();
  }
  cout << "n clicks: " << clicked << endl;

  Global::shutdown();
}

int main(int argc, char **argv) {
  vector<string> args;
  for (int i(1); i < argc; ++i)
    args.push_back(argv[i]);

  if (args.size() < 2)
    throw StringException("not enough args, use: record|replay filename");
  if (args[0] == "record")
    record(args[1]);
  if (args[0] == "replay")
    replay(args[1]);
  if (args[0] == "analyse")
    analyse(args[1]);
  if (args[0] == "learn")
    learn(args[1]);
}
