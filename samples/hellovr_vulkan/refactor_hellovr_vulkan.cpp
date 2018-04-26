//========= Copyright Valve Corporation ============//

#include <string>
#include <cstdlib>
#include <inttypes.h>
#include <openvr.h>
#include <deque>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "shared/lodepng.h"
#include "shared/Matrices.h"
#include "shared/pathtools.h"

#include "img.h"
#include "util.h"
#include "utilvr.h"
#include "global.h"
#include "vulkansystem.h"
#include "vrsystem.h"
#include "windowsystem.h"
#include "flywheel.h"

#include "learningsystem.h"
#include "volumenetwork.h"
#include "network.h"
#include "trainer.h"

using namespace std;

enum Orientation {
  Horizontal = 0,
  Vertical = 1,
  Laying = 2
};

struct ExperimentStep {
  Orientation orientation = Horizontal;
  float xdir = 0;
  float ydir = 0;
  float zdir = 0;
  int n_clicks = 0;

  float long_side = 0;
  float short_side = 0;

  ExperimentStep(){}
  ExperimentStep(Orientation o, float xdir_, float ydir_, float zdir_, int n_clicks_, float long_side_, float short_side_) :
    orientation(o), xdir(xdir_), ydir(ydir_), zdir(zdir_), n_clicks(n_clicks_),
    long_side(long_side_), short_side(short_side_)
  {}
};

struct FittsWorld {
  Scene &scene;
  int click = 0;
  int step = 0;
  int choice = 0;
  
  vector<ExperimentStep> steps;
  
  

