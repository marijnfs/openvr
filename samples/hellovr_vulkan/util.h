#ifndef __UTIL_REFRACTOR_H__
#define __UTIL_REFRACTOR_H__

#include <string>
#include <fstream>
#include <streambuf>
#include <vulkan/vulkan.h>
#include <openvr.h>
#include <iostream>
#include <SDL.h>
#include "shared/Matrices.h"

#if defined(POSIX)
#include "unistd.h"
#endif

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

inline void ThreadSleep( unsigned long nMilliseconds )
{
#if defined(_WIN32)
  ::Sleep( nMilliseconds );
#elif defined(POSIX)
  usleep( nMilliseconds * 1000 );
#endif
}

template <typename T>
inline void check(VkResult res, std::string str, T other) {
  if (res != VK_SUCCESS) {
    std::cerr << str << other << " error: " << res << std::endl;
    throw "err";
  }
}


inline void check(VkResult res, std::string str) {
	check(res, str, "");

}

inline void check(vr::EVRInitError err) {
  if ( err != vr::VRInitError_None ) {
    std::cerr << "Unable to init vr: " << vr::VR_GetVRInitErrorAsEnglishDescription(err) << std::endl;
    throw "";
  }
}

inline void sdl_check(int err) {
  if (err < 0) {
    std::cerr << "SDL error: " << SDL_GetError() << std::endl;
    throw "";
  }
}

struct StringException : public std::exception {
	StringException(std::string msg_): msg(msg_){}
	char const* what() const throw() {return msg.c_str();}
	~StringException() throw() {}
	std::string msg;
};


inline std::string read_all(std::string path) {
	std::ifstream t(path.c_str());
	if (!t)
		throw "failed to open";

	std::string str;

	t.seekg(0, std::ios::end);   
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(t)),
	            std::istreambuf_iterator<char>());
	return str;
}

inline void gen_mipmap_rgba( const uint8_t *src, uint8_t *dst, int width, int height, int *width_out, int *height_out )
{
	*width_out = width / 2;
	if ( *width_out <= 0 )
	{
		*width_out = 1;
	}
	*height_out = height / 2;
	if ( *height_out <= 0 )
	{
		*height_out = 1;
	}

	for ( int y = 0; y < *height_out; y++ )
	{
		for ( int x = 0; x < *width_out; x++ )
		{
			int nSrcIndex[4];
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			float a = 0.0f;

			nSrcIndex[0] = ( ( ( y * 2 ) * width ) + ( x * 2 ) ) * 4;
			nSrcIndex[1] = ( ( ( y * 2 ) * width ) + ( x * 2 + 1 ) ) * 4;
			nSrcIndex[2] = ( ( ( ( y * 2 ) + 1 ) * width ) + ( x * 2 ) ) * 4;
			nSrcIndex[3] = ( ( ( ( y * 2 ) + 1 ) * width ) + ( x * 2 + 1 ) ) * 4;

			// Sum all pixels
			for ( int nSample = 0; nSample < 4; nSample++ )
			{
				r += src[ nSrcIndex[ nSample ] ];
				g += src[ nSrcIndex[ nSample ] + 1 ];
				b += src[ nSrcIndex[ nSample ] + 2 ];
				a += src[ nSrcIndex[ nSample ] + 3 ];
			}

			// Average results
			r /= 4.0;
			g /= 4.0;
			b /= 4.0;
			a /= 4.0;

			// Store resulting pixels
			dst[ ( y * ( *width_out ) + x ) * 4 ] = ( uint8_t ) ( r );
			dst[ ( y * ( *width_out ) + x ) * 4 + 1] = ( uint8_t ) ( g );
			dst[ ( y * ( *width_out ) + x ) * 4 + 2] = ( uint8_t ) ( b );
			dst[ ( y * ( *width_out ) + x ) * 4 + 3] = ( uint8_t ) ( a );
		}
	}
}


inline Matrix4 vrmat_to_mat4( const vr::HmdMatrix34_t &matPose )
{
  Matrix4 matrixObj(
    matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
    matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
    matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
    matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
    );
  return matrixObj;
}



struct Pos3Tex2
{
  Vector3 pos;
  Vector2 texpos;
};

struct Pos2Tex2
{
  Vector2 pos;
  Vector2 texpos;
};

#endif