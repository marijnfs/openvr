//========= Copyright Valve Corporation ============//

#include <string>
#include <cstdlib>
#include <inttypes.h>
#include <openvr.h>
#include <deque>
#include <iostream>
#include <fstream>

#include "shared/lodepng.h"
#include "shared/Matrices.h"
#include "shared/pathtools.h"

#include "util.h"
#include "global.h"
#include "vulkansystem.h"
#include "vrsystem.h"
#include "windowsystem.h"
#include "flywheel.h"

#include "learningsystem.h"

using namespace std;

struct FittsWorld {
  Scene &scene;
  int choice = -1;
  FittsWorld(Scene &scene_) : scene(scene_) {
    init();
  }
  

  void init() {
    cout << "Fitts World INIT" << endl;
    //scene.add_canvas("test");
    scene.add_hmd();
    scene.add_object("controller", new Controller(true));
    //scene.set_pos("test", Pos(1, 1, 1));
    
    scene.register_function("on_in_box", std::bind(&FittsWorld::on_in_box, *this));
    scene.register_function("on_start", std::bind(&FittsWorld::on_start, *this));
    scene.add_variable("mode", new FreeVariable());
    scene.variable<FreeVariable>("mode").set_value(1);
    scene.add_trigger(new ClickTrigger(scene("controller")), "on_start");
    cout << "Fitts World Done INIT" << endl;
  }

  void on_in_box() {
    cout << "ON IN BOX" << endl;

    if (scene.find<Controller>("controller").clicked == true) {
      scene.clear_scene();
      scene.set_reward(1);

      scene.add_trigger(new NextTrigger(), "on_start");

      //scene.end_recording();
      //scene.clear_objects();
      //scene.clear_triggers();
      //scene.add_trigger(new ClickTrigger(), "on_start");
    }
  }
  
  void on_start() {
    scene.set_reward(0);
    scene.clear_objects();
    scene.clear_triggers();

    vector<string> boxes = {"box1", "box2", "box3"};

    float x_seperation(.1);
    float distance(.05);
    float base_height(.9);
    float box_width_depth(.03);
    float box_height(.2);
    
    scene.add_box("box1");
    scene.set_pos("box1", Pos(x_seperation, base_height, -distance));
    scene.find<Box>("box1").set_dim(box_width_depth, box_height, box_width_depth);
    scene.find<Box>("box1").set_texture("white-checker.png");
    
    scene.add_box("box2");
    scene.set_pos("box2", Pos(0, base_height, -distance));
    scene.find<Box>("box2").set_dim(box_width_depth, box_height, box_width_depth);
    scene.find<Box>("box2").set_texture("white-checker.png");
    
    scene.add_box("box3");
    scene.set_pos("box3", Pos(-x_seperation, base_height, -distance));
    scene.find<Box>("box3").set_dim(box_width_depth, box_height, box_width_depth);
    scene.find<Box>("box3").set_texture("white-checker.png");

    cout << "done setting boxes" << endl;
    int new_choice = rand() % 3;
    while (new_choice == choice)
      new_choice = rand() % 3;
    choice = new_choice;
    
    scene.find<Box>(boxes[choice]).set_texture("blue-checker.png");
    scene.variable<FreeVariable>("mode").set_value(choice);

    scene.add_trigger(new InBoxTrigger(scene(boxes[choice]), scene("controller")), "on_in_box");
    
    scene.set_reward(0);
    scene.start_recording();
    
  }
};

void test() {
  Global::scene().clear();
}

int record() {
  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  //preloading images
  ImageFlywheel::image("stub.png");
  ImageFlywheel::image("gray.png");
  ImageFlywheel::image("blue.png");
  ImageFlywheel::image("red.png");

  auto &scene = Global::scene();
  FittsWorld world(scene);
  vk.end_submit_cmd();
  

  
  Timer a_timer(1./90);
  uint i(0);
  Recording recording;
  while (i++ < 10000) {
    //cout << i << endl;
    vr.update_track_pose();
    scene.step();
    scene.snap(&recording);

    vr.render(scene);
    vr.wait_frame();
    //vr.request_poses();
    //a_timer.wait();
  }

  cout << "writing: " << endl;
  recording.save("test.save", scene);
  cout << "done: " << endl;
  recording.release();
  Global::shutdown();
}

int replay() {
  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  //preloading images
  ImageFlywheel::image("stub.png");
  ImageFlywheel::image("gray.png");
  ImageFlywheel::image("blue.png");
  ImageFlywheel::image("red.png");
  ImageFlywheel::image("white-checker.png");
  ImageFlywheel::image("blue-checker.png");
  ImageFlywheel::image("red-checker.png");

  auto &scene = Global::scene();
  FittsWorld world(scene);
  vk.end_submit_cmd();
  

  
  Timer a_timer(1./90);
  uint i(0);
  Recording recording;
  recording.load("test.save", &scene);
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
    for (auto o : scene.objects)
      cout << o.first << endl;
    vr.hmd_pose = Matrix4(scene.find<HMD>("hmd").to_mat4());
    cout << "scene " << i << " item: " << scene.objects.size() << endl;
    vr.render(scene);
    vr.wait_frame();
    ++i;
    //vr.request_poses();
    //a_timer.wait();
  }

  Global::shutdown();
}

int main() {
  //record();
  replay();
}