  FittsWorld(Scene &scene_) : scene(scene_) {
    srand(123123);
    init_experiments();
    init();
  }

 
  void init_experiments() {
    //ExperimentStep(Orientation o, float xdir_, float ydir_, float zdir_, int n_clicks_, float long_side_, float short_side_) :
    vector<float> multipliers = {1.0, 2.0, 4.0};
    int n_clicks(10);
    float long_side(.2);
    float short_side(.01);
    float dist(.02);
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Vertical, dist * m + short_side * m2, 0, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Vertical, 0., 0, dist * m + short_side * m2, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Vertical, dist * m + short_side * m2, 0, dist * m + short_side * m2, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Vertical, dist * m + short_side * m2, 0, -dist * m - short_side * m2, n_clicks, long_side, short_side * m2));
    
    ////Horizontal
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Horizontal, 0, dist * m + short_side * m2, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Horizontal, 0., 0, dist * m + short_side * m2, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Horizontal, 0, dist * m + short_side * m2, dist * m + short_side * m2, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
      steps.push_back(ExperimentStep(Horizontal, 0, dist * m + short_side * m2, -dist * m - short_side * m2, n_clicks, long_side, short_side * m2));
    
    //Laying
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Laying, dist * m + short_side * m2, 0, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Laying, 0., dist * m + short_side * m2, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
        steps.push_back(ExperimentStep(Laying, dist * m + short_side * m2, dist * m + short_side * m2, 0, n_clicks, long_side, short_side * m2));
    
    for (auto m : multipliers)
      for (auto m2 : multipliers)
      steps.push_back(ExperimentStep(Laying, dist * m + short_side * m2, -dist * m - short_side * m2, 0, n_clicks, long_side, short_side * m2));
    random_shuffle(steps.begin(), steps.end());
    
    steps.resize(steps.size() / 2);///TODO
  }
    
  void init() {
    cout << "Fitts World INIT" << endl;
    //scene.add_canvas("test");
    scene.add_hmd();
    scene.add_object("controller", new Controller(true));
    //scene.set_pos("test", Pos(1, 1, 1));

    scene.add_variable("step", new FreeVariable());
    scene.add_variable("click", new FreeVariable());

    scene.add_variable("orientation", new FreeVariable());
    scene.add_variable("xdir", new FreeVariable());
    scene.add_variable("ydir", new FreeVariable());
    scene.add_variable("zdir", new FreeVariable());

    scene.add_variable("long_side", new FreeVariable());
    scene.add_variable("short_side", new FreeVariable());
    
    scene.add_variable("choice", new FreeVariable());

    scene.add_variable("start", new MarkVariable());
    scene.add_variable("end", new MarkVariable());
    
    scene.register_function("on_in_box", std::bind(&FittsWorld::on_in_box, *this));
    scene.register_function("on_start", std::bind(&FittsWorld::on_start, *this));
    scene.add_trigger(new ClickTrigger(scene("controller")), "on_start");

    scene.add_box("startbox");
    scene.set_pos("startbox", Pos(0, .9, -.05));
    scene.find<Box>("startbox").set_dim(.05, .05, .05);
    scene.find<Box>("startbox").set_texture("white-checker.png");

    cout << "Fitts World Done INIT" << endl;
  }

  void on_in_box() {
    //cout << "ON IN BOX" << endl;

    if (scene.find<Controller>("controller").clicked == true) {
      scene.clear_scene();
      scene.set_reward(1);

      scene.add_trigger(new NextTrigger(), "on_start");
      scene.variable<MarkVariable>("end").set_value(1);
      //scene.end_recording();
      //scene.clear_objects();
      //scene.clear_triggers();
      //scene.add_trigger(new ClickTrigger(), "on_start");
    }
  }
  
  void on_start() {
    click++;
    if (click >= steps[step].n_clicks) {
      step++;
      cout << step << "/" << steps.size() << endl;
      click = 0;
    }
    if (step >= steps.size()) {
      scene.stop = true;
      return;
    }

    ExperimentStep &cur_step(steps[step]);

    int new_choice = rand() % 3;
    while (new_choice == choice)
      new_choice = rand() % 3;
    choice = new_choice;

    //store experiment vars;
    scene.variable<FreeVariable>("step").set_value(step);
    scene.variable<FreeVariable>("click").set_value(click);
    scene.variable<FreeVariable>("orientation").set_value(cur_step.orientation);

    scene.variable<FreeVariable>("long_side").set_value(cur_step.long_side);
    scene.variable<FreeVariable>("short_side").set_value(cur_step.short_side);

    scene.variable<FreeVariable>("xdir").set_value(cur_step.xdir);
    scene.variable<FreeVariable>("ydir").set_value(cur_step.ydir);
    scene.variable<FreeVariable>("zdir").set_value(cur_step.zdir);

    scene.variable<FreeVariable>("choice").set_value(choice);
    scene.variable<MarkVariable>("start").set_value(1);
    //scene.variable<FreeVariable>("start").set_value(1);
        
    scene.set_reward(0);
    scene.clear_objects();
    scene.clear_triggers();

    vector<string> boxes = {"box1", "box2", "box3"};

    float long_side(cur_step.long_side), short_side(cur_step.short_side);
    
    float x(0);
    float y(.9);
    float z(-.05);
    
    float box_width(cur_step.orientation == Horizontal ? long_side : short_side);
    float box_height(cur_step.orientation == Vertical ? long_side : short_side);
    float box_depth(cur_step.orientation == Laying ? long_side : short_side);
    
    scene.add_box("box1");
    scene.set_pos("box1", Pos(x - cur_step.xdir, y - cur_step.ydir, z - cur_step.zdir));
    scene.find<Box>("box1").set_dim(box_width, box_height, box_depth);
    scene.find<Box>("box1").set_texture("white-checker.png");
    
    scene.add_box("box2");
    scene.set_pos("box2", Pos(x, y, z));
    scene.find<Box>("box2").set_dim(box_width, box_height, box_depth);
    scene.find<Box>("box2").set_texture("white-checker.png");
    
    scene.add_box("box3");
    scene.set_pos("box3", Pos(x + cur_step.xdir, y + cur_step.ydir, z + cur_step.zdir));
    scene.find<Box>("box3").set_dim(box_width, box_height, box_depth);
    scene.find<Box>("box3").set_texture("white-checker.png");

    scene.find<Box>(boxes[choice]).set_texture("blue-checker.png");        
    scene.add_trigger(new InBoxTrigger(scene(boxes[choice]), scene("controller")), "on_in_box");
    
    scene.set_reward(0);
    scene.start_recording();
    
  }
};

void test() {
  Global::scene().clear();
}

