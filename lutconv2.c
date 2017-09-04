/*
	lutconv2.c : Improved, general purpose LUT converter.
	Turns 2D LUT textures into DDS Volume Maps for convenience.
	(C)2017 Marisa Kirisame, UnSX Team.
	Released under the GNU General Public License version 3 (or later).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <png.h>

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
ddsheader_t ddshead =
{
	.magic = 0x20534444, .size = 124, .flags = 0x0080100f, .height = 0,
	.width = 0, .pitch = 0, .depth = 0, .mipmaps = 0,
	.reserved1 = {0,0,0,0,0,0,0,0,0,0,0}, .pf_size = 32, .pf_flags = 0x41,
	.pf_fourcc = 0, .pf_bitcount = 32, .pf_rmask = 0xff,
	.pf_gmask = 0xff00, .pf_bmask = 0xff0000, .pf_amask = 0xff000000,
	.caps = {0x1000,0x200000,0,0}, .reserved2 = 0
};

uint8_t *indata, *outdata;
int width, height, depth, alpha;

/* vertical strip LUT, the format I use */
int vertconv( void )
{
	int x, y, z, i, s;
	ddshead.pf_flags = 0x41;
	ddshead.pf_bitcount = 32;
	/* guess the format */
	if ( (width == 8) && (height == 4096) )
	{
		/* VGA 8x16 font table */
		printf("conv: VGA 8x16 font table\n");
		ddshead.width = width = 8;
		ddshead.height = height = 16;
		ddshead.depth = depth = 256;
		ddshead.pitch = 8*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 0;
		while ( i < s )
		{
			outdata[i++] = indata[x++];
			outdata[i++] = indata[x++];
			outdata[i++] = indata[x++];
			if ( alpha ) outdata[i++] = indata[x++];
			else outdata[i++] = 255;
		}
		return 1;
	}
	else if ( (width == 16) && (height == 256) )
	{
		/* unpadded 16x16x16 LUT */
		printf("conv: vertical 16x16x16 LUT\n");
		ddshead.width = width = 16;
		ddshead.height = height = 16;
		ddshead.depth = depth = 16;
		ddshead.pitch = 16*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 0;
		while ( i < s )
		{
			outdata[i++] = indata[x++];
			outdata[i++] = indata[x++];
			outdata[i++] = indata[x++];
			if ( alpha ) outdata[i++] = indata[x++];
			else outdata[i++] = 255;
		}
		return 1;
	}
	else if ( (width == 32) && (height == 512) )
	{
		/* padded 16x16x16 LUT */
		printf("conv: vertical padded 16x16x16 LUT\n");
		ddshead.width = width = 16;
		ddshead.height = height = 16;
		ddshead.depth = depth = 16;
		ddshead.pitch = 16*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 8;
		y = 8;
		z = 0;
		while ( i < s )
		{
			outdata[i++] = indata[(x+y*32+z*512)*(3+alpha)];
			outdata[i++] = indata[(x+y*32+z*512)*(3+alpha)+1];
			outdata[i++] = indata[(x+y*32+z*512)*(3+alpha)+2];
			if ( alpha )
				outdata[i++] = indata[(x+y*32+z*512)
					*(3+alpha)+3];
			else outdata[i++] = 255;
			x++;
			if ( x >= 24 )
			{
				x = 8;
				y++;
			}
			if ( y >= 24 )
			{
				y = 8;
				z++;
			}
		}
		return 1;
	}
	else if ( (width == 64) && (height == 64) )
	{
		/* square 16x16x16 LUT (assuming column-major grid) */
		printf("conv: column-major 4x4 grid 16x16x16 LUT\n");
		ddshead.width = width = 16;
		ddshead.height = height = 16;
		ddshead.depth = depth = 16;
		ddshead.pitch = 16*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 0;
		y = 0;
		z = 0;
		while ( i < s )
		{
			outdata[i++] = indata[(x+y*64+(z&0x3)*1024
				+(z&0xc)*4)*(3+alpha)];
			outdata[i++] = indata[(x+y*64+(z&0x3)*1024
				+(z&0xc)*4)*(3+alpha)+1];
			outdata[i++] = indata[(x+y*64+(z&0x3)*1024
				+(z&0xc)*4)*(3+alpha)+2];
			if ( alpha )
				outdata[i++] = indata[indata[(x+y*64+(z&0x3)
					*1024+(z&0xc)*4)*(3+alpha)+3]];
			else outdata[i++] = 255;
			x++;
			if ( x >= 16 )
			{
				x = 0;
				y++;
			}
			if ( y >= 16 )
			{
				y = 0;
				z++;
			}
		}
		return 1;
	}
	else if ( (width == 64) && (height == 4096) )
	{
		/* unpadded 64x64x64 LUT */
		printf("conv: vertical 64x64x64 LUT\n");
		ddshead.width = width = 64;
		ddshead.height = height = 64;
		ddshead.depth = depth = 64;
		ddshead.pitch = 64*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 0;
		while ( i < s )
		{
			outdata[i++] = indata[x++];
			outdata[i++] = indata[x++];
			outdata[i++] = indata[x++];
			if ( alpha ) outdata[i++] = indata[x++];
			else outdata[i++] = 255;
		}
		return 1;
	}
	else if ( (width == 128) && (height == 8192) )
	{
		/* padded 64x64x64 LUT */
		printf("conv: vertical padded 64x64x64 LUT\n");
		ddshead.width = width = 64;
		ddshead.height = height = 64;
		ddshead.depth = depth = 64;
		ddshead.pitch = 64*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 32;
		y = 32;
		z = 0;
		while ( i < s )
		{
			outdata[i++] = indata[(x+y*128+z*16384)*(3+alpha)];
			outdata[i++] = indata[(x+y*128+z*16384)*(3+alpha)+1];
			outdata[i++] = indata[(x+y*128+z*16384)*(3+alpha)+2];
			if ( alpha )
				outdata[i++] = indata[(x+y*128+z*16384)
					*(3+alpha)+3];
			else outdata[i++] = 255;
			x++;
			if ( x >= 96 )
			{
				x = 32;
				y++;
			}
			if ( y >= 96 )
			{
				y = 32;
				z++;
			}
		}
		return 1;
	}
	else if ( (width == 512) && (height == 512) )
	{
		/* square 64x64x64 LUT (assuming column-major grid) */
		printf("conv: column-major 8x8 grid 64x64x64 LUT\n");
		ddshead.width = width = 64;
		ddshead.height = height = 64;
		ddshead.depth = depth = 64;
		ddshead.pitch = 64*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 0;
		y = 0;
		z = 0;
		while ( i < s )
		{
			outdata[i++] = indata[(x+y*512+(z&0x7)*32768
				+(z&0x38)*8)*(3+alpha)];
			outdata[i++] = indata[(x+y*512+(z&0x7)*32768
				+(z&0x38)*8)*(3+alpha)+1];
			outdata[i++] = indata[(x+y*512+(z&0x7)*32768
				+(z&0x38)*8)*(3+alpha)+2];
			if ( alpha )
				outdata[i++] = indata[indata[(x+y*512+(z&0x7)
					*32768+(z&0x38)*16)*(3+alpha)+3]];
			else outdata[i++] = 255;
			x++;
			if ( x >= 64 )
			{
				x = 0;
				y++;
			}
			if ( y >= 64 )
			{
				y = 0;
				z++;
			}
		}
		return 1;
	}
	else if ( (width == 256) && (height == 65536) )
	{
		/* unpadded 256x256x256 LUT (rare) */
		printf("conv: vertical 256x256x256 LUT\n");
		ddshead.width = width = 256;
		ddshead.height = height = 256;
		ddshead.depth = depth = 256;
		ddshead.pitch = 256*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 0;
		while ( i < s )
		{
			outdata[i++] = indata[x++];
			outdata[i++] = indata[x++];
			outdata[i++] = indata[x++];
			if ( alpha ) outdata[i++] = indata[x++];
			else outdata[i++] = 255;
		}
		return 1;
	}
	else if ( (width == 512) && (height == 131072) )
	{
		/* padded 256x256x256 LUT (very rare) */
		printf("conv: vertical padded 256x256x256 LUT\n");
		ddshead.width = width = 256;
		ddshead.height = height = 256;
		ddshead.depth = depth = 256;
		ddshead.pitch = 256*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 128;
		y = 128;
		z = 0;
		while ( i < s )
		{
			outdata[i++] = indata[(x+y*512+z*262144)*(3+alpha)];
			outdata[i++] = indata[(x+y*512+z*262144)*(3+alpha)+1];
			outdata[i++] = indata[(x+y*512+z*262144)*(3+alpha)+2];
			if ( alpha )
				outdata[i++] = indata[(x+y*512+z*262144)
					*(3+alpha)+3];
			else outdata[i++] = 255;
			x++;
			if ( x >= 384 )
			{
				x = 128;
				y++;
			}
			if ( y >= 384 )
			{
				y = 128;
				z++;
			}
		}
		return 1;
	}
	else if ( (width == 4096) && (height == 4096) )
	{
		/* square 256x256x256 LUT (assuming column-major grid) */
		printf("conv: column-major 16x16 grid 256x256x256 LUT\n");
		ddshead.width = width = 256;
		ddshead.height = height = 256;
		ddshead.depth = depth = 256;
		ddshead.pitch = 256*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 0;
		y = 0;
		z = 0;
		while ( i < s )
		{
			outdata[i++] = indata[(x+y*4096+(z&0x0F)*1048576
				+(z&0xF0)*16)*(3+alpha)];
			outdata[i++] = indata[(x+y*4096+(z&0x0F)*1048576
				+(z&0xF0)*16)*(3+alpha)+1];
			outdata[i++] = indata[(x+y*4096+(z&0x0F)*1048576
				+(z&0xF0)*16)*(3+alpha)+2];
			if ( alpha )
				outdata[i++] = indata[indata[(x+y*4096+(z&0x0F)
					*1048576+(z&0xF0)*16)*(3+alpha)+3]];
			else outdata[i++] = 255;
			x++;
			if ( x >= 256 )
			{
				x = 0;
				y++;
			}
			if ( y >= 256 )
			{
				y = 0;
				z++;
			}
		}
		return 1;
	}
	printf("conv: Never heard of a LUT texture of %ux%u\n",width,height);
	return 0;
}

