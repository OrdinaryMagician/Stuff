/*
	lutflat.c : LUT flattener.
	Converts DDS Volume Map LUTs back into flat textures.
	(C)2017 Marisa Kirisame, UnSX Team.
	Released under the MIT license.
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

uint8_t *indata, *outdata;
int width, height, depth;
unsigned pngw, pngh;

int flattenlut( void )
{
	int x = 0, y = 0, z = 0, i = 0, s = width*height*depth*4;
	/* guess the format */
	if ( (width == 256) && (height == 256) && (depth == 256) )
	{
		printf("flatten: 256x256x256 table\n");
		pngw = pngh = 4096;
		while ( i < s )
		{
			outdata[(x+y*4096+(z&0x0F)*1048576+(z&0xF0)*16)*3]
				= indata[i++];
			outdata[(x+y*4096+(z&0x0F)*1048576+(z&0xF0)*16)*3+1]
				= indata[i++];
			outdata[(x+y*4096+(z&0x0F)*1048576+(z&0xF0)*16)*3+2]
				= indata[i++];
			i++;
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
	else if ( (width == 64) && (height == 64) && (depth == 64) )
	{
		printf("flatten: 64x64x64 table\n");
		pngw = pngh = 512;
		while ( i < s )
		{
			outdata[(x+y*512+(z&0x7)*32768+(z&0x38)*8)*3]
				= indata[i++];
			outdata[(x+y*512+(z&0x7)*32768+(z&0x38)*8)*3+1]
				= indata[i++];
			outdata[(x+y*512+(z&0x7)*32768+(z&0x38)*8)*3+2]
				= indata[i++];
			i++;
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
	else if ( (width == 32) && (height == 32) && (depth == 32) )
	{
		printf("flatten: 32x32x32 table\n");
		pngw = 1024;
		pngh = 32;
		while ( i < s )
		{
			outdata[(x+y*1024+z*32)*3] = indata[i++];
			outdata[(x+y*1024+z*32)*3+1] = indata[i++];
			outdata[(x+y*1024+z*32)*3+2] = indata[i++];
			i++;
			x++;
			if ( x >= 32 )
			{
				x = 0;
				y++;
			}
			if ( y >= 32 )
			{
				y = 0;
				z++;
			}
		}
		return 1;
	}
	else if ( (width == 16) && (height == 16) && (depth == 16) )
	{
		printf("flatten: 16x16x16 table\n");
		pngw = pngh = 64;
		while ( i < s )
		{
			outdata[(x+y*64+(z&0x3)*1024+(z&0xc)*4)*3]
				= indata[i++];
			outdata[(x+y*64+(z&0x3)*1024+(z&0xc)*4)*3+1]
				= indata[i++];
			outdata[(x+y*64+(z&0x3)*1024+(z&0xc)*4)*3+2]
				= indata[i++];
			i++;
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
	printf("flat: Unsupported dimensions %ux%ux%u\n",width,height,depth);
	return 0;
}

int savepng( const char *filename )
{
	if ( !filename )
	{
		printf("png: no filename passed\n");
		return 1;
	}
	png_structp pngp;
	png_infop infp;
	FILE *pf;
	if ( !(pf = fopen(filename,"wb")) )
	{
		printf("png: could not open file: %s\n",strerror(errno));
		return 1;
	}
	pngp = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
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
		png_destroy_write_struct(&pngp,&infp);
		return 1;
	}
	if ( setjmp(png_jmpbuf(pngp)) )
	{
		printf("png: failed to set up error handler\n");
		png_destroy_write_struct(&pngp,&infp);
		fclose(pf);
		return 1;
	}
	png_init_io(pngp,pf);
	png_set_IHDR(pngp,infp,pngw,pngh,8,PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE);
	png_write_info(pngp,infp);
	png_bytepp rptr = malloc(sizeof(png_bytep)*pngh);
	for ( unsigned i=0; i<pngh; i++ )
		rptr[i] = outdata+i*pngw*3;
	png_set_rows(pngp,infp,rptr);
	png_write_png(pngp,infp,PNG_TRANSFORM_IDENTITY,0);
	png_destroy_write_struct(&pngp,&infp);
	free(rptr);
	return 0;
}

int main( int argc, char **argv )
{
	printf("LUTFLAT 1.0 - (C)2017 Marisa Kirisame, UnSX Team.\n"
		"This program is free software under the GNU GPL v3.\n"
		"See https://www.gnu.org/licenses/gpl.html for details.\n\n");
	if ( argc < 2 )
	{
		printf("No files have been passed for processing.\n");
		return 1;
	}
	FILE *fin;
	char fname[255] = {0};
	char *ext;
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
				"lutflat requires an uncompressed RGBA8 "
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
		outdata = malloc(width*height*depth*3);
		fread(indata,width*height*depth*4,1,fin);
		fclose(fin);
		int res = flattenlut();
		free(indata);
		if ( !res )
		{
			printf("Failed to process %s\n",argv[i]);
			free(outdata);
			continue;
		}
		strcpy(fname,argv[i]);
		ext = strrchr(fname,'.');
		if ( ext ) *ext = 0;
		strcat(fname,".png");
		if ( savepng(fname) )
			printf("Failed to save %s\n",fname);
		free(outdata);
	}
	return 0;
}
