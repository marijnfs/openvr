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

using namespace std;


int main() {
  Global::inst();
  Global::ws();
  Global::vr();

  /*
  auto vr = Global::vr();

  //setup environment
  auto &ob = vr.create_object();
  Matrix4 pos;
  pos.translate(1, 1, 1);

  ob.init_cube(pos);


  while (true) {
  	// render and present

  	vr.render_frame();
  	
  	// grab action
  	auto action = vr.get_action();

  	// grab observation
  	auto observation = vr.get_observation();
  	// store action and observation

  	// step simulation
  	vr.step(action);
  }

  */ 

  //Global::vk();
  Global::vr();

  //Global::ws();
  while (true) {
  	cout << "lala" << endl;
  }
}
