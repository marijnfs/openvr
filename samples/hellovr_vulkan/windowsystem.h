#ifndef __WINDOW_SYSTEM_H__
#define __WINDOW_SYSTEM_H__

#if defined( _WIN32 )
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define SDL_VIDEO_DRIVER_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <SDL.h>
#include <SDL_syswm.h>
#include <stdio.h>


struct WindowSystem {
  SDL_Window *window;
  uint32_t width, height;
  Buffer vertex_buf, index_buf;

  WindowSystem();

  void setup_window();

};

#endif
