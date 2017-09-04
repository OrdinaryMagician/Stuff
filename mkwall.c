/*
	mkwall.c : The de-facto sayachan network wallpaper generator.
	(C)2015-2017 Marisa Kirisame, UnSX Team. For personal use mostly.
	Released under the GNU GPLv3 (or any later version).
	Please refer to <https://gnu.org/licenses/gpl.txt> for license terms.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <Imlib2.h>
#include <time.h>

/*
   TODO:
    - Option to put waifu on all screens
    - Find a faster library for imaging
    - Use xcb instead of Xlib
*/

/* helper macro for linear interpolation */
#define lerp(a,b,t) ((1.f-(t))*(a)+(t)*(b))

/* a BGRA pixel (packed just in case, even if pointless) */
typedef struct
{
	unsigned char b,g,r,a;
} __attribute__((packed)) pixel_t;

/*
   generate a BGRA8 image filled with a 4x4 diagonal stripe pattern
   interpolating between two specific colours
*/
void* generate_diag( int w, int h, pixel_t cup, pixel_t cdn )
{
	const float stripe[4] = {0.625f,0.375f,0.f,1.f};
	int x, y, i;
	pixel_t *diag = malloc(sizeof(pixel_t)*w*h);
	i = 0;
	for ( y=0; y<h; y++ )
	{
		for ( x=0; x<w; x++ )
		{
			diag[y*w+x].b = lerp(cup.b,cdn.b,stripe[i%4]);
			diag[y*w+x].g = lerp(cup.g,cdn.g,stripe[i%4]);
			diag[y*w+x].r = lerp(cup.r,cdn.r,stripe[i%4]);
			diag[y*w+x].a = 255;
			i++;
		}
		i=y%4;
	}
	return diag;
}

int main( int argc, char **argv )
{
	clock_t tick, tock;
	if ( argc < 6 )
	{
		printf("usage: %s <waifu> <RGB8> <RGB8> <l|c|r> <screen>"
			" <filename>\n",basename(argv[0]));
		return 1;
	}
	Display *d = XOpenDisplay(0);
	Screen *s = ScreenOfDisplay(d,DefaultScreen(d));
	int rw = s->width, rh = s->height;
	Imlib_Image wall, waifu;
	tick = clock();
	waifu = imlib_load_image(argv[1]);
	if ( !waifu ) return 2;
	tock = clock();
	printf("load waifu: %fs\n",(double)(tock-tick)/CLOCKS_PER_SEC);
	imlib_context_set_image(waifu);
	int ww = imlib_image_get_width(), wh = imlib_image_get_height();
	pixel_t cup, cdn;
	/* read the hex RGB8 gradient colours */
	sscanf(argv[2],"%02hhx%02hhx%02hhx",&cup.r,&cup.g,&cup.b);
	sscanf(argv[3],"%02hhx%02hhx%02hhx",&cdn.r,&cdn.g,&cdn.b);
	tick = clock();
	wall = imlib_create_image_using_data(rw,rh,
		generate_diag(rw,rh,cup,cdn));
	tock = clock();
	printf("generate diag: %fs\n",(double)(tock-tick)/CLOCKS_PER_SEC);
	imlib_context_set_image(wall);
	/*
	   get the specified screen geometry if xinerama is active, otherwise
	   just use the full virtual screen
	*/
	int sw, sh, ox = 0, oy = 0;
	int dummy1, dummy2;
	if ( XineramaQueryExtension(d,&dummy1,&dummy2) && XineramaIsActive(d) )
	{
		printf("xinerama active, querying...\n");
		int nxs, sno;
		XineramaScreenInfo *xs = XineramaQueryScreens(d,&nxs);
		sscanf(argv[5],"%d",&sno);
		if ( (nxs+sno) < 0 ) sno = 0;
		if ( sno >= nxs ) sno = nxs-1;
		if ( sno < 0 ) sno = nxs+sno;
		printf("%d screens available, #%d selected\n",nxs,sno);
		for ( int i=0; i<nxs; i++ )
		{
			printf(" Screen %d: %hdx%hd+%hd+%hd\n",
				xs[i].screen_number,xs[i].width,xs[i].height,
				xs[i].x_org,xs[i].y_org);
		}
		rw = xs[sno].width;
		rh = xs[sno].height;
		ox = xs[sno].x_org;
		oy = xs[sno].y_org;
		XFree(xs);
	}
	sw = (ww*rh)/wh;
	sh = rh;
	ox += (*argv[4]=='l')?(0):(*argv[4]=='r')?(rw-sw):((rw-sw)/2);
	tick = clock();
	imlib_blend_image_onto_image(waifu,0,0,0,ww,wh,ox,oy,sw,sh);
	tock = clock();
	printf("blend images: %fs\n",(double)(tock-tick)/CLOCKS_PER_SEC);
	/* save to file */
	char *ext = strrchr(argv[6],'.');
	if ( ext ) imlib_image_set_format(ext+1);
	tick = clock();
	imlib_save_image(argv[6]);
	tock = clock();
	printf("save image: %fs\n",(double)(tock-tick)/CLOCKS_PER_SEC);
	XCloseDisplay(d);
	imlib_free_image();
	imlib_context_set_image(waifu);
	imlib_free_image();
	return 0;
}