int record(string filename) {
  if (exists(filename))
    throw StringException("file already exists");

  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  
  
  //preloading images
  ImageFlywheel::preload();
  //ImageFlywheel::image("stub.png");
  //ImageFlywheel::image("gray.png");
  //ImageFlywheel::image("blue.png");
  //ImageFlywheel::image("red.png");

  auto &scene = Global::scene();

  //FittsWorld world(scene);
  Script world_script;
  world_script.run("fittsworld.lua");
  vk.end_submit_cmd();
  
  
  Timer a_timer(1./90);
  uint i(0);
  Recording recording;
  while (!scene.stop) {
    //cout << i << endl;
    vr.update_track_pose();
    scene.step();
    scene.snap(&recording);

    vr.render(scene);
    vr.wait_frame();

    //cout << "elapsed: " << a_timer.elapsed() << endl;
    //if (a_timer.elapsed() > 60.) {
      // recording.save(filename, scene);
    // a_timer.start();
    // }
    //vr.request_poses();
    //a_timer.wait();
  }

  cout << "writing: " << endl;
  recording.save(filename, scene);
  cout << "done: " << endl;
  recording.release();
  Global::shutdown();
}

int replay(string filename) {
  if (!exists(filename))
    throw StringException("file doesnt exist");
      
  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  //preloading images
  ImageFlywheel::preload();
  //ImageFlywheel::image("stub.png");
  //ImageFlywheel::image("gray.png");
  //ImageFlywheel::image("blue.png");
  //ImageFlywheel::image("red.png");
  //ImageFlywheel::image("white-checker.png");
  //ImageFlywheel::image("blue-checker.png");
  //ImageFlywheel::image("red-checker.png");

  auto &scene = Global::scene();
  FittsWorld world(scene);
  vk.end_submit_cmd();
  

  
  Timer a_timer(1./90);
  uint i(0);
  Recording recording;
  recording.load(filename, &scene);
  cout << "recording size: " << recording.size() << endl;
  /*
  for (auto o : scene.objects)
    cout << o.first << " " << scene.names[o.second->nameid] << endl;

  for (auto v : scene.variables)
    cout << v.first << " " << v.second->val << " " << scene.names[v.second->nameid] << endl;
  for (auto t : scene.triggers)
    cout << scene.names[t->function_nameid] << endl;
  */                                           
  while (i < recording.size()) {
    //cout << i << endl;
    //vr.update_track_pose();
    //scene.step();
    //scene.snap(&recording);

    recording.load_scene(i, &scene);
    vr.hmd_pose = Matrix4(scene.find<HMD>("hmd").to_mat4());
    cout << "scene " << i << " items: " << scene.objects.size() << endl;
    vr.render(scene);
    vr.wait_frame();
    ++i;
    //vr.request_poses();
    //a_timer.wait();
  }

  Global::shutdown();
}

