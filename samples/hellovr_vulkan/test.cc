#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

std::ostream &operator<<(std::ostream &out, glm::mat4 m) {
  return out << m[0][0] << " " << m[1][0] << " " << m[2][0] << " " << m[3][0] << endl
             << m[0][1] << " " << m[1][1] << " " << m[2][1] << " " << m[3][1] << endl
             << m[0][2] << " " << m[1][2] << " " << m[2][2] << " " << m[3][2] << endl
             << m[0][3] << " " << m[1][3] << " " << m[2][3] << " " << m[3][3] << endl;
}

int main() {
  glm::fmat4 m(1.);
  
  m = glm::translate(m, glm::vec3(2., -1, .3));
  m = glm::rotate(m, float(.5), glm::fvec3(1.,1.,1.));

  auto ptr = (float*)glm::value_ptr(m);

  glm::quat quat = glm::quat_cast(m);
  
  glm::vec3 pos;
  pos[0] = ptr[12];
  pos[1] = ptr[13];
  pos[2] = ptr[14];

  //auto m2 = glm::mat4_cast(quat);
  auto m2 = glm::mat4_cast(quat);
  m2[3][0] = pos[0];
  m2[3][1] = pos[1];
  m2[3][2] = pos[2];
  
  cout << "test " << m << endl;
  cout << m2 << endl;
  cout << pos[0] << " " << pos[1] << " " << pos[2] << endl;
}
