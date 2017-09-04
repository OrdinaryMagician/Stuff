/*
	mkvolume.c : Hastily written program to turn 2D LUTs into volume maps.
	Reads raw RGB8 data and writes uncompressed RGB8 DDS because I'm lazy.
	(C)2015 Marisa Kirisame, UnSX Team.
	Released under the MIT License (see bottom of file).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define dword __UINT32_TYPE__
#define byte __UINT8_TYPE__

typedef struct
{
	char magic[4];
	dword size, flags, height, width, pitch, depth, mipmaps, reserved1[11],
	pf_size, pf_flags;
	char pf_fourcc[4];
	dword pf_bitcount, pf_rmask, pf_gmask, pf_bmask, pf_amask, caps[4],
		reserved2;
} __attribute__((packed)) ddsheader_t;

ddsheader_t head =
{
	.magic = "DDS ", .size = 124,
	.flags = 0x0080100F, /* caps|height|width|pitch|pixelformat|depth */
	.height = 0, .width = 0, .pitch = 0, .depth = 0, /* set later */
	.mipmaps = 0, .reserved1 = {0,0,0,0,0,0,0,0,0,0,0}, .pf_size = 32,
	.pf_flags = 0x40 /* uncompressed */, .pf_fourcc = {0}, /* N/A */
	.pf_bitcount = 24, .pf_rmask = 0xff, .pf_gmask = 0xff00,
	.pf_bmask = 0xff0000, .pf_amask = 0, /* RGB8 mask, no alpha */
	.caps = {0x1000,0x200000,0,0}, /* texture, volume */ .reserved2 = 0
};

typedef struct { byte r,g,b; } __attribute__((packed)) pixel_t;

pixel_t *tex = 0;
unsigned length = 0, numLUTs = 0;
#define area (length*length)
#define volume (length*length*length)
FILE *dat;

int readLUT( unsigned n )
{
	char fn[256];
	snprintf(fn,256,"lutout_%u.dds",n);
	FILE *lf = fopen(fn,"w");
	if ( !lf )
	{
		fprintf(stderr,"Could not open %s: %s\n",fn,strerror(errno));
		return 0;
	}
	unsigned c = fwrite(&head,1,128,lf);
	/* header goes here */
	if ( c != 128 )
	{
		fprintf(stderr,"Failed to write header (%u != %u)\n",c,128);
		return 0;
	}
	/* read entire LUT into memory */
	c = fread(tex,1,3*volume,dat);
	if ( c != 3*volume )
	{
		fprintf(stderr,"Failed to read data (%u != %u)\n",c,3*volume);
		return 0;
	}
	/* write slices */
	for ( unsigned i=0; i<length; i++ ) for ( unsigned j=0; j<length; j++ )
	{
		c = fwrite(&tex[i*length+j*area],1,3*length,lf);
		if ( c == 3*length ) continue;
		fprintf(stderr,"Failed to write line %u, layer %u (%u != %u)",
			j,i,c,3*length);
	}
	fclose(lf);
	return 1;
}

int main( int argc, char **argv )
{
	if ( argc < 4 )
	{
		fprintf(stderr,"Usage: %s <file> <length> <numLUTs>",argv[0]);
		fprintf(stderr,"  Data is read as raw RGB8\n");
		return 1;
	}
	dat = fopen(argv[1],"r");
	if ( !dat )
	{
		fprintf(stderr,"Could not open %s: %s\n",argv[1],
			strerror(errno));
		return 1;
	}
	sscanf(argv[2],"%u",&length);
	sscanf(argv[3],"%u",&numLUTs);
	head.width = head.height = head.depth = length;
	head.pitch = length*3;
	tex = malloc(3*length*length*length);
	printf("Ready to read %u %u³ LUTs from stdin\n",numLUTs,length);
	printf("%u bytes each, %u bytes total\n",3*volume,3*volume*numLUTs);
	for ( unsigned i=0; i<numLUTs; i++ )
		printf("Table %u %s\n",i,readLUT(i)?"OK":"FAIL");
	free(tex);
	fclose(dat);
	return 0;
}

/*
Copyright (c) 2015 UnSX Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
