#ifndef __VRSYSTEM_H__
#define __VRSYSTEM_H__

#include <vector>
#include <string>
#include <openvr.h>

#include "shared/Matrices.h"
#include "buffer.h"

struct VRSystem {
  vr::IVRSystem *hmd;
  vr::IVRRenderModels *render_models;
  std::string driver_str, display_str;

  vr::TrackedDevicePose_t tracked_pose[ vr::k_unMaxTrackedDeviceCount ];
  Matrix4 tracked_pose_mat4[ vr::k_unMaxTrackedDeviceCount ];
  vr::TrackedDeviceClass device_class[ vr::k_unMaxTrackedDeviceCount ];

  Matrix4 hmd_pose;
  Matrix4 eye_pos_left, eye_pos_right, eye_pose_center;
  Matrix4 projection_left, projection_right;

  FrameRenderBuffer left_eye_fb, right_eye_fb;

  std::vector<Buffer> eye_pos_buffer;

  uint32_t render_width, render_height;
  float near_clip, far_clip;

  VRSystem();

  void render_stereo_targets() ;

  Matrix4 get_eye_transform( vr::Hmd_Eye eye );

  Matrix4 get_hmd_projection( vr::Hmd_Eye eye );


  Matrix4 get_view_projection( vr::Hmd_Eye eye );

  void render_scene();

  void setup_render_models();

  void setup_render_model_for_device(int d);

  void update_track_pose();

  std::string query_str(vr::TrackedDeviceIndex_t devidx, vr::TrackedDeviceProperty prop);

  std::vector<std::string> get_inst_ext_required();

  std::vector<std::string> get_dev_ext_required();

  std::vector<std::string> get_inst_ext_required_verified();

  std::vector<std::string> get_dev_ext_required_verified();

  ~VRSystem();

  void setup_render_targets();
};

#endif