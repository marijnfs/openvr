#include "global.cc"

using namespace std;


Global::Global() {
  vk_ptr = new VulkanSystem();
  vr_ptr = new VRSystem();
  ws_ptr = new WindowSystem();
}