int learn(string filename) {
  if (!exists(filename))
    throw StringException("file doesnt exist");

  Handler::cudnn();
  Handler::set_device(0);
  
  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  //preloading images
  ImageFlywheel::preload();

  auto &scene = Global::scene();
  FittsWorld world(scene);
  vk.end_submit_cmd();
  
  Timer a_timer(1./90);
  Recording recording;
  recording.load(filename, &scene);
  cout << "recording size: " << recording.size() << endl;

  
  int N = 16;
  int c = 3 * 2; //stereo rgb
  int h = 32;

  int act_dim = 13;
  int obs_dim = 6;
  int vis_dim = 64;
  int aggr_dim = 32;
  
  int width = VIVE_WIDTH;
  int height = VIVE_HEIGHT;
  VolumeShape img_input{N, c, width, height};
  VolumeShape network_output{N, h, 1, 1};


  //Visual processing network, most cpu intensive
  bool first_grad = false;
  VolumeNetwork vis_net(img_input, first_grad);
 
  auto base_pool = new PoolingOperation<F>(2, 2);
  auto conv1 = new ConvolutionOperation<F>(vis_net.output_shape().c, 4, 5, 5);
  auto pool1 = new PoolingOperation<F>(4, 4);
  vis_net.add_slicewise(base_pool);
  vis_net.add_slicewise(conv1);
  vis_net.add_slicewise(pool1);
  
  auto conv2 = new ConvolutionOperation<F>(vis_net.output_shape().c, 8, 5, 5);
  auto pool2 = new PoolingOperation<F>(4, 4);
  vis_net.add_slicewise(conv2);
  vis_net.add_slicewise(pool2);
  
  auto squash = new SquashOperation<F>(vis_net.output_shape().tensor_shape(), vis_dim);
 
  vis_net.add_slicewise(squash);
  vis_net.add_tanh();
  //output should be {z, c, 1, 1}
  vis_net.finish();

  //Aggregation Network combining output of visual processing network and state vector
  VolumeShape aggr_input{N, vis_dim + obs_dim, 1, 1};
  VolumeNetwork aggr_net(aggr_input);
  aggr_net.add_univlstm(1, 1, 32);
  aggr_net.add_univlstm(1, 1, aggr_dim);
  aggr_net.finish();
  
  TensorShape actor_in{N, aggr_dim, 1, 1};
  Network<F> actor_net(actor_in);
  actor_net.add_conv(64, 1, 1);
  actor_net.add_relu();
  actor_net.add_conv(64, 1, 1);
  actor_net.add_relu();
  actor_net.add_conv(act_dim, 1, 1);

  actor_net.finish();
  
  TensorShape value_in{N, aggr_dim, 1, 1};
  Network<F> value_net(actor_in);
  value_net.add_conv(64, 1, 1);
  value_net.add_relu();
  value_net.add_conv(64, 1, 1);
  value_net.add_relu();
  value_net.add_conv(1, 1, 1);
  value_net.finish();
  
  VolumeShape q_in{N, aggr_dim + act_dim, 1, 1}; //qnet, could be also advantage
  VolumeNetwork q_net(q_in);
  q_net.add_univlstm(1, 1, 32);
  q_net.add_univlstm(1, 1, 32);
  q_net.add_fc(1);
  q_net.finish();

  //initialisation
  float std(.03);

  vis_net.init_uniform(std);
  aggr_net.init_uniform(std);
  q_net.init_uniform(std);
  actor_net.init_uniform(std);
  value_net.init_uniform(std);

  Trainer vis_trainer(vis_net.param_vec.n, .005, .0000001, 400);
  Trainer aggr_trainer(aggr_net.param_vec.n, .002, .0000001, 400);
  Trainer actor_trainer(actor_net.param_vec.n, .001, .0000001, 400);
  
  //Volume target_actions(VolumeShape{N, act_dim, 1, 1});
  Tensor<F> action_targets_tensor(N, act_dim, 1, 1);
  vector<float> action_targets(N * act_dim);

  //load if applicable
  if (exists("vis.net"))
    vis_net.load("vis.net");
  if (exists("aggr.net"))
    aggr_net.load("aggr.net");
  if (exists("q.net"))
    q_net.load("q.net");
  if (exists("actor.net"))
    actor_net.load("actor.net");
  if (exists("value.net"))
    value_net.load("value.net");
  
  
  int epoch(0);
  while (true) {
    int b = rand() % (recording.size() - N + 1);
    int e = b + N;
    Pose cur_pose, last_pose;

    //First setup all inputs for an episode
    int t(0);

    std::vector<float> nimg(3 * 2 * VIVE_WIDTH * VIVE_HEIGHT);
    
    for (int i(b); i < e; ++i, ++t) {
      recording.load_scene(i, &scene);
      
      vr.hmd_pose = Matrix4(scene.find<HMD>("hmd").to_mat4());
      cout << "scene " << i << " items: " << scene.objects.size() << endl;
      bool headless(true);

      //
      ////vr.render(scene, &img);
      ////vr.render(scene, headless);
      vr.render(scene, false, &nimg);
      vr.wait_frame();
      //std::vector<float> nimg(img.begin(), img.end());
      //normalize_mt(&img);
      //cout << nimg.size() << endl;

      //write_img1c("bla2.png", VIVE_WIDTH, VIVE_HEIGHT, &nimg[0]);
      copy_cpu_to_gpu<float>(&nimg[0], vis_net.input().slice(t), nimg.size());
      
      //vis_net.input().draw_slice("bla.png", t, 0);
      //normalise_fast(vis_net.input().slice(t), nimg.size());

      last_pose = cur_pose;
      cur_pose.from_scene(scene);
      if (t == 0) last_pose = cur_pose;
      
      Action action(last_pose, cur_pose);
      
      auto a_vec = action.to_vector();
      auto o_vec = cur_pose.to_obs_vector();
      
      cout << "action: " << a_vec << " " << o_vec << endl;
      //if (t) //ignore first acti3~on
      copy(a_vec.begin(), a_vec.end(), action_targets.begin() + act_dim * t);
      copy_cpu_to_gpu(&o_vec[0], aggr_net.input().data(t, 0), o_vec.size());
    }

    action_targets_tensor.from_vector(action_targets);
    //run visual network
    vis_net.forward();

    cout << "vis output: " << vis_net.output().to_vector() << endl;
    //aggregator
    for (int t(0); t < N; ++t)
      //aggregator already has observation data, we add vis output after that
      copy_gpu_to_gpu(vis_net.output().data(t), aggr_net.input().data(t, obs_dim), vis_dim);
    aggr_net.forward();
    cout << "agg output: " << aggr_net.output().to_vector() << endl;
    
    //copy aggr to nets
    copy_gpu_to_gpu(aggr_net.output().data(), actor_net.input().data, aggr_net.output().size());
    copy_gpu_to_gpu(aggr_net.output().data(), value_net.input().data, aggr_net.output().size());
    
    actor_net.forward();
    value_net.forward(); //not for imitation

    
    //copy actions and aggr to q  //not for imitation //HERE BE SEGFAULT?
    /*for (int t(0); t < N - 1; ++t) {
      copy_gpu_to_gpu(actor_net.output().ptr(t), q_net.input().data(t + 1, 0), act_dim);
      copy_gpu_to_gpu(aggr_net.output().data(t), q_net.input().data(t, act_dim), aggr_dim);
    }
    q_net.forward();     //not for imitation
    */

    //====== Imitation backward:
    
    actor_net.calculate_loss(action_targets_tensor);
    //cout << "actor action: " << actor_net.output().to_vector() << endl;
    cout << "actor loss: " << actor_net.loss() << endl;
    actor_net.backward();
    
    copy_gpu_to_gpu(actor_net.input_grad().data, aggr_net.output_grad().data(), actor_net.input_grad().size());
    aggr_net.backward();
    
    for (int t(0); t < N; ++t)
      copy_gpu_to_gpu(aggr_net.input_grad().data(t, obs_dim), vis_net.output_grad().data(t), vis_dim);
    vis_net.backward();

    vis_trainer.update(&vis_net.param_vec, vis_net.grad_vec);
    aggr_trainer.update(&aggr_net.param_vec, aggr_net.grad_vec);
    actor_trainer.update(&actor_net.param_vec, actor_net.grad_vec);
    //set action targets
    //run backward
    
    //====== Qlearning backward:
    //run forward
    //calculate q targets
    //run backward q -> calculate action update
    //run backward rest
    if (epoch % 10 == 0) {
      vis_net.save("vis.net");
      aggr_net.save("aggr.net");
      q_net.save("q.net");
      actor_net.save("actor.net");
      value_net.save("value.net");
    }
      
    ++epoch;
  }
  
  Global::shutdown();
  return 0;
}

