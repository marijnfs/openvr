#include "windowsystem.h"
#include "util.h"

using namespace std;

WindowSystem::WindowSystem() : width(800), height(800) {

}

void WindowSystem::init() {
	sdl_check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER));


	setup_window();
}

void WindowSystem::setup_window() {
	cout << "in setup window" << endl;
	int pos_x = 700;
	int pos_y = 100;
	Uint32 wflags = SDL_WINDOW_SHOWN;

	window = SDL_CreateWindow( "hellovr [Vulkan]", pos_x, pos_y, width, height, wflags );
	if (!window) {
		cerr << "SDL Window problem: " << SDL_GetError() << endl;
		throw "";
	}

	cout << "setting title" << endl;

	string title("Window Title");
	SDL_SetWindowTitle( window, title.c_str() );
	

	vector<Pos2Tex2> verts;

	//left eye verts
	verts.push_back( Pos2Tex2{ Vector2(-1, -1), Vector2(0, 1)});
	verts.push_back( Pos2Tex2{ Vector2(0, -1), Vector2(1, 1)} );
	verts.push_back( Pos2Tex2{ Vector2(-1, 1), Vector2(0, 0)} );
	verts.push_back( Pos2Tex2{ Vector2(0, 1), Vector2(1, 0) });

// right eye verts
	verts.push_back( Pos2Tex2{ Vector2(0, -1), Vector2(0, 1)} );
	verts.push_back( Pos2Tex2{ Vector2(1, -1), Vector2(1, 1)} );
	verts.push_back( Pos2Tex2{ Vector2(0, 1), Vector2(0, 0)} );
	verts.push_back( Pos2Tex2{ Vector2(1, 1), Vector2(1, 0)} );

	uint16_t indices[] = { 0, 1, 3,   0, 3, 2,   4, 5, 7,   4, 7, 6};

	cout << "setting up buffers" << endl;

//aTODO: add initialisation
	vertex_buf.init(sizeof(Pos2Tex2) * verts.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, DEVICE);
	index_buf.init(sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, DEVICE);

	cout << " done with buffers" << endl;
}
