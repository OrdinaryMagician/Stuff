/*
	UDMF map visualizer - Kinda like dmvis but for UDMF maps specifically.
	(C)2018 Marisa Kirisame, UnSX Team.
	Released under the GNU General Public License version 3 (or later).
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

/*
   At the moment output is broken on most image viewers, but it can be fixed by
   running it through gifsicle.

   Things left to do:
    - Figure out why output is bugged.
    - Properly sort lines like dmvis.
    - Full-sector drawing like dmvis.
    - Skipping over already drawn lines like dmvis.
    - Support doom/hexen map format and reading from wads.
    - Rename project to "cdmvis" when the previous point is done.
    - Customizable colorschemes.
    - Fill sector insides.
    - Isometric perspective view? (+ 3d floor and slope handling)
    - Draw things too?
*/

typedef struct
{
	double x, y;
} vertex_t;

typedef struct
{
	int v1, v2, type;
	int fs, bs;
} line_t;

typedef struct
{
	int s;
	int v1, v2, type;
} side_t;

vertex_t *verts = 0;
line_t *lines = 0;
side_t *sides = 0;
int nverts = 0, nlines = 0, nsides = 0;

#define LN_ONESIDED 0
#define LN_TWOSIDED 1
#define LN_ASPECIAL 2

int cmp_sides_by_vertex( const void *a, const void *b )
{
	side_t *sa = (side_t*)a;
	side_t *sb = (side_t*)b;
	return sa->v2 - sb->v1;
}
int cmp_sides_by_sector( const void *a, const void *b )
{
	side_t *sa = (side_t*)a;
	side_t *sb = (side_t*)b;
	return sa->s - sb->s;
}

#define LAST_NONE       0
#define LAST_IDENT      1
#define LAST_EQUALS     2

static int bx, by, bpitch;
static uint8_t *b = 0;

#define swap(t,x,y) {t tmp=x;x=y;y=tmp;}

static void bresenham( int col, int x0, int y0, int x1, int y1 )
{
	// copied from proto-agl
	x0 -= bx;
	x1 -= bx;
	y0 -= by;
	y1 -= by;
	int steep = 0;
	if ( abs(x0-x1) < abs(y0-y1) )
	{
		swap(int,x0,y0);
		swap(int,x1,y1);
		steep = 1;
	}
	if ( x0 > x1 )
	{
		swap(int,x0,x1);
		swap(int,y0,y1);
	}
	int dx = x1-x0;
	int dy = y1-y0;
	int der = abs(dy)*2;
	int er = 0;
	int y = y0;
	for ( int x=x0; x<=x1; x++ )
	{
		int px = x, py = y;
		if ( steep ) swap(int,px,py);
		b[px+py*bpitch] = col;
		er += der;
		if ( er > dx )
		{
			y += (y1>y0)?1:-1;
			er -= dx*2;
		}
	}
}

