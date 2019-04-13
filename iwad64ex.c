/*
	iwad64ex.c : Extracts the embedded IWAD in Hexen 64.
	Currently does not decompress the data (or fix the endianness in data).
	(C)2019 Marisa Kirisame, UnSX Team.
	Released under the GNU General Public License version 3.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

typedef struct
{
	char sig[4];
	uint32_t nlumps;
	uint32_t dirpos;
} wad_t;

typedef struct
{
	uint32_t filepos, size;
	char name[8];
} lump_t;

char *data;
wad_t *wad;
lump_t *lumps;

uint32_t btl( uint32_t b )
{
	return ((b>>24)&0xff)|((b<<8)&0xff0000)
		|((b>>8)&0xff00)|((b<<24)&0xff000000);
}

int main( int argc, char **argv )
{
	if ( argc < 3 )
	{
		fprintf(stderr,"usage: iwad64ex <romfile> <wadfile>\n");
		return 1;
	}
	FILE *romfile = fopen(argv[1],"rb");
	if ( !romfile )
	{
		fprintf(stderr,"failed to open %s for reading: %s\n",
			argv[1],strerror(errno));
		return 2;
	}
	ssize_t fpos = 0;
	char buf[4096];
	ssize_t found = 0;
	while ( !feof(romfile) )
	{
		memset(buf,0,4096);
		int bsiz = fread(buf,1,4096,romfile);
		// to avoid GNU-exclusive memmem function I'll just roll out
		// my own thing here (fuckin' GNU and their exclusives...)
		for ( int i=0; i<bsiz-5; i++ )
		{
			if ( !memcmp(buf+i,"IWAD",4)
				|| !memcmp(buf+i,"PWAD",4) )
			{
				fpos += i;
				found = 1;
				break;
			}
		}
		if ( found ) break;
		fpos += bsiz;
	}
	if ( !found )
	{
		fclose(romfile);
		fprintf(stderr,"WAD signature not found in ROM\n");
		return 8;
	}
	printf("Embedded wad found at %#zx\n",fpos);
	data = malloc(12);
	if ( !data )
	{
		fclose(romfile);
		fprintf(stderr,"cannot allocate 12 bytes for wad header: %s",
			strerror(errno));
		return 16;
	}
	fseek(romfile,fpos,SEEK_SET);
	fread(data,1,12,romfile);
	wad = (wad_t*)data;
	// fix endianness
	wad->nlumps = btl(wad->nlumps);
	wad->dirpos = btl(wad->dirpos);
	// reallocate to fit
	size_t fullsiz = wad->dirpos+wad->nlumps*16;
	data = realloc(data,fullsiz);
	wad = (wad_t*)data;
	if ( errno )
	{
		fclose(romfile);
		free(data);
		fprintf(stderr,"cannot allocate %zd bytes for full wad: %s",
			fullsiz,strerror(errno));
		return 32;
	}
	fread(data+12,1,fullsiz-12,romfile);
	lumps = (lump_t*)(data+wad->dirpos);
	// loop and fix endianness
	for ( int i=0; i<wad->nlumps; i++ )
	{
		lumps[i].filepos = btl(lumps[i].filepos);
		lumps[i].size = btl(lumps[i].size);
		printf("%.8s %d %d\n",lumps[i].name,lumps[i].size,
			lumps[i].filepos);
	}
	fclose(romfile);
	FILE *wadfile = fopen(argv[2],"wb");
	if ( !wadfile )
	{
		fprintf(stderr,"failed to open %s for writing: %s\n",
			argv[2],strerror(errno));
		return 4;
	}
	fwrite(data,1,fullsiz,wadfile);
	fclose(wadfile);
	free(data);
	printf("wrote %s, %d bytes\n",argv[2],fullsiz);
	return 0;
}
