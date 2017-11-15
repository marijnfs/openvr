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
  Global::init();
  /*
  auto vr = Global::vr();

  auto &ob = vr.create_object();
  Matrix4 pos;
  pos.translate(1, 1, 1);

  ob.init_cube(pos);


  while (true) {
  	vr.render_frame();
  }

  */ 

  Global::vk();
  Global::ws();
  while (true) ;
}