void getsidebounds( int i, int *x, int *y, int *w, int *h )
{
	int minx = 0, miny = 0, maxx = 0, maxy = 0;
	if ( i < nsides )
	{
		if ( i > 0 )
		{
			minx = verts[sides[i-1].v1].x;
			if ( verts[sides[i-1].v2].x < minx )
				minx = verts[sides[i-1].v2].x;
			if ( verts[sides[i].v1].x < minx )
				minx = verts[sides[i].v1].x;
			if ( verts[sides[i].v2].x < minx )
				minx = verts[sides[i].v2].x;
			maxx = verts[sides[i-1].v1].x;
			if ( verts[sides[i-1].v2].x > maxx )
				maxx = verts[sides[i-1].v2].x;
			if ( verts[sides[i].v1].x > maxx )
				maxx = verts[sides[i].v1].x;
			if ( verts[sides[i].v2].x > maxx )
				maxx = verts[sides[i].v2].x;
			miny = verts[sides[i-1].v1].y;
			if ( verts[sides[i-1].v2].y < miny )
				miny = verts[sides[i-1].v2].y;
			if ( verts[sides[i].v1].y < miny )
				miny = verts[sides[i].v1].y;
			if ( verts[sides[i].v2].y < miny )
				miny = verts[sides[i].v2].y;
			maxy = verts[sides[i-1].v1].y;
			if ( verts[sides[i-1].v2].y > maxy )
				maxy = verts[sides[i-1].v2].y;
			if ( verts[sides[i].v1].y > maxy )
				maxy = verts[sides[i].v1].y;
			if ( verts[sides[i].v2].y > maxy )
				maxy = verts[sides[i].v2].y;
		}
		else
		{
			minx = verts[sides[i].v1].x;
			if ( verts[sides[i].v2].x < minx )
				minx = verts[sides[i].v2].x;
			maxx = verts[sides[i].v1].x;
			if ( verts[sides[i].v2].x > maxx )
				maxx = verts[sides[i].v2].x;
			miny = verts[sides[i].v1].y;
			if ( verts[sides[i].v2].y < miny )
				miny = verts[sides[i].v2].y;
			maxy = verts[sides[i].v1].y;
			if ( verts[sides[i].v2].y > maxy )
				maxy = verts[sides[i].v2].y;
		}
	}
	else if ( i > 0 )
	{
		minx = verts[sides[i-1].v1].x;
		if ( verts[sides[i-1].v2].x < minx )
			minx = verts[sides[i-1].v2].x;
		maxx = verts[sides[i-1].v1].x;
		if ( verts[sides[i-1].v2].x > maxx )
			maxx = verts[sides[i-1].v2].x;
		miny = verts[sides[i-1].v1].y;
		if ( verts[sides[i-1].v2].y < miny )
			miny = verts[sides[i-1].v2].y;
		maxy = verts[sides[i-1].v1].y;
		if ( verts[sides[i-1].v2].y > maxy )
			maxy = verts[sides[i-1].v2].y;
	}
	*x = minx;
	*y = miny;
	*w = (maxx-minx) + 1;
	*h = (maxy-miny) + 1;
}

void drawside( int i )
{
	// draw previous side normally (if any)
	if ( i > 0 )
	{
		int c = 3;
		if ( sides[i-1].type&LN_ASPECIAL ) c = 2;
		else if ( sides[i-1].type&LN_TWOSIDED ) c = 4;
		bresenham(c,verts[sides[i-1].v1].x,verts[sides[i-1].v1].y,
			verts[sides[i-1].v2].x,verts[sides[i-1].v2].y);
	}
	// draw current line highlighted (if any)
	if ( i < nsides )
	{
		bresenham(1,verts[sides[i].v1].x,verts[sides[i].v1].y,
			verts[sides[i].v2].x,verts[sides[i].v2].y);
	}
}

uint8_t gifpal[8][3] =
{
	{255,255,255},	// background
	{220,0,0},	// cursor
	{220,130,50},	// special line
	{0,0,0},	// one-sided line
	{144,144,144},	// two-sided line
	{0,0,0},
	{0,0,0},
	{0,0,0},
};

#define LZW_MAXCODLEN 12
#define LZW_MAXNCODES (1<<LZW_MAXCODLEN)

typedef struct
{
	uint64_t bits;
	int nbits;
	int code;
	int initialncodes, ncodes;
	int initialclen, clen;
	int maxclen;
	int rstcode;
	struct
	{
		uint16_t next[256];
	} codes[LZW_MAXNCODES];
} lzwenc_t;

lzwenc_t lzwenc;

void resetlzw( void )
{
	lzwenc.ncodes = lzwenc.initialncodes;
	lzwenc.clen = lzwenc.initialclen;
	memset(lzwenc.codes,0,512*(1<<lzwenc.maxclen));
}

void initlzw( int minclen, int maxclen, int nlit, int nresv,
	int rstcode )
{
	lzwenc.bits = 0;
	lzwenc.nbits = 0;
	lzwenc.code = -1;
	lzwenc.initialncodes = nlit+nresv;
	lzwenc.initialclen = minclen;
	lzwenc.maxclen = maxclen;
	lzwenc.rstcode = rstcode;
	resetlzw();
}

