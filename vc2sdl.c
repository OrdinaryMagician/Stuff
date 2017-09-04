#include <bcm_host.h>
#include <SDL2/SDL.h>
#include <time.h>

#define TMAX 64
int ti = 0;
float ts = 0.f, tl[TMAX] = {0.f};

float avg_fps( float nt )
{
	ts = ts-tl[ti]+nt;
	tl[ti] = nt;
	if ( ++ti == TMAX ) ti = 0;
	return ts/TMAX;
}

#define NANOS_SEC 1000000000L

long ticker( void )
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
	return ts.tv_nsec+ts.tv_sec*NANOS_SEC;
}

int main( int argc, char **argv )
{
	unsigned reduce = 1;
	unsigned width = 720, height = 480;
	if ( argc > 2 )
	{
		sscanf(argv[2],"%u",&reduce);
		sscanf(argv[1],"%ux%u",&width,&height);
	}
	width /= reduce;
	height /= reduce;
	bcm_host_init();
	DISPMANX_DISPLAY_HANDLE_T d = vc_dispmanx_display_open(0);
	if ( !d ) return 1;
	DISPMANX_MODEINFO_T m;
	if ( vc_dispmanx_display_get_info(d,&m) )
	{
		vc_dispmanx_display_close(d);
		return 2;
	}
	unsigned pt;
	DISPMANX_RESOURCE_HANDLE_T r =
		vc_dispmanx_resource_create(VC_IMAGE_RGB888,width,height,&pt);
	if ( !r )
	{
		vc_dispmanx_display_close(d);
		return 4;
	}
	VC_RECT_T rt;
	vc_dispmanx_rect_set(&rt,0,0,width,height);
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *w = SDL_CreateWindow("VC4 Feed",SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,width,height,SDL_WINDOW_SHOWN);
	SDL_Surface *ws = SDL_GetWindowSurface(w);
	SDL_Surface *copy = SDL_CreateRGBSurface(0,width,height,24,0xFF,0xFF00,
		0xFF0000,0);
	int active = 1;
	SDL_Event e;
	long tick, tock;
	float frame = 0.f, fps = NAN, fpsmin = INFINITY, fpsmax = -INFINITY,
		fpsavg = 0.f;
	while ( active )
	{
		tick = ticker();
		while ( SDL_PollEvent(&e) )
			if ( e.type == SDL_QUIT ) active = 0;
		vc_dispmanx_snapshot(d,r,0);
		vc_dispmanx_resource_read_data(r,&rt,copy->pixels,copy->pitch);
		SDL_BlitSurface(copy,0,ws,0);
		SDL_UpdateWindowSurface(w);
		tock = ticker();
		frame = (float)(tock-tick)/NANOS_SEC;
		fps = 1.f/frame;
		fpsavg = avg_fps(fps);
		if ( fps > fpsmax ) fpsmax = fps;
		if ( fps < fpsmin ) fpsmin = fps;
		printf("FPS: %.2f (%.2f min, %.2f max, %.2f avg)\n",fps,
			fpsmin,fpsmax,fpsavg);
	}
	vc_dispmanx_display_close(d);
	SDL_FreeSurface(copy);
	SDL_DestroyWindow(w);
	SDL_Quit();
	return 0;
}
