#ifndef __FRAMERENDERBUFFER_H__
#define __FRAMERENDERBUFFER_H__

#include "buffer.h"
#include "vulkansystem.h"

struct FrameRenderBuffer {
    Image img, depth_stencil;
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
    int width = 0, height = 0;
  Descriptor desc;
  
    void init(int width_, int height_);



	void start_render_pass();
	void end_render_pass();
};




#endif