void emitlzwcode( int code )
{
	lzwenc.bits |= ((uint64_t)code)<<lzwenc.nbits;
	lzwenc.nbits += lzwenc.clen;
}

void emitlzwb( uint8_t b )
{
	if ( lzwenc.code < 0 )
	{
		lzwenc.code = b;
		return;
	}
	uint16_t ncode = lzwenc.codes[lzwenc.code].next[b];
	if ( ncode ) lzwenc.code = ncode;
	else
	{
		emitlzwcode(lzwenc.code);
		lzwenc.codes[lzwenc.code].next[b] = lzwenc.ncodes++;
		lzwenc.code = b;
		if ( lzwenc.ncodes > (1<<lzwenc.clen) )
		{
			if ( lzwenc.clen < lzwenc.maxclen )
				lzwenc.clen++;
			else
			{
				emitlzwcode(lzwenc.rstcode);
				resetlzw();
			}
		}
	}
}

void emitlzwend( int end )
{
	emitlzwcode(lzwenc.code);
	if ( end ) emitlzwcode(end);
}

int nextlzwob( int flush )
{
	if ( lzwenc.nbits >= 8 )
	{
		int out = lzwenc.bits&0xff;
		lzwenc.bits >>= 8;
		lzwenc.nbits -= 8;
		return out;
	}
	else if ( flush && (lzwenc.nbits > 0) )
	{
		int out = lzwenc.bits&0xff;
		lzwenc.bits = 0;
		lzwenc.nbits = 0;
		return out;
	}
	else return -1;
}

void drainlzw( FILE *f, uint8_t *buf, int flush )
{
	for ( ; ; )
	{
		int b = nextlzwob(flush);
		if ( b < 0 ) break;
		buf[0]++;
		buf[buf[0]] = b;
		if ( buf[0] == 255 )
		{
			fwrite(buf,1,256,f);
			buf[0] = 0;
		}
	}
	if ( flush )
	{
		if ( buf[0] != 0 ) fwrite(buf,1,buf[0]+1,f);
		fputc(0,f);
		buf[0] = 0;
	}
}

void writelzw( uint8_t *data, int siz, int bits, FILE *f )
{
	fputc(bits,f);
	int nlit = 1<<bits;
	int ccode = nlit;
	int ecode = nlit+1;
	initlzw(bits+1,12,nlit,2,ccode);
	uint8_t buffer[256] = {0};
	for ( int i=0; i<siz; i++ )
	{
		emitlzwb(data[i]);
		drainlzw(f,buffer,0);
	}
	emitlzwend(ecode);
	drainlzw(f,buffer,1);
}

