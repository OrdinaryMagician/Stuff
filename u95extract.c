/*
	extract text, meshes, textures, palettes and sounds from 1995 packages.
	TODO: extract animseq and meshmap data for meshes
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <png.h>

#define NAMES_MARK       "[REFERENCED NAMES]"
#define IMPORT_MARK      "[IMPORTED RESOURCES]"
#define EXPORT_MARK      "[EXPORTED RESOURCES]"
#define DATA_MARK        "[EXPORTED RESOURCE DATA]"
#define HEADER_MARK      "[EXPORTED RESOURCE HEADERS]"
#define TABLE_MARK       "[EXPORTED RESOURCE TABLE]"
#define TRAILER_MARK     "[SUMMARY]"
#define RES_FILE_TAG     "Unreal Resource\0"

enum EResourceType
{
	RES_None		= 0,	// No resource.
	RES_Buffer		= 1,	// A large binary object holding misc saveable data.
	RES_Array		= 3,	// An array of resources.
	RES_TextBuffer	= 4,	// A text buffer.
	RES_Texture		= 5,	// Texture or compound texture.
	RES_Font		= 6,	// Font for use in game.
	RES_Palette		= 7,	// A palette.
	RES_Script		= 9,	// Script.
	RES_Class		= 10,	// An actor class.
	RES_ActorList		= 11,	// An array of actors.
	RES_Sound		= 12,	// Sound effect.
	RES_Mesh		= 14,	// Animated mesh.
	RES_Vectors		= 16,	// 32-bit floating point vector list.
	RES_BspNodes		= 17,	// Bsp node list.
	RES_BspSurfs		= 18,	// Bsp polygon list.
	RES_LightMesh		= 19,	// Bsp polygon lighting mesh.
	RES_Polys		= 20,	// Editor polygon list.
	RES_Model		= 21,	// Model or level map.
	RES_Level		= 22,	// A game level.
	RES_Camera		= 25,	// A rendering camera on this machine.
	RES_Player		= 28,	// A remote player logged into the local server.
	RES_VertPool		= 29,	// A vertex pool corresponding to a Bsp and FPoints/FVectors table.
	RES_Ambient		= 30,	// An ambient sound definition.
	RES_TransBuffer	= 31,	// Transaction tracking buffer.
	RES_MeshMap		= 32,	// MeshMap.
	RES_Bounds		= 33,	// Bounding structure.
	RES_Terrain		= 34,	// Terrain.
	RES_Enum		= 35,	// Enumeration (array of FName's).
};

typedef struct
{
	uint16_t version, nexports, nimports, nnames;
	uint32_t onames, oimports, oheaders, oexports;
	uint8_t pad[32];
	char tag[16];
} __attribute__((packed)) resourcefiletrailer_t;

typedef struct
{
	char name[16];
	uint16_t unused;
} __attribute__((packed)) nameentry_t;

typedef struct
{
	char name[16];
	uint16_t index;
	uint8_t type, unknown;
	uint8_t pad1[6];
	uint16_t headersize;
	uint8_t pad2[10];
	uint32_t dataofs, datasize;
	uint8_t pad3[8];
} __attribute__((packed)) resnamefileentry_t;

typedef struct
{
	uint8_t r, g, b, x;
} __attribute__((packed)) color_t;

// global stuff
FILE *f;
resourcefiletrailer_t tail;
nameentry_t *names;
resnamefileentry_t *imports;
resnamefileentry_t *exports;
size_t *eheaders;

int writepng( const char *filename, unsigned char *fdata, int fw, int fh,
	png_color *fpal, int fpalsiz )
{
	if ( !filename ) return 0;
	png_structp pngp;
	png_infop infp;
	FILE *pf;
	if ( !(pf = fopen(filename,"wb")) ) return 0;
	pngp = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if ( !pngp )
	{
		fclose(pf);
		return 0;
	}
	infp = png_create_info_struct(pngp);
	if ( !infp )
	{
		fclose(pf);
		png_destroy_write_struct(&pngp,0);
		return 0;
	}
	if ( setjmp(png_jmpbuf(pngp)) )
	{
		png_destroy_write_struct(&pngp,&infp);
		fclose(pf);
		return 0;
	}
	png_init_io(pngp,pf);
	if ( fpal )
	{
		png_set_IHDR(pngp,infp,fw,fh,8,PNG_COLOR_TYPE_PALETTE,
			PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
		png_set_PLTE(pngp,infp,fpal,fpalsiz);
		unsigned char t = 0;
		png_set_tRNS(pngp,infp,&t,1,0);
		png_write_info(pngp,infp);
		for ( int i=0; i<fh; i++ ) png_write_row(pngp,fdata+(fw*i));
	}
	else
	{
		png_set_IHDR(pngp,infp,fw,fh,8,PNG_COLOR_TYPE_RGB,
			PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
		png_write_info(pngp,infp);
		for ( int i=0; i<fh; i++ ) png_write_row(pngp,fdata+(fw*3*i));
	}
	png_write_end(pngp,infp);
	png_destroy_write_struct(&pngp,&infp);
	fclose(pf);
	return 1;
}

void extract_pal( uint32_t i )
{
	mkdir("Palettes",0755);
	color_t cpal[256];
	png_color fpal[256];
	uint8_t dat[16384];
	fseek(f,exports[i].dataofs,SEEK_SET);
	fread(cpal,sizeof(color_t),256,f);
	for ( int i=0; i<256; i++ )
	{
		fpal[i].red = cpal[i].r;
		fpal[i].green = cpal[i].g;
		fpal[i].blue = cpal[i].b;
	}
	for ( int y=0; y<128; y++ ) for ( int x=0; x<128; x++ )
	{
		dat[x+y*128] = (x/8)+(y/8)*16;
	}
	char fname[256];
	snprintf(fname,256,"Palettes/%.16s.png",exports[i].name);
	writepng(fname,dat,128,128,fpal,256);
}

png_color defpal[256] =
{
	{  0,  0,  0}, {128,  0,  0}, {  0,128,  0}, {128,128,  0},
	{  0,  0,128}, {128,  0,128}, {  0,128,128}, {192,192,192},
	{192,220,192}, {166,202,240}, {  0,255,255}, {  0,255,255},
	{  0,255,255}, {  0,255,255}, {  0,255,255}, {  0,255,255},
	{255,227,203}, {255,215,179}, {211,179,151}, {243,179,123},
	{239,219,131}, {215,115, 59}, {131,175,215}, {223,187,127},
	{235,215,199}, {231,203,175}, {203,163,135}, {223,163,107},
	{207,207,107}, {207, 91, 43}, {115,163,207}, {219,175, 95},
	{219,207,195}, {211,191,171}, {195,151,127}, {203,151, 95},
	{171,191, 91}, {199, 67, 31}, {103,151,199}, {223,159, 47},
	{199,195,187}, {191,179,167}, {187,139,115}, {187,139, 83},
	{143,175, 79}, {191, 43, 19}, { 91,139,191}, {211,139, 39},
	{183,183,183}, {183,171,159}, {183,131,107}, {179,131, 75},
	{135,163, 75}, {183, 19,  7}, { 79,127,183}, {199,123, 31},
	{175,175,175}, {175,163,151}, {171,123, 99}, {167,123, 71},
	{127,155, 71}, {175,  0,  0}, { 71,119,175}, {187,107, 27},
	{163,163,163}, {163,151,139}, {159,115, 91}, {159,115, 67},
	{119,143, 67}, {163,  0,  0}, { 63,111,163}, {175, 95, 19},
	{151,151,151}, {151,139,127}, {151,103, 87}, {151,111, 63},
	{111,135, 63}, {151,  0,  0}, { 59,103,155}, {163, 79, 15},
	{139,139,139}, {139,127,119}, {139, 95, 79}, {139,103, 59},
	{103,127, 59}, {139,  0,  0}, { 55, 95,147}, {151, 67, 11},
	{131,131,131}, {131,119,107}, {127, 87, 71}, {131, 95, 55},
	{ 95,119, 55}, {131,  7,  0}, { 47, 87,135}, {139, 55,  7},
	{119,119,119}, {119,107, 99}, {119, 83, 67}, {123, 91, 51},
	{ 91,111, 51}, {119,  7,  0}, { 43, 83,127}, {131, 51,  7},
	{107,107,107}, {107, 95, 87}, {107, 75, 59}, {115, 87, 47},
	{ 87,103, 47}, {107,  7,  0}, { 39, 75,115}, {123, 43,  7},
	{ 99, 99, 99}, { 99, 87, 79}, { 99, 67, 55}, {103, 79, 43},
	{ 79, 95, 43}, { 99,  7,  0}, { 35, 67,107}, {115, 35,  0},
	{ 91, 91, 91}, { 91, 79, 75}, { 91, 63, 51}, { 95, 71, 39},
	{ 75, 87, 39}, { 91,  7,  0}, { 31, 63, 99}, {107, 27,  0},
	{ 83, 83, 83}, { 87, 75, 67}, { 83, 55, 47}, { 83, 63, 35},
	{ 67, 79, 39}, { 83,  7,  0}, { 27, 55, 91}, { 99, 23,  0},
	{ 79, 79, 79}, { 79, 71, 63}, { 79, 51, 43}, { 75, 55, 31},
	{ 63, 75, 35}, { 75,  0,  0}, { 23, 51, 83}, { 95, 19,  0},
	{ 71, 71, 71}, { 71, 63, 59}, { 71, 47, 39}, { 67, 51, 27},
	{ 59, 67, 31}, { 71,  0,  0}, { 23, 43, 75}, { 87, 15,  0},
	{ 63, 63, 63}, { 67, 59, 55}, { 67, 43, 35}, { 63, 47, 23},
	{ 51, 63, 27}, { 63,  0,  0}, { 19, 39, 71}, { 79, 11,  0},
	{ 59, 59, 59}, { 59, 55, 47}, { 59, 39, 31}, { 59, 43, 23},
	{ 47, 59, 27}, { 59,  0,  0}, { 19, 35, 63}, { 71,  7,  0},
	{ 51, 51, 51}, { 55, 47, 43}, { 55, 35, 27}, { 51, 39, 19},
	{ 43, 51, 23}, { 51,  0,  0}, { 15, 31, 55}, { 67,  7,  0},
	{ 47, 47, 47}, { 47, 43, 39}, { 47, 31, 27}, { 47, 35, 15},
	{ 39, 47, 19}, { 47,  0,  0}, { 15, 27, 51}, { 59,  0,  0},
	{ 43, 43, 43}, { 43, 39, 35}, { 43, 27, 23}, { 43, 31, 15},
	{ 35, 43, 19}, { 39,  0,  0}, { 11, 23, 43}, { 51,  0,  0},
	{ 39, 39, 39}, { 35, 31, 27}, { 35, 19, 15}, { 35, 27, 11},
	{ 27, 35, 11}, { 35,  0,  0}, {  7, 23, 35}, { 47,  0,  0},
	{ 35, 35, 35}, { 31, 27, 23}, { 31, 11, 11}, { 31, 23,  7},
	{ 23, 31,  7}, { 27,  0,  0}, {  7, 19, 31}, { 35,  0,  0},
	{ 27, 27, 27}, { 23, 23, 19}, { 23,  7,  7}, { 23, 19,  0},
	{ 19, 23,  7}, { 23,  0,  0}, {  0, 19, 23}, { 23,  0,  0},
	{ 23, 23, 23}, { 15, 15, 15}, { 19,  0,  0}, { 19, 15,  0},
	{ 15, 19,  0}, { 15,  0,  0}, {  0, 15, 19}, { 15, 15,  0},
	{ 15, 15, 15}, { 11, 11,  7}, { 11,  0,  0}, { 11, 11,  0},
	{ 11, 11,  0}, { 11,  0,  0}, {  0, 11, 11}, {  0, 11,  0},
	{  7,  7,  7}, {  7,  7,  7}, {  7,  0,  0}, {  7,  7,  0},
	{  7,  7,  0}, {  7,  0,  0}, {  0,  7,  7}, {  0,  7,  7},
	{  0,255,255}, {  0,255,255}, {  0,255,255}, {  0,255,255},
	{  0,255,255}, {  0,255,255}, {255,251,240}, {160,160,164},
	{128,128,128}, {255,  0,  0}, {  0,255,  0}, {255,255,  0},
	{  0,  0,255}, {255,  0,255}, {  0,255,255}, {255,255,255},
};

void extract_tex( uint32_t i )
{
	mkdir("Textures",0755);
	fseek(f,eheaders[i],SEEK_SET);
	uint16_t width, height;
	fread(&width,2,1,f);
	fread(&height,2,1,f);
	uint8_t *pxdata = malloc(width*height);
	fseek(f,exports[i].dataofs,SEEK_SET);
	fread(pxdata,1,width*height,f);
	char fname[256];
	snprintf(fname,256,"Textures/%.16s.png",exports[i].name);
	writepng(fname,pxdata,width,height,&defpal[0],256);
	free(pxdata);
}

void debug_printheader( uint32_t i )
{
	uint8_t *header = malloc(exports[i].headersize);
	fseek(f,eheaders[i],SEEK_SET);
	fread(header,exports[i].headersize,1,f);
	int j = 0;
	printf("%.16s header (%u):\n",exports[i].name,exports[i].headersize);
	for ( j=0; j<exports[i].headersize; j++ )
	{
		if ( !(j%8) ) printf("  ");
		printf("%02x%c",header[j],((j+1)%16)?' ':'\n');
	}
	if ( j%16 ) printf("\n");
	free(header);
}

void debug_printdata( uint32_t i )
{
	uint8_t *data = malloc(exports[i].datasize);
	fseek(f,exports[i].dataofs,SEEK_SET);
	fread(data,exports[i].datasize,1,f);
	int j = 0;
	printf("%.16s data (%u):\n",exports[i].name,exports[i].datasize);
	for ( j=0; j<exports[i].datasize; j++ )
	{
		if ( !(j%8) ) printf("  ");
		printf("%02x%c",data[j],((j+1)%16)?' ':'\n');
	}
	if ( j%16 ) printf("\n");
	free(data);
}

typedef struct
{
	uint16_t numpolys;
	uint16_t numverts;
	/* everything below is unused */
	uint16_t bogusrot;
	uint16_t bogusframe;
	uint32_t bogusnorm[3];
	uint32_t fixscale;
	uint32_t unused[3];
	uint8_t padding[12];
} __attribute__((packed)) dataheader_t;
typedef struct
{
	uint16_t vertices[3];
	uint8_t type;
	uint8_t color; /* unused */
	uint8_t uv[3][2];
	uint8_t texnum;
	uint8_t flags; /* unused */
} __attribute__((packed)) datapoly_t;
typedef struct
{
	uint16_t numframes;
	uint16_t framesize;
} __attribute__((packed)) aniheader_t;