int rollout(string filename) {
  if (!exists(filename))
    throw StringException("file doesnt exist");

  Handler::cudnn();
  Handler::set_device(0);
  
  auto &ws = Global::ws();
  auto &vr = Global::vr();
  auto &vk = Global::vk();

  
  vr.setup();
  ws.setup();
  vk.setup();

  //preloading images
  ImageFlywheel::preload();

  auto &scene = Global::scene();
  FittsWorld world(scene);
  vk.end_submit_cmd();
  
  Timer a_timer(1./90);
  Recording recording;
  recording.load(filename, &scene);
  cout << "recording size: " << recording.size() << endl;

  int N = 15;
  int c = 3 * 2; //stereo rgb
  int h = 32;

  int act_dim = 13;
  int obs_dim = 6;
  int vis_dim = 64;
  int aggr_dim = 32;
  
  int width = VIVE_WIDTH;
  int height = VIVE_HEIGHT;
  VolumeShape img_input{N, c, width, height};
  VolumeShape network_output{N, h, 1, 1};


  //Visual processing network, most cpu intensive
  bool first_grad = false;
  VolumeNetwork vis_net(img_input, first_grad);
 
  auto base_pool = new PoolingOperation<F>(2, 2);
  auto conv1 = new ConvolutionOperation<F>(vis_net.output_shape().c, 4, 5, 5);
  auto pool1 = new PoolingOperation<F>(4, 4);
  vis_net.add_slicewise(base_pool);
  vis_net.add_slicewise(conv1);
  vis_net.add_slicewise(pool1);
  
  auto conv2 = new ConvolutionOperation<F>(vis_net.output_shape().c, 8, 5, 5);
  auto pool2 = new PoolingOperation<F>(4, 4);
  vis_net.add_slicewise(conv2);
  vis_net.add_slicewise(pool2);
  
  auto squash = new SquashOperation<F>(vis_net.output_shape().tensor_shape(), vis_dim);
 
  vis_net.add_slicewise(squash);
  vis_net.add_tanh();
  //output should be {z, c, 1, 1}
  vis_net.finish();

  //Aggregation Network combining output of visual processing network and state vector
  VolumeShape aggr_input{N, vis_dim + obs_dim, 1, 1};
  VolumeNetwork aggr_net(aggr_input);
  aggr_net.add_univlstm(1, 1, 32);
  aggr_net.add_univlstm(1, 1, aggr_dim);
  aggr_net.finish();
  
  TensorShape actor_in{N, aggr_dim, 1, 1};
  Network<F> actor_net(actor_in);
  actor_net.add_conv(64, 1, 1);
  actor_net.add_relu();
  actor_net.add_conv(64, 1, 1);
  actor_net.add_relu();
  actor_net.add_conv(act_dim, 1, 1);

  actor_net.finish();
  
  TensorShape value_in{N, aggr_dim, 1, 1};
  Network<F> value_net(actor_in);
  value_net.add_conv(64, 1, 1);
  value_net.add_relu();
  value_net.add_conv(64, 1, 1);
  value_net.add_relu();
  value_net.add_conv(1, 1, 1);
  value_net.finish();
  
  VolumeShape q_in{N, aggr_dim + act_dim, 1, 1}; //qnet, could be also advantage
  VolumeNetwork q_net(q_in);
  q_net.add_univlstm(1, 1, 32);
  q_net.add_univlstm(1, 1, 32);
  q_net.add_fc(1);
  q_net.finish();

  //initialisation
  float std(.1);

  vis_net.init_uniform(std);
  aggr_net.init_uniform(std);
  q_net.init_uniform(std);
  actor_net.init_uniform(std);
  value_net.init_uniform(std);

  Trainer vis_trainer(vis_net.param_vec.n, .005, .0000001, 400);
  Trainer aggr_trainer(aggr_net.param_vec.n, .002, .0000001, 400);
  Trainer actor_trainer(actor_net.param_vec.n, .001, .0000001, 400);
  
  //Volume target_actions(VolumeShape{N, act_dim, 1, 1});
  Tensor<F> action_targets_tensor(N, act_dim, 1, 1);
  vector<float> action_targets(N * act_dim);

  //load if applicable
  if (exists("vis.net"))
    vis_net.load("vis.net");
  if (exists("aggr.net"))
    aggr_net.load("aggr.net");
  if (exists("q.net"))
    q_net.load("q.net");
  if (exists("actor.net"))
    actor_net.load("actor.net");
  if (exists("value.net"))
    value_net.load("value.net");
  
  
  int epoch(0);
  while (true) {
    int b = rand() % (recording.size() - N + 1);
    int e = b + N;
    Pose cur_pose, last_pose;

    //First setup all inputs for an episode
    int t(0);

    std::vector<float> nimg(3 * 2 * VIVE_WIDTH * VIVE_HEIGHT);
    recording.load_scene(b, &scene);
    for (int i(b); i < e; ++i, ++t) {      
      vr.hmd_pose = Matrix4(scene.find<HMD>("hmd").to_mat4());
      cout << "scene " << i << " items: " << scene.objects.size() << endl;
      bool headless(true);

      //
      ////vr.render(scene, &img);
      ////vr.render(scene, headless);
      vr.render(scene, false, &nimg);
      vr.wait_frame();
      //std::vector<float> nimg(img.begin(), img.end());
      //normalize_mt(&img);
      //cout << nimg.size() << endl;

      //write_img1c("bla2.png", VIVE_WIDTH, VIVE_HEIGHT, &nimg[0]);
      copy_cpu_to_gpu<float>(&nimg[0], vis_net.input().slice(t), nimg.size());
      
      //vis_net.input().draw_slice("bla.png", t, 0);
      //normalise_fast(vis_net.input().slice(t), nimg.size());

      last_pose = cur_pose;
      cur_pose.from_scene(scene);
      //Action action(last_pose, cur_pose);
      
      auto o_vec = cur_pose.to_obs_vector();
      
      
      copy_cpu_to_gpu(&o_vec[0], aggr_net.input().data(t, 0), o_vec.size());

      action_targets_tensor.from_vector(action_targets);
      
      //run visual network
      vis_net.forward();
      copy_gpu_to_gpu(vis_net.output().data(t), aggr_net.input().data(t, obs_dim), vis_dim);
      aggr_net.forward();
      //cout << "agg output: " << aggr_net.output().to_vector() << endl;
      
      //copy aggr to nets
      copy_gpu_to_gpu(aggr_net.output().data(), actor_net.input().data, aggr_net.output().size());
      copy_gpu_to_gpu(aggr_net.output().data(), value_net.input().data, aggr_net.output().size());
      
      actor_net.forward();
      value_net.forward(); //not for imitation
      auto whole_action_vec = actor_net.output().to_vector();
      auto action_vec = vector<float>(whole_action_vec.begin() + t * act_dim, whole_action_vec.begin() + (t+1) * act_dim);
      Action act(action_vec);
      cur_pose.apply(act);
      cur_pose.apply_to_scene(scene);
    }
      
    ++epoch;
  }
  
  Global::shutdown();
  return 0;
}

