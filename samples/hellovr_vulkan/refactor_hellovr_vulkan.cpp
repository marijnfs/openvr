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

  FittsWorld() : scene(Global::scene()) {}
  
  void init() {
    //setup world
    scene.add_object("hmd", new HMD());
    scene.add_object("controller", new Controller(true));

    scene.register_function("onstart", std::bind(&FittsWorld::on_start, *this));
    scene.register_function("onwin", std::bind(&FittsWorld::on_win, *this));
    //scene.add_variable("dist", new DistanceVariable(scene("target"), scene("controller")));
    scene.add_variable("mode", new FreeVariable());
    scene.add_trigger(new ClickTrigger(), "onstart");
  }

  void add_points(int choice) {
    scene.add_canvas("canvas1");
    scene.add_canvas("canvas2");
    scene.add_canvas("canvas3");

    scene.set_texture("canvas1", "circle");
    scene.set_texture("canvas2", "circle");
    scene.set_texture("canvas3", "circle");
     
    scene.set_pos("canvas1", Pos{0, -1, -1});
    scene.set_pos("canvas2", Pos{0, -1, 0});
    scene.set_pos("canvas3", Pos{0, -1, 1});

    scene.add_point("target");
    switch(choice) {
    case 0: scene.set_pos("target", Pos{0, -1, -1});
      scene.set_texture("canvas1", "cross");
      break;
    case 1: scene.set_pos("target", Pos{0, -1, 0}); break;
      scene.set_texture("canvas2", "cross");
      break;
    case 2: scene.set_pos("target", Pos{0, -1, 1}); break;
      scene.set_texture("canvas3", "cross");
      break;
    }

  }

  void on_box() {
    
    //check for click
    
  }
  
  void on_win() {
    scene.set_reward(1);
    scene.end_recording();
    scene.add_trigger(new ClickTrigger(), "onstart");
  }
  
  void on_start() {
    scene.clear();

    int choice = rand() % 3;
    scene.variable<FreeVariable>("mode").set_value(choice);
    add_points(choice);

    
    scene.add_trigger(new LimitTrigger(scene("dist"), .1), "onwin");
    scene.set_reward(0);
    scene.start_recording();
    
  }
};


int main() {


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
  vk.end_submit_cmd();
  
  auto &scene = Global::scene();
  //scene.add_canvas("test");
  scene.add_hmd();
  scene.add_object("controller", new Controller(true));
  //scene.set_pos("test", Pos(1, 1, 1));

  scene.add_box("box");
  scene.set_pos("box", Pos(.4, 0, -.4));
  scene.find<Box>("box").set_dim(.02, .2, .02);


  scene.add_box("box2");
  scene.set_pos("box2", Pos(0, 0, -.4));
  scene.find<Box>("box2").set_dim(.02, .2, .02);
  scene.find<Box>("box2").set_texture("blue.png");
  
  scene.add_box("box3");
  scene.set_pos("box3", Pos(-.4, 0, -.4));
  scene.find<Box>("box3").set_dim(.02, .2, .02);
  scene.find<Box>("box2").set_texture("gray.png");
    
  Timer a_timer(1./90);
  uint i(0);

  Recording recording;
  while (i++ < 1000) {
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
  recording.save("test.save");
  cout << "done: " << endl;
    
  glm::fvec3 v;
  glm::fquat q;
  q * v;

  Global::shutdown();
}