void mkanim( int imgw )
{
	/* calculate bounds, adjust scaling */
	double minx, miny, maxx, maxy, mapw, maph, scale;
	int border = 8, imgh;
	maxx = minx = verts[0].x;
	maxy = miny = -verts[0].y;
	for ( int i=0; i<nverts; i++ )
	{
		if ( verts[i].x < minx ) minx = verts[i].x;
		if ( verts[i].x > maxx ) maxx = verts[i].x;
		if ( -verts[i].y < miny ) miny = -verts[i].y;
		if ( -verts[i].y > maxy ) maxy = -verts[i].y;
	}
	mapw = maxx-minx;
	maph = maxy-miny;
	double mx = (mapw > maph) ? mapw : maph;
	scale = (imgw-border*2)/mx;
	minx *= scale;
	miny *= scale;
	maxx *= scale;
	maxy *= scale;
	imgh = maxy-miny+2*border;
	for ( int i=0; i<nverts; i++ )
	{
		verts[i].x = (scale*verts[i].x)-minx + border;
		verts[i].y = (scale*(-verts[i].y))-miny + border;
	}
	/*
	   Was going to use giflib for saving this but the library is a
	   horrendous, barely documented mess.
	   I'll just do it myself.
	*/
	FILE *gif = fopen("udmfvis.gif","wb");
	if ( !gif )
	{
		fprintf(stderr,"couldn't open output: %s\n",strerror(errno));
		return;
	}
	uint8_t ghead[] =
	{
		'G','I','F','8','9','a',
		imgw&0xff,imgw>>8,
		imgh&0xff,imgh>>8,
		0xF2,0x00,0x00,
	};
	fwrite(ghead,sizeof(ghead),1,gif);
	fwrite(gifpal,3,8,gif);
	uint8_t appext[] =
	{
		0x21,0xff,0x0b,
		'N','E','T','S','C','A','P','E',
		'2','.','0',
		0x03,0x01,0x00,0x00,0x00,
	};
	fwrite(appext,sizeof(appext),1,gif);
	/* allocate base layer */
	b = calloc(imgw*imgh,1);
	uint8_t gce[] =
	{
		0x21,0xf9,0x04,0x04,
		0x02,0x00,
		0x00,0x00,
	};
	fwrite(gce,sizeof(gce),1,gif);
	uint8_t ihead[] =
	{
		0x2c,
		0x00,0x00,
		0x00,0x00,
		imgw&0xff,imgw>>8,
		imgh&0xff,imgh>>8,
		0x00,
	};
	fwrite(ihead,sizeof(ihead),1,gif);
	writelzw(b,imgw*imgh,3,gif);
	for ( int i=0; i<=nsides; i++ )
	{
		printf("\rwriting frame %d of %d",i,nsides);
		int delay = (i<nsides)?2:3000;
		uint8_t gce[] =
		{
			0x21,0xf9,0x04,0x05,
			delay&0xff,delay>>8,
			0x00,0x00,
		};
		fwrite(gce,sizeof(gce),1,gif);
		/* calculate bounds */
		int x, y, w, h;
		getsidebounds(i,&x,&y,&w,&h);
		bx = x;
		by = y;
		bpitch = w;
		uint8_t ihead[] =
		{
			0x2c,
			x&0xff,x>>8,
			y&0xff,y>>8,
			w&0xff,w>>8,
			h&0xff,h>>8,
			0x00,
		};
		fwrite(ihead,sizeof(ihead),1,gif);
		/* render */
		memset(b,0,w*h);
		drawside(i);
		/* write frame */
		writelzw(b,w*h,3,gif);
	}
	free(b);
	putchar('\n');
	fputc(0x3b,gif);
	fclose(gif);
}

void process_startblock( char *blk )
{
	if ( !strcasecmp(blk,"vertex") )
	{
		if ( !verts ) verts = malloc(sizeof(vertex_t));
		else verts = realloc(verts,sizeof(vertex_t)*(nverts+1));
		memset(verts+nverts,0,sizeof(vertex_t));
	}
	else if ( !strcasecmp(blk,"linedef") )
	{
		if ( !lines ) lines = malloc(sizeof(line_t));
		else lines = realloc(lines,sizeof(line_t)*(nlines+1));
		memset(lines+nlines,0,sizeof(line_t));
		lines[nlines].bs = -1;
	}
	else if ( !strcasecmp(blk,"sidedef") )
	{
		if ( !sides ) sides = malloc(sizeof(side_t));
		else sides = realloc(sides,sizeof(side_t)*(nsides+1));
		memset(sides+nsides,0,sizeof(side_t));
	}
}

void process_assignment( char *blk, char *id, char *val )
{
	if ( !strcasecmp(blk,"vertex") )
	{
		if ( !strcasecmp(id,"x") )
			sscanf(val,"%lf",&verts[nverts].x);
		else if ( !strcasecmp(id,"y") )
			sscanf(val,"%lf",&verts[nverts].y);
	}
	else if ( !strcasecmp(blk,"linedef") )
	{
		if ( !strcasecmp(id,"v1") )
			sscanf(val,"%d",&lines[nlines].v1);
		else if ( !strcasecmp(id,"v2") )
			sscanf(val,"%d",&lines[nlines].v2);
		else if ( !strcasecmp(id,"sidefront") )
			sscanf(val,"%d",&lines[nlines].fs);
		else if ( !strcasecmp(id,"sideback") )
		{
			sscanf(val,"%d",&lines[nlines].bs);
			lines[nlines].type |= LN_TWOSIDED;
		}
		else if ( !strcasecmp(id,"special") )
			lines[nlines].type |= LN_ASPECIAL;
	}
	else if ( !strcasecmp(blk,"sidedef") )
	{
		if ( !strcasecmp(id,"sector") )
			sscanf(val,"%d",&sides[nsides].s);
	}
}