/* horizontal strip LUT, what everyone else except me uses */
int horconv( void )
{
	int x, y, z, i, s;
	ddshead.pf_flags = 0x41;
	ddshead.pf_bitcount = 32;
	/* guess the format */
	if ( (width == 256) && (height == 16) )
	{
		/* 16x16x16 LUT */
		printf("conv: horizontal 16x16x16 LUT\n");
		ddshead.width = width = 16;
		ddshead.height = height = 16;
		ddshead.depth = depth = 16;
		ddshead.pitch = 16*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 0;
		y = 0;
		z = 0;
		while ( i < s )
		{
			outdata[i++] = indata[(x+y*256+z*16)*(3+alpha)];
			outdata[i++] = indata[(x+y*256+z*16)*(3+alpha)+1];
			outdata[i++] = indata[(x+y*256+z*16)*(3+alpha)+2];
			if ( alpha )
				outdata[i++] = indata[(x+y*256+z*16)
					*(3+alpha)+3];
			else outdata[i++] = 255;
			x++;
			if ( x >= 16 )
			{
				x = 0;
				y++;
			}
			if ( y >= 16 )
			{
				y = 0;
				z++;
			}
		}
		return 1;
	}
	if ( (width == 512) && (height == 32) )
	{
		/* padded 16x16x16 LUT */
		printf("conv: horizontal padded 16x16x16 LUT\n");
		ddshead.width = width = 16;
		ddshead.height = height = 16;
		ddshead.depth = depth = 16;
		ddshead.pitch = 16*4;
		s = width*height*depth*4;
		outdata = malloc(s);
		i = 0;
		x = 8;
		y = 8;
		z = 0;
		while ( i < s )
		{
			outdata[i++] = indata[(x+y*512+z*32)*(3+alpha)];
			outdata[i++] = indata[(x+y*512+z*32)*(3+alpha)+1];
			outdata[i++] = indata[(x+y*512+z*32)*(3+alpha)+2];
			if ( alpha )
				outdata[i++] = indata[(x+y*512+z*32)
					*(3+alpha)+3];
			else outdata[i++] = 255;
			x++;
			if ( x >= 24 )
			{
				x = 8;
				y++;
			}
			if ( y >= 24 )
			{
				y = 8;
				z++;
			}
		}
		return 1;
	}
	printf("conv: Never heard of a LUT texture of %ux%u\n",width,height);
	return 0;
}

