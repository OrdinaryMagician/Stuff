/*
	fuzz.c : Fancy software filter.
	I was bored.
	(C)2016 Marisa Kirisame, UnSX Team.
	Released under the MIT license.
*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>

typedef struct
{
	float r,g,b,a;
} rgbfpx_t;

typedef struct
{
	Uint8 r,g,b,a;
} __attribute__((packed)) rgb8px_t;

float t = 0.f;
const int fw = 640, fh = 480;

float rn( float sdx, float sdy )
{
	return cosf(sdy*3874.8674f+sdx*6783.5325f)*2737.8474f;
}

float fract( float x )
{
	return x-floorf(x);
}

SDL_Window *w;
SDL_Surface *ws, *fz;

rgbfpx_t *fl1, *fl2, *fl3;
const int fw1 = 320, fw2 = 160, fw3 = 80, fh1 = 240, fh2 = 120, fh3 = 60;

void mklayer1( void )
{
	for ( int y=0; y<fh1; y++ )
	{
		float rg;
		for ( int x=0; x<fw1; x++ )
		{
			rg = 2.f*fabsf(fract(rn(x,y)+t*1.3526f)-0.5f);
			fl1[x+y*fw1].r = 0.71f*rg;
			fl1[x+y*fw1].g = 0.67f*rg;
			fl1[x+y*fw1].b = 0.95f*rg;
			fl1[x+y*fw1].a = 1.f*rg;
		}
	}
}

void mklayer2( void )
{
	for ( int y=0; y<fh2; y++ )
	{
		float rg;
		for ( int x=0; x<fw2; x++ )
		{
			rg = 2.f*fabsf(fract(rn(x,y)+t*0.7843f)-0.5f);
			fl2[x+y*fw2].r = 0.66f*rg;
			fl2[x+y*fw2].g = 0.84f*rg;
			fl2[x+y*fw2].b = 0.73f*rg;
			fl2[x+y*fw2].a = 1.f*rg;
		}
	}
}

void mklayer3( void )
{
	for ( int y=0; y<fh3; y++ )
	{
		float rg;
		for ( int x=0; x<fw3; x++ )
		{
			rg = 2.f*fabsf(fract(rn(x,y)+t*0.3725f)-0.5f);
			fl3[x+y*fw3].r = 0.95f*rg;
			fl3[x+y*fw3].g = 0.73f*rg;
			fl3[x+y*fw3].b = 0.81f*rg;
			fl3[x+y*fw3].a = 1.f*rg;
		}
	}
}

void mergedown( void )
{
	rgbfpx_t twofivefive = {255.f,255.f,255.f,255.f};
	#pragma omp parallel for
	for ( int y=0; y<fh; y++ )
	{
		int y1, y2, y3;
		rgb8px_t *stripe;
		rgbfpx_t merged;
		y1 = y>>1;
		y2 = y>>2;
		y3 = y>>3;
		stripe = (rgb8px_t*)((Uint8*)fz->pixels+y*fz->pitch);
		for ( int x=0; x<fw; x++ )
		{
			int x1, x2, x3;
			x1 = x>>1;
			x2 = x>>2;
			x3 = x>>3;
			// gcc is stupid
			asm(	"movaps %1,%%xmm0\n"
				"mulps %2,%%xmm0\n"
				"mulps %3,%%xmm0\n"
				"mulps %4,%%xmm0\n"
				"movaps %%xmm0,%0\n"
				:"=m"(merged)
				:"m"(fl1[x1+y1*fw1]),"m"(fl2[x2+y2*fw2]),
				"m"(fl3[x3+y3*fw3]),"m"(twofivefive));
			stripe[x].r = merged.r;
			stripe[x].g = merged.g;
			stripe[x].b = merged.b;
			stripe[x].a = merged.a;
		}
	}
}

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

int main( void )
{
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
	w = SDL_CreateWindow("Fuzz",SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,fw,fh,SDL_WINDOW_SHOWN);
	ws = SDL_GetWindowSurface(w);
	fz = SDL_CreateRGBSurface(0,fw,fh,32,0xFF,0xFF00,0xFF0000,0xFF000000);
	SDL_SetSurfaceBlendMode(fz,SDL_BLENDMODE_NONE);
	fl1 = malloc(sizeof(rgbfpx_t)*fw1*fh1);
	fl2 = malloc(sizeof(rgbfpx_t)*fw2*fh2);
	fl3 = malloc(sizeof(rgbfpx_t)*fw3*fh3);
	TTF_Init();
	TTF_Font *fon = TTF_OpenFont("/usr/share/fonts/misc/unifont.bdf",16);
	SDL_Event e;
	SDL_Color fpscol = {160,0,0,255};
	int active = 1;
	long tick, tock;
	float frame = 0.f, fps = NAN, fpsmin = INFINITY, fpsmax = -INFINITY,
		fpsavg = 0.f;
	char fpst[16];
	while ( active )
	{
		tick = ticker();
		while ( SDL_PollEvent(&e) ) if ( (e.type == SDL_QUIT)
			|| ((e.type == SDL_KEYDOWN)
			&& (e.key.keysym.sym == SDLK_ESCAPE)) ) active = 0;
		mklayer1();
		mklayer2();
		mklayer3();
		mergedown();
		SDL_BlitSurface(fz,0,ws,0);
		snprintf(fpst,15,"%.2f FPS",fps);
		SDL_Surface *txt = TTF_RenderText_Blended(fon,fpst,fpscol);
		SDL_BlitSurface(txt,0,ws,0);
		SDL_FreeSurface(txt);
		SDL_UpdateWindowSurface(w);
		tock = ticker();
		frame = (float)(tock-tick)/NANOS_SEC;
		fps = 1.f/frame;
		fpsavg = avg_fps(fps);
		if ( fps > fpsmax ) fpsmax = fps;
		if ( fps < fpsmin ) fpsmin = fps;
		printf("FPS: %.2f (%.2f min, %.2f max, %.2f avg)\n",fps,
			fpsmin,fpsmax,fpsavg);
		t += frame;
	}
	TTF_CloseFont(fon);
	TTF_Quit();
	free(fl3);
	free(fl2);
	free(fl1);
	SDL_FreeSurface(fz);
	SDL_DestroyWindow(w);
	SDL_Quit();
	return 0;
}