void extract_msh( uint32_t i )
{
	/*debug_printheader(i);
	debug_printdata(i);
	return*/
	mkdir("Models",0755);
	fseek(f,eheaders[i],SEEK_SET);
	uint16_t npolys, nverts, nframes;
	fread(&npolys,2,1,f);
	fseek(f,2,SEEK_CUR);
	fread(&nverts,2,1,f);
	fseek(f,2,SEEK_CUR);
	fread(&nframes,2,1,f);
	char fname[256];
	FILE *out;
	// write datafile
	snprintf(fname,256,"Models/%.16s_d.3d",exports[i].name);
	out = fopen(fname,"wb");
	dataheader_t dhead =
	{
		.numpolys = npolys,
		.numverts = nverts,
	};
	fwrite(&dhead,sizeof(dataheader_t),1,out);
	fseek(f,exports[i].dataofs,SEEK_SET);
	datapoly_t *dpoly = calloc(sizeof(datapoly_t),dhead.numpolys);
	fread(dpoly,sizeof(datapoly_t),dhead.numpolys,f);
	fwrite(dpoly,sizeof(datapoly_t),dhead.numpolys,out);
	free(dpoly);
	fclose(out);
	// write anivfile
	snprintf(fname,256,"Models/%.16s_a.3d",exports[i].name);
	out = fopen(fname,"wb");
	aniheader_t ahead =
	{
		.numframes = nframes,
		.framesize = nverts*4,
	};
	fwrite(&ahead,sizeof(aniheader_t),1,out);
	uint32_t *uverts = calloc(sizeof(uint32_t),dhead.numverts
		*ahead.numframes);
	fread(uverts,sizeof(uint32_t),dhead.numverts*ahead.numframes,f);
	fwrite(uverts,sizeof(uint32_t),dhead.numverts*ahead.numframes,out);
	free(uverts);
	fclose(out);

}

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		fprintf(stderr,"usage: u95extract <archive>\n");
		return 0;
	}
	f = fopen(argv[1],"rb");
	if ( !f )
	{
		fprintf(stderr,"Cannot open %s: %s\n",argv[1],strerror(errno));
		return 1;
	}
	fseek(f,-sizeof(resourcefiletrailer_t),SEEK_END);
	fread(&tail,sizeof(resourcefiletrailer_t),1,f);
	if ( strncmp(tail.tag,RES_FILE_TAG,16) )
	{
		fprintf(stderr,"Not a valid Unreal Resource file.\n");
		fclose(f);
		return 1;
	}
	if ( tail.version != 1 )
	{
		fprintf(stderr,"Unreal Resource version %04x not supported.\n",
			tail.version);
		fclose(f);
		return 1;
	}
	printf("%s - %u names, %u imports, %u exports\n",argv[1],tail.nnames,
		tail.nimports,tail.nexports);
	names = calloc(tail.nnames,sizeof(nameentry_t));
	imports = calloc(tail.nimports,sizeof(resnamefileentry_t));
	exports = calloc(tail.nexports,sizeof(resnamefileentry_t));
	eheaders = calloc(tail.nexports,sizeof(size_t));
	fseek(f,tail.onames,SEEK_SET);
	fread(names,sizeof(nameentry_t),tail.nnames,f);
	fseek(f,tail.oimports,SEEK_SET);
	fread(imports,sizeof(resnamefileentry_t),tail.nimports,f);
	fseek(f,tail.oexports,SEEK_SET);
	fread(exports,sizeof(resnamefileentry_t),tail.nexports,f);
	fseek(f,tail.oheaders,SEEK_SET);
	for ( int i=0; i<tail.nexports; i++ )
	{
		eheaders[i] = ftell(f);
		fseek(f,exports[i].headersize,SEEK_CUR);
	}
	for ( int i=0; i<tail.nexports; i++ )
	{
		switch( exports[i].type )
		{
		case RES_Palette:
			extract_pal(i);
			break;
		case RES_Texture:
			extract_tex(i);
			break;
		case RES_Mesh:
			extract_msh(i);
			break;
		}
	}
	free(names);
	free(imports);
	free(exports);
	free(eheaders);
	fclose(f);
	return 0;
}
