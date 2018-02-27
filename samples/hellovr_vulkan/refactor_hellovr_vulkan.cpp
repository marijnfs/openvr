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
  auto &scene = Global::scene();

  scene.add_canvas("test");
  scene.set_pos("test", vector<float>{1, 1, 1});

  Timer a_timer(1./60);
  uint i(0);
  while (i++ < 60) {
    //cout << i << endl;
    a_timer.wait();
  }
}
