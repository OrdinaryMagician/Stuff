/*
	lutsmooth.c : LUT smoothening.
	Softens Volume LUTs by applying gaussian blur across all three axes.
	(C)2017 Marisa Kirisame, UnSX Team.
	Released under the MIT license.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define dword uint32_t
#define byte uint8_t
#pragma pack(push,1)
typedef struct
{
	dword magic, size, flags, height, width, pitch, depth, mipmaps,
	reserved1[11], pf_size, pf_flags, pf_fourcc, pf_bitcount, pf_rmask,
	pf_gmask, pf_bmask, pf_amask, caps[4], reserved2;
} ddsheader_t;
#pragma pack(pop)

uint8_t *indata, *outdata;
int width, height, depth;
float gauss8[8] =
{
	0.134598, 0.127325, 0.107778, 0.081638,
	0.055335, 0.033562, 0.018216, 0.008847
};

void getpx( int x, int y, int z, float *r, float *g, float *b )
{
	/* clamp coords */
	if ( x < 0 ) x = 0;
	if ( y < 0 ) y = 0;
	if ( z < 0 ) z = 0;
	if ( x >= width ) x = width-1;
	if ( y >= height ) y = height-1;
	if ( z >= depth ) z = depth-1;
	*r = indata[(x+y*width+z*width*height)*4]/255.0;
	*g = indata[(x+y*width+z*width*height)*4+1]/255.0;
	*b = indata[(x+y*width+z*width*height)*4+2]/255.0;
}

void putpx( int x, int y, int z, float r, float g, float b )
{
	/* clamp coords */
	if ( x < 0 ) x = 0;
	if ( y < 0 ) y = 0;
	if ( z < 0 ) z = 0;
	if ( x >= width ) x = width-1;
	if ( y >= height ) y = height-1;
	if ( z >= depth ) z = depth-1;
	/* clamp color */
	if ( r < 0.0 ) r = 0.0;
	if ( g < 0.0 ) g = 0.0;
	if ( b < 0.0 ) b = 0.0;
	if ( r > 1.0 ) r = 1.0;
	if ( g > 1.0 ) g = 1.0;
	if ( b > 1.0 ) b = 1.0;
	outdata[(x+y*width+z*width*height)*4] = (r*255);
	outdata[(x+y*width+z*width*height)*4+1] = (g*255);
	outdata[(x+y*width+z*width*height)*4+2] = (b*255);
	outdata[(x+y*width+z*width*height)*4+3] = 255;	/* always opaque */
}

int smoothenlut( void )
{
	int x = 0, y = 0, z = 0, i = 0, s = width*height*depth;
	float r1, g1, b1, r2, g2, b2;
	int j, k, l;
	while ( i < s )
	{
		r2 = g2 = b2 = 0.0;
		for ( l=-7; l<=7; l++ )
		{
			for ( k=-7; k<=7; k++ )
			{
				for ( j=-7; j<=7; j++ )
				{
					getpx(x+j,y+k,z+l,&r1,&g1,&b1);
					r2 += r1*gauss8[abs(j)]*gauss8[abs(k)]
						*gauss8[abs(l)];
					g2 += g1*gauss8[abs(j)]*gauss8[abs(k)]
						*gauss8[abs(l)];
					b2 += b1*gauss8[abs(j)]*gauss8[abs(k)]
						*gauss8[abs(l)];
				}
			}
		}
		putpx(x,y,z,r2,g2,b2);
		i++;
		x++;
		if ( x >= width )
		{
			x = 0;
			y++;
		}
		if ( y >= height )
		{
			y = 0;
			z++;
		}
		/* print a progress bar because this can take long */
		printf("\rlutsmooth: Applying blur... %3d%%",(i*100)/s);
	}
	putchar('\n');
	return 1;
}

int main( int argc, char **argv )
{
	printf("LUTSMOOTH 1.0 - (C)2017 Marisa Kirisame, UnSX Team.\n"
		"This program is free software under the GNU GPL v3.\n"
		"See https://www.gnu.org/licenses/gpl.html for details.\n\n");
	FILE *fin, *fout;
	char fname[255] = {0};
	ddsheader_t head;
	for ( int i=1; i<argc; i++ )
	{
		if ( !(fin = fopen(argv[i],"rb")) )
		{
			printf("Failed to load %s: %s\n",argv[i],
				strerror(errno));
			continue;
		}
		fread(&head,sizeof(ddsheader_t),1,fin);
		if ( head.magic != 0x20534444 )
		{
			printf("File %s is not a valid DDS file\n",argv[i]);
			fclose(fin);
			continue;
		}
		if ( (head.flags != 0x80100f) || (head.pf_flags != 0x41)
			|| (head.pf_fourcc != 0) || (head.pf_bitcount != 32)
			|| (head.mipmaps != 0) || (head.pf_rmask != 0xff)
			|| (head.width == 0) || (head.height == 0)
			|| (head.depth == 0) )
		{
			printf("%s has one or more unsupported properties.\n"
				"lutsmooth requires an uncompressed RGBA8 "
				"volume map with no mipmaps.\n"
				" flags: expected 80100f got %x\n"
				" pf_flags: expected 41 got %x\n"
				" pf_fourcc: expected 0 got %x\n"
				" pf_bitcount: expected 32 got %u\n"
				" mipmaps: expected 0 got %u\n"
				" pf_rmask: expected ff got %x\n"
				" width: expected nonzero got %u\n"
				" height: expected nonzero got %u\n"
				" depth: expected nonzero got %u\n",argv[i],
				head.flags,head.pf_flags,head.pf_fourcc,
				head.pf_bitcount,head.mipmaps,head.pf_rmask,
				head.width,head.height,head.depth);
			fclose(fin);
			continue;
		}
		width = head.width;
		height = head.height;
		depth = head.depth;
		indata = malloc(width*height*depth*4);
		outdata = malloc(width*height*depth*4);
		fread(indata,width*height*depth*4,1,fin);
		fclose(fin);
		int res = smoothenlut();
		free(indata);
		if ( !res )
		{
			printf("Failed to process %s\n",argv[i]);
			free(outdata);
			continue;
		}
		strcpy(fname,argv[i]);
		strcat(fname,".old");
		if ( rename(argv[i],fname) )
		{
			printf("Failed to rename %s to %s: %s\n",argv[i],fname,
				strerror(errno));
			free(outdata);
			continue;
		}
		if ( !(fout = fopen(argv[i],"wb")) )
		{
			printf("Failed to create %s: %s\n",fname,
				strerror(errno));
			free(outdata);
			continue;
		}
		fwrite(&head,sizeof(ddsheader_t),1,fout);
		fwrite(outdata,width*height*depth*4,1,fout);
		fclose(fout);
		free(outdata);
	}
	return 0;
}
