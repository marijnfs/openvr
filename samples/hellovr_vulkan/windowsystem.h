#ifndef __WINDOW_SYSTEM_H__
#define __WINDOW_SYSTEM_H__

#include <SDL.h>
#include <SDL_syswm.h>
#include <stdio.h>

#include "buffer.h"
#include "framerenderbuffer.h"

struct WindowSystem {
  SDL_Window *window;
  uint32_t width, height;

  Buffer vertex_buf, index_buf;
  FrameRenderBuffer *framebuffer;
  
  WindowSystem();
  void init();

  void setup_window();
  void setup_buffers();
  
  void show_message(std::string str);
};

#endif