/* png loader code, boilerplate written according to official docs */
int loadpng( const char *filename )
{
	if ( !filename )
	{
		printf("png: no filename passed\n");
		return 1;
	}
	png_structp pngp;
	png_infop infp;
	unsigned int sread = 0;
	int col, inter;
	FILE *pf;
	if ( !(pf = fopen(filename,"rb")) )
	{
		printf("png: could not open file: %s\n",strerror(errno));
		return 1;
	}
	unsigned char header[8];
	fread(header,8,1,pf);
	if ( png_sig_cmp(header,0,8) )
	{
		printf("png: bad signature, not a PNG file\n");
		return 1;
	}
	sread += 8;
	pngp = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if ( !pngp )
	{
		printf("png: failed to create png_struct\n");
		fclose(pf);
		return 1;
	}
	infp = png_create_info_struct(pngp);
	if ( !infp )
	{
		printf("png: failed to create png_info\n");
		fclose(pf);
		png_destroy_read_struct(&pngp,0,0);
		return 1;
	}
	if ( setjmp(png_jmpbuf(pngp)) )
	{
		printf("png: failed to set up error handler\n");
		png_destroy_read_struct(&pngp,&infp,0);
		fclose(pf);
		return 1;
	}
	png_init_io(pngp,pf);
	png_set_sig_bytes(pngp,sread);
	png_read_png(pngp,infp,PNG_TRANSFORM_STRIP_16|PNG_TRANSFORM_PACKING
		|PNG_TRANSFORM_EXPAND,0);
	png_uint_32 w,h;
	int dep;
	png_get_IHDR(pngp,infp,&w,&h,&dep,&col,&inter,0,0);
	if ( (col != PNG_COLOR_TYPE_RGB) && (col != PNG_COLOR_TYPE_RGB_ALPHA) )
	{
		printf("png: only RGB or RGBA are supported\n");
		png_destroy_read_struct(&pngp,&infp,0);
		fclose(pf);
		return 1;
	}
	if ( dep != 8 )
	{
		printf("png: only 8-bit channels are supported\n");
		png_destroy_read_struct(&pngp,&infp,0);
		fclose(pf);
		return 1;
	}
	width = w;
	height = h;
	alpha = (col==PNG_COLOR_TYPE_RGB_ALPHA);
	int rbytes = png_get_rowbytes(pngp,infp);
	indata = malloc(rbytes*h);
	png_bytepp rptr = png_get_rows(pngp,infp);
	for ( unsigned i=0; i<h; i++ )
		memcpy(indata+(rbytes*i),rptr[i],rbytes);
	png_destroy_read_struct(&pngp,&infp,0);
	fclose(pf);
	return 0;
}

