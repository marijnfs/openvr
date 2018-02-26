#include <vector>
#include <iostream>
#include <map>
#include <string>
#include <cmath>
#include <sstream>
#include <chrono>
#include <thread>

using namespace std;

#include "scene.h"
#include "db.h"
#include "serialise.h"

struct World {
  Scene &scene;


  
  void init() {
    //setup world
    scene.add_object("hmd", new HMD());
    scene.add_object("controller", new Controller(true));

    scene.register_function("onstart", std::bind(&World::on_start, *this));
    scene.register_function("onwin", std::bind(&World::on_win, *this));
    scene.add_variable("dist", new DistanceVariable(scene("target"), scene("controller")));
    scene.add_trigger(new ClickTrigger(), "onstart");
  }

  void add_points(int choice) {
    scene.add_screen("screen1");
    scene.add_screen("screen2");
    scene.add_screen("screen3");

    scene.set_texture("screen1", "circle");
    scene.set_texture("screen2", "circle");
    scene.set_texture("screen3", "circle");
     
    scene.set_pos("screen1", Pos{0, -1, -1});
    scene.set_pos("screen2", Pos{0, -1, 0});
    scene.set_pos("screen3", Pos{0, -1, 1});

    scene.add_point("target");
    switch(choice) {
    case 0: scene.set_pos("target", Pos{0, -1, -1});
      scene.set_texture("screen1", "cross");
      break;
    case 1: scene.set_pos("target", Pos{0, -1, 0}); break;
      scene.set_texture("screen2", "cross");
      break;
    case 2: scene.set_pos("target", Pos{0, -1, 1}); break;
      scene.set_texture("screen3", "cross");
      break;
    }

  }

  void on_win() {
    scene.set_reward(1);
    scene.end_recording();
    scene.add_trigger(new ClickTrigger(), "onstart");
  }
  
  void on_start() {
    scene.clear();

    int choice = rand() % 3;
    add_points(choice);

    
    scene.add_trigger(new LimitTrigger(scene("dist"), .1), "onwin");
    scene.set_reward(0);
    scene.start_recording();
    
  }
};


int main() {
  Scene scene;

  
  cout << "start" << endl;

  Pos p1{0, 0, 1};
  Pos p2{0, 0, 1};
  Pos p3{0, 0, 1};

  
  
  scene.add_screen("o1");
  scene.add_screen("o2");
  scene.add_screen("o3");

  scene.set_pos("o1", p1);
  scene.set_pos("o2", p2);
  scene.set_pos("o3", p3);

  int goal = rand() % 3;

  
  
  while (scene.time < 2000) {
    
    scene.step();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
      
  }

  cout << "end" << endl;  
    
}