void process_endblock( char *blk )
{
	if ( !strcasecmp(blk,"vertex") )
	{
		nverts++;
	}
	else if ( !strcasecmp(blk,"linedef") )
	{
		nlines++;
	}
	else if ( !strcasecmp(blk,"sidedef") )
	{
		nsides++;
	}
}

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		fprintf(stderr,"usage: %s <textmap> [width]\n",argv[0]);
		return 1;
	}
	FILE *tmap = fopen(argv[1],"rb");
	if ( !tmap )
	{
		fprintf(stderr,"couldn't open textmap: %s\n",strerror(errno));
		return 2;
	}
	int ch;
	int last = 0;
	char id[256] = {0};
	char val[256] = {0};
	char blk[256] = {0};
	while ( !feof(tmap) )
	{
		/* check for comments */
		if ( (ch = fgetc(tmap)) == '/' )
		{
			if ( (ch = fgetc(tmap)) == '/' )
			{
				/* single line comment */
				while ( (ch = fgetc(tmap)) != '\n' );
			}
			else if ( (ch = fgetc(tmap)) == '*' )
			{
				/* multi-block comment (like this one) */
				while ( !feof(tmap) )
				{
					if ( (ch = fgetc(tmap)) != '*' )
						continue;
					if ( (ch = fgetc(tmap)) == '/' )
						break;
				}
			}
		}
		if ( (ch == ' ') || (ch == '\t') || (ch == '\n') ) continue;
		switch ( last )
		{
		case LAST_NONE:
			/* check for block end */
			if ( ch == '}' )
			{
				process_endblock(blk);
				blk[0] = 0;
				break;
			}
			/* read identifier */
			fseek(tmap,-1,SEEK_CUR);
			fscanf(tmap,"%[^ \t\n=/]",id);
			last = LAST_IDENT;
			break;
		case LAST_IDENT:
			/* assignment or block? */
			if ( ch == '=' ) last = LAST_EQUALS;
			else if ( ch == '{' )
			{
				strcpy(blk,id);
				process_startblock(blk);
				last = LAST_NONE;
			}
			break;
		case LAST_EQUALS:
			/* read the value */
			fseek(tmap,-1,SEEK_CUR);
			fscanf(tmap,"%[^;]",val);
			process_assignment(blk,id,val);
			ch = fgetc(tmap);
			last = LAST_NONE;
			break;
		}
	}
	fclose(tmap);
	/* sanity check */
	if ( (nverts <= 0) || (nlines <= 0) || (nsides <= 0) )
	{
		fprintf(stderr,"No valid map data found.\n");
		return 4;
	}
	/* transfer info to sides */
	for ( int i=0; i<nlines; i++ )
	{
		sides[lines[i].fs].v1 = lines[i].v1;
		sides[lines[i].fs].v2 = lines[i].v2;
		sides[lines[i].fs].type = lines[i].type;
		if ( lines[i].bs != -1 )
		{
			sides[lines[i].bs].v1 = lines[i].v2;
			sides[lines[i].bs].v2 = lines[i].v1;
			sides[lines[i].bs].type = lines[i].type;
		}
	}
	/* sort sides */
	qsort(sides,nsides,sizeof(side_t),cmp_sides_by_vertex);
	qsort(sides,nsides,sizeof(side_t),cmp_sides_by_sector);
	/* do the animation! */
	int iw = 1024;
	if ( argc > 2 ) sscanf(argv[2],"%d",&iw);
	mkanim(iw);
	if ( verts ) free(verts);
	if ( lines ) free(lines);
	if ( sides ) free(sides);
	return 0;
}