int main( int argc, char **argv )
{
	printf("LUTCONV 2.0 - (C)2017 Marisa Kirisame, UnSX Team.\n"
		"This program is free software under the GNU GPL v3.\n"
		"See https://www.gnu.org/licenses/gpl.html for details.\n\n");
	if ( argc < 2 )
	{
		printf("No files have been passed for processing.\n");
		return 1;
	}
	FILE *fout;
	char fname[255] = {0};
	char *ext;
	for ( int i=1; i<argc; i++ )
	{
		if ( loadpng(argv[i]) )
		{
			printf("Failed to load %s\n",argv[i]);
			continue;
		}
		int res = (width>height)?horconv():vertconv();
		free(indata);
		if ( !res )
		{
			printf("Failed to process %s\n",argv[i]);
			continue;
		}
		strcpy(fname,argv[i]);
		ext = strrchr(fname,'.');
		if ( ext ) *ext = 0;
		strcat(fname,".dds");
		fout = fopen(fname,"wb");
		if ( !fout )
		{
			printf("Cannot open %s: %s\n",fname,strerror(errno));
			free(outdata);
			continue;
		}
		fwrite(&ddshead,sizeof(ddsheader_t),1,fout);
		fwrite(outdata,width*height*depth*4,1,fout);
		fclose(fout);
		free(outdata);
	}
	return 0;
}
