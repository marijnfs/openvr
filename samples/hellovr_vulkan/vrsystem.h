#ifndef __VRSYSTEM_H__
#define __VRSYSTEM_H__

#include <vector>
#include <string>
#include <openvr.h>

#include "shared/Matrices.h"
#include "buffer.h"
#include "vulkansystem.h"
#include "scene.h"

struct TrackedController {
  Matrix4 t;
  bool clicked = false;
  
    std::vector<float> get_pos();
  void set_t(Matrix4 &t);
};

struct VRSystem {
  vr::IVRSystem *ivrsystem;
  vr::IVRRenderModels *render_models;
  std::string driver_str, display_str;

  //tracking vars

  vr::TrackedDevicePose_t tracked_pose[ vr::k_unMaxTrackedDeviceCount ];
  Matrix4 tracked_pose_mat4[ vr::k_unMaxTrackedDeviceCount ];
  vr::TrackedDeviceClass device_class[ vr::k_unMaxTrackedDeviceCount ];

  //common matrices
  Matrix4 hmd_pose;
  Matrix4 eye_pos_left, eye_pos_right, eye_pose_center;
  Matrix4 projection_left, projection_right;

  //controllers;
  TrackedController left_controller, right_controller;
  
  //render targets
  FrameRenderBuffer left_eye_fb, right_eye_fb;
  
  Buffer left_eye_buf, right_eye_buf;
  void *left_eye_mvp, *right_eye_mvp;
  
  ////buffers
  //std::vector<Buffer> eye_pos_buffer;

  uint32_t render_width, render_height;
  float near_clip, far_clip;


  VRSystem();
  void init();

  Matrix4 get_eye_transform( vr::Hmd_Eye eye );
  Matrix4 get_hmd_projection( vr::Hmd_Eye eye );
  Matrix4 get_view_projection( vr::Hmd_Eye eye );
  void update_track_pose();

  void render(Scene &scene);
  void render_stereo_targets(Scene &scene);
  void render_companion_window();

  void setup_render_models();
  void setup_render_model_for_device(int d);
  void setup_render_targets();


  std::string query_str(vr::TrackedDeviceIndex_t devidx, vr::TrackedDeviceProperty prop);

  std::vector<std::string> get_inst_ext_required();
  std::vector<std::string> get_dev_ext_required();
  std::vector<std::string> get_inst_ext_required_verified();
  std::vector<std::string> get_dev_ext_required_verified();

  ~VRSystem();

};

#endif
