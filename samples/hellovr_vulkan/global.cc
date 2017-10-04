#include "global.h"

using namespace std;


Global::Global() {
  ws_ptr = new WindowSystem();
  vk_ptr = new VulkanSystem();
  vr_ptr = new VRSystem();
}