int analyse(string filename) {
  if (!exists(filename))
    throw StringException("file doesnt exist");
      
  auto &scene = Global::scene();
  FittsWorld world(scene);
  
  Timer a_timer(1./90);
  uint i(0);
  Recording recording;
  recording.load(filename, &scene);
  cout << "recording size: " << recording.size() << endl;
  /*
  for (auto o : scene.objects)
    cout << o.first << " " << scene.names[o.second->nameid] << endl;

  for (auto v : scene.variables)
    cout << v.first << " " << v.second->val << " " << scene.names[v.second->nameid] << endl;
  for (auto t : scene.triggers)
    cout << scene.names[t->function_nameid] << endl;
  */                                           
  int clicked(0);
  Pos start_pos;
  int start_frame(0);
  int start_clicks(0);

  ofstream datafile(filename + ".data");
  datafile << "ORIENTATION WIDTH XDIR YDIR ZDIR STARTX STARTY STARTZ ENDX ENDY ENDZ NFRAMES" << endl;
  while (i < recording.size()) {
    //cout << i << endl;
    //vr.update_track_pose();
    //scene.step();
    //scene.snap(&recording);

    
    recording.load_scene(i, &scene);
    
    try {
      if (scene.find<Controller>("controller").clicked)
        clicked++;

      if (scene.variable<MarkVariable>("start").val > 0) {
        start_frame = i;
        start_clicks = clicked;
        start_pos = scene.find<Controller>("controller").p;
      }
      
      if (scene.variable<MarkVariable>("end").val > 0) {
        Pos cur_pos = scene.find<Controller>("controller").p;
        datafile << scene.variable<FreeVariable>("orientation").val << " "
             << scene.variable<FreeVariable>("short_side").val << " "
             << scene.variable<FreeVariable>("xdir").val << " "
             << scene.variable<FreeVariable>("ydir").val << " " 
             << scene.variable<FreeVariable>("zdir").val << " "
             << start_pos.x << " "
             << start_pos.y << " "
             << start_pos.z << " "
             << cur_pos.x << " "
             << cur_pos.y << " "
             << cur_pos.z << " "
             << (i - start_frame) << endl;
      }
    } catch(...) {}
    
    //cout << "scene " << i << " items: " << scene.objects.size() << endl;
    ++i;
    //vr.request_poses();
    //a_timer.wait();
  }
  datafile.flush();
  cout << "n clicks: " << clicked << endl;
  
  Global::shutdown();
}

int main(int argc, char **argv) {
  vector<string> args;
  for (int i(1); i < argc; ++i)
    args.push_back(argv[i]);

  if (args.size() < 2)
    throw StringException("not enough args, use: record|replay filename");
  if (args[0] == "record")
    return record(args[1]);
  if (args[0] == "replay")
    return replay(args[1]);
  if (args[0] == "analyse")
    return analyse(args[1]);
  if (args[0] == "learn")
    return learn(args[1]);
  if (args[0] == "rollout")
    return rollout(args[1]);
  throw StringException("Call right arguments");
}
