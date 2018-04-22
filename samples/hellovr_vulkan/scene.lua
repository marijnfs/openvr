horizontal = 0
vertical = 1
laying = 2


function on_start()
   click = click + 1
   if click > n_clicks then
      click = 0
      step = step + 1
   end
   if step > #steps then
      stop()
   end

   local cur_step = steps[step]
   
   choice = choose(3, choice)
   set_variable("step", step)
   set_variable("click", click)
   set_variable("orientation", cur_step.orientation)
   set_variable("long_side", cur_step.long_side)
   set_variable("short_side", cur_step.short_side)
   set_variable("xdir", cur_step.xdir)
   set_variable("ydir", cur_step.ydir)
   set_variable("zdir", cur_step.zdir)
   set_variable("choice", choice)
   set_variable("start", 1)

   clear_objects()
   clear_triggers()

   local boxes = {"box1", "box2", "box3"}
   local l, s = cur_step.long_side, cur_step.short_side
   
   local bw = (cur_step.orientation == horizontal) and l or s
   local bh = (cur_step.orientation == vertical) and l or s
   local bd = (cur_step.orientation == laying) and l or s

   local x, y, z = 0, .9, -.05
   
   add_box("box1")
   set_pos("box1", x - cur_step.xdir, y - cur_step.ydir, z - cur_step.zdir)
   set_dim("box1", bw, bh, bd)
   set_texture("box1", "white-checker.png")

   add_box("box2")
   set_pos("box2", x, y, z)
   set_dim("box2", bw, bh, bd)
   set_texture("box2", "white-checker.png")

   add_box("box3")
   set_pos("box3", x + cur_step.xdir, y + cur_step.ydir, z + cur_step.zdir)
   set_dim("box3", bw, bh, bd)
   set_texture("box3", "white-checker.png")

   set_texture(boxes[choice], "blue-checker.png")
   add_inbox_trigger(boxes[choice], "controller", "on_in_box")

   set_reward(0)
   start_recording()
end

function on_in_box()
   if is_clicked("controller") then
      clear_scene();
      set_reward(1);
      set_variable("end", 1)
      add_next_trigger("on_start")
   end
end

function add_step(orientation_, xdir_, ydir_, zdir_, n_clicks_, long_side_, short_side_)
   table.insert(steps, {orientation = orientation,
                        xdir = xdir_,
                        ydir = ydir_,
                        zdir = zdir_,
                        n_clicks = n_clicks_,
                        long_side = long_side_,
                        short_side = short_side_})
end

function init()
   steps = {}
   choice = 0
   step = 0
   click = 0
   
   n_clicks = 10
   long_size = .2
   short_size = .01
   dist = .02
   
   add_hmd()
   add_controller("controller")
   add_variable("step")
   add_variable("click")
   add_variable("orientation")
   add_variable("xdir")
   add_variable("ydir")
   add_variable("zdir")
   add_variable("long_side")
   add_variable("short_side")
   add_variable("choice")
   add_mark_variable("start")
   add_mark_variable("end")
   
   register_function("on_in_box", on_in_box)
   register_function("on_start", on_start)
   add_click_trigger("controller", "on_start")
   
   add_box("startbox")
   set_pos("startbox", 0, .9, -.05)
   set_dim("startbox", .05, .05, .05)
   set_texture("startbox", "white-checker")
end

init()
