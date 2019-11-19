/*
	extract text, meshes, textures, palettes and sounds from 0.82~0.84
	packages.
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
#define RES_FILE_TAG     "Unreal Resource\x1A"

#define RES_FILE_VERSION_083 (0x0009)
#define RES_FILE_VERSION_084 (0x000A)

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

const char typenames[36][32] =
{
	"None", "Buffer", "Type2", "Array", "TextBuffer", "Texture", "Font",
	"Palette", "Type8", "Script", "Class", "ActorList", "Sound", "Type13",
	"Mesh", "Type15", "Vectors", "BspNodes", "BspSurfs", "LightMesh",
	"Polys", "Model", "Level", "Type23", "Type24", "Camera", "Type26",
	"Type27", "Player", "VertPool", "Ambient", "TransBuffer", "MeshMap",
	"Bounds", "Terrain", "Enum"
};

typedef struct
{
	uint32_t version, nexports, nimports, nnames, onames, oimports,
		oexports, oheaders, lheaders;
	uint8_t pad[16];
	char deltafname[80];
	char tag[32];
} resourcefiletrailer_t;

typedef struct
{
	char name[32];
	uint32_t flags;
} nameentry_t;

typedef struct
{
	char name[32];
	uint32_t version, type, size;
} resnamefileentry_t;

typedef struct
{
	uint32_t lead;	// unused leading data
	uint32_t type;	// should be equal to the export's type
	char name[32];	// descriptive name of class
	uint32_t dataptr;	// empty, set at runtime by engine
	uint32_t index, version, filenum, flags, dataofs, datasize, crc, unused;
} uresource_t;

typedef struct
{
	uresource_t res;
	int32_t num, max;
} udatabase_t;

typedef struct
{
	udatabase_t db;
} ubuffer_t;

typedef struct
{
	udatabase_t db;
} uarray_t;

typedef struct
{
	udatabase_t db;
	int32_t pos;
} utextbuffer_t;

typedef struct
{
	uint8_t r, g, b, x;
} color_t;

typedef struct
{
	uresource_t res;
	uint32_t class, palette, microtexture, fireparams;
	uint16_t familyname, unusedname;
	float paldiffusionc, diffusec, specularc, frictionc;
	uint32_t footstepsound, hitsound, polyflags, notile, lockcount,
		cameracaps, pad3, pad4, pad5, pad6, pad7, pad8;
	int32_t usize, vsize, datasize, colorbytes;
	color_t mipzero;
	uint32_t mipofs[8];
	uint8_t ubits, vbits, unused1, unused2, pad[32];
} utexture083_t;

typedef struct
{
	uresource_t res;
	uint32_t class, palette, microtexture, fireparams;
	uint16_t familyname, unusedname;
	uint32_t pad1;
	float diffusec, specularc, frictionc;
	uint32_t footstepsound, hitsound, polyflags, untiled, lockcount,
		cameracaps, pad3, pad4, pad5, pad6, pad7, pad8;
	int32_t usize, vsize, datasize, colorbytes;
	color_t mipzero;
	uint32_t mipofs[8];
	uint8_t ubits, vbits, pad9, pad10, pad[32];
} utexture084_t;

typedef struct
{
	int32_t startu, startv, usize, vsize;
} fontcharacter_t;

typedef struct
{
	udatabase_t db;
	uint32_t texture;
} ufont_t;

typedef struct
{
	udatabase_t db;
} upalette_t;

typedef struct
{
	uresource_t res;
	uint32_t class;
	int32_t length, numstacktree, codeoffset, pad[5];
} uscript_t;

/*typedef struct
{
	udatabase_t db;
	uint32_t parentclass, scripttext, script, intrinsic, actorfunc,
		scripttextcrc, parenttextcrc, pad1, pad2, pad3;
	actor_t defaultactor;	// no way I'm defining this fucking thing
	int32_t length, numstacktree, codeoffset, pad[5];
} uclass_t;*/

typedef struct
{
	udatabase_t db;
	uint32_t locktype, trans, staticactors, dynamicactors, collidingactors,
		activeactors, unusedactors, justdeletedactors;
} uactorlist_t;

typedef struct
{
	uresource_t res;
	int32_t datasize;
	int16_t familyname;
	int32_t soundid;
	uint8_t pad[256];
} usound_t;

typedef struct
{
	float x, y, z;
	uint32_t flags;
} vector_t;

typedef struct
{
	int32_t pitch, yaw, roll;
} rotator_t;

typedef struct
{
	vector_t min, max;
} boundingrect_t;

typedef struct
{
	boundingrect_t rect;
	vector_t sphere;
} boundingvolume_t;

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

typedef struct
{
	int16_t seqname;
	int32_t startframe, numframes, rate;
} animseq_t;

typedef struct
{
	int32_t numverttris, trilistoffset;
} vertlink_t;

typedef struct
{
	uresource_t res;
	vector_t origin;
	rotator_t rotorigin;
	boundingvolume_t bound;
	int32_t ntris, mtris, nverts, mverts, nvertlinks, mvertlinks,
		nanimframes, manimframes, nanimseqs, manimseqs;
} umesh083_t;

typedef struct
{
	uresource_t res;
	vector_t origin;
	rotator_t rotorigin;
	boundingrect_t bound;
	int32_t ntris, mtris, nverts, mverts, nvertlinks, mvertlinks,
		nanimframes, manimframes, nanimseqs, manimseqs;
} umesh084_t;

typedef struct
{
	udatabase_t db;
} uvectors_t;

typedef struct
{
	vector_t plane;
	uint64_t zonemask;
	int32_t nodeflags, ivertpool, isurf, iback, ifront, iplane, ibound;
	uint8_t izone[2], numvertices, pad1;
	int32_t idynamic[2];
} bspnode_t;

typedef struct
{
	int32_t izoneactor, unused1;
	uint32_t unused2;
	uint64_t connectivity, visibility, unused3;
} zoneproperties083_t;

typedef struct
{
	int32_t izoneactor, unused;
	uint64_t connectivity, visibility;
} zoneproperties084_t;

typedef struct
{
	udatabase_t db;
	uint32_t numzones, numuniqueplanes, unused1, unused2;
	zoneproperties083_t zones[64];
} ubspnodes083_t;

typedef struct
{
	udatabase_t db;
	uint32_t numzones, numuniqueplanes, unused1, unused2;
	zoneproperties084_t zones[64];
} ubspnodes084_t;

typedef struct
{
	uint32_t texture, brush, polyflags;
	int32_t pbase, vnormal, vtextureu, vtexturev, ilightmesh, ibrushpoly;
	uint8_t panu, panuv;
	int32_t iactor;
	int16_t laststarty, lastendy;
} bspsurf_t;

typedef struct
{
	udatabase_t db;
} ubspsurfs_t;

typedef struct
{
	int32_t dataoffset, textureustart, texturevstart, meshusize, meshvsize,
		meshspacing, numstaticlights, numdynamiclights, ilightactor[16];
	uint8_t meshshift, meshubits, meshvbits, unused;
} lightmeshindex_t;

typedef struct
{
	uresource_t res;
	int32_t numindices, numdatabytes;
} ulightmesh_t;

typedef struct
{
	vector_t base, normal, textureu, texturev, vertex[16];
	uint32_t polyflags, brush, texture;
	int16_t groupname, itemname;
	int32_t numvertices, ilink, ibrushpoly;
	uint8_t panu, panv, izone[2];
} poly_t;

typedef struct
{
	udatabase_t db;
} upolys_t;

typedef struct
{
	vector_t scale;
	float sheerrate;
	int32_t sheeraxis;
} scale_t;

typedef struct
{
	uresource_t res;
	uint32_t vectors, points, bspnodes, bspsurfs, vertpool, polys,
		lightmesh, bounds, terrain;
	vector_t location;
	rotator_t rotation;
	vector_t prepivot;
	scale_t scaling;
	uint32_t csgoper, locktype, color, polyflags, modelflags;
	vector_t postpivot;
	scale_t postscale, tempscale;
	boundingvolume_t bound[2];
	uint8_t pad[32];
} umodel_t;

typedef struct
{
	uresource_t res;
	uint32_t levelstate;
	uint32_t beginplay, model, actorlist, playerlist, brusharray, misc,
		textblocks[16];
} ulevel_t;

/*typedef struct
{
} ucamera_t;

typedef struct
{
} uplayer_t;*/

typedef struct
{
	int32_t pvertex, iside;
} vertpool_t;

typedef struct
{
	udatabase_t db;
	int32_t numsharedsides;
} uvertpool_t;

/*typedef struct
{
} uambient_t;

typedef struct
{
} utransbuffer_t;*/

typedef struct
{
	uresource_t res;
	uint32_t mesh, maxtextures, andflags, orflags;
	vector_t scale;
} umeshmap_t;

typedef struct
{
	udatabase_t db;
} ubounds_t;

typedef struct
{
	int32_t gridx0, gridx1, gridy0, gridy1;
} terrainindex_t;

typedef struct
{
	udatabase_t db;
	uint32_t layerflags[8], heightmaps[8], tilemaps[8], tilerefs[128];
} uterrain_t;

typedef struct
{
	udatabase_t db;
} uenum_t;

typedef struct
{
	uint32_t hdrsize, recsize;	// only the essentials needed
} resourcetype_t;

// global stuff
FILE *f;
resourcefiletrailer_t tail;
nameentry_t *names;
resnamefileentry_t *imports;
resnamefileentry_t *exports;
char *headers;
char **eheaders;

// initialized at launch
resourcetype_t types[36] = {{0}};

void initialize_types( void )
{
	// RES_None
	types[RES_Buffer].hdrsize = sizeof(ubuffer_t);
	types[RES_Buffer].recsize = sizeof(char);
	types[RES_Array].hdrsize = sizeof(uarray_t);
	types[RES_Array].recsize = sizeof(uint32_t);
	types[RES_TextBuffer].hdrsize = sizeof(utextbuffer_t);
	types[RES_TextBuffer].recsize = sizeof(char);
	if ( tail.version == RES_FILE_VERSION_083 )
		types[RES_Texture].hdrsize = sizeof(utexture083_t);
	else types[RES_Texture].hdrsize = sizeof(utexture084_t);
	types[RES_Texture].recsize = 0;
	types[RES_Font].hdrsize = sizeof(ufont_t);
	types[RES_Font].recsize = sizeof(fontcharacter_t);
	types[RES_Palette].hdrsize = sizeof(upalette_t);
	types[RES_Palette].recsize = sizeof(color_t);
	types[RES_Script].hdrsize = sizeof(uscript_t);
	types[RES_Script].recsize = 1;
	// guessed, can't be bothered to declare AActor
	if ( tail.version == RES_FILE_VERSION_083 )
		types[RES_Class].hdrsize = 1168;
	else types[RES_Class].hdrsize = 1272;
	types[RES_Class].recsize = 0;
	types[RES_ActorList].hdrsize = sizeof(uactorlist_t);
	types[RES_ActorList].recsize = 0;	// will have to guess
	types[RES_Sound].hdrsize = sizeof(usound_t);
	types[RES_Sound].recsize = 1;
	if ( tail.version == RES_FILE_VERSION_083 )
		types[RES_Mesh].hdrsize = sizeof(umesh083_t);
	else types[RES_Mesh].hdrsize = sizeof(umesh084_t);
	types[RES_Mesh].recsize = 0;
	types[RES_Vectors].hdrsize = sizeof(uvectors_t);
	types[RES_Vectors].recsize = sizeof(vector_t);
	if ( tail.version == RES_FILE_VERSION_083 )
		types[RES_BspNodes].hdrsize = sizeof(ubspnodes083_t);
	else types[RES_BspNodes].hdrsize = sizeof(ubspnodes084_t);
	types[RES_BspNodes].recsize = sizeof(bspnode_t);
	types[RES_BspSurfs].hdrsize = sizeof(ubspsurfs_t);
	types[RES_BspSurfs].recsize = sizeof(bspsurf_t);
	types[RES_LightMesh].hdrsize = sizeof(ulightmesh_t);
	types[RES_LightMesh].recsize = 0;
	types[RES_Polys].hdrsize = sizeof(upolys_t);
	types[RES_Polys].recsize = sizeof(poly_t);
	types[RES_Model].hdrsize = sizeof(umodel_t);
	types[RES_Model].recsize = 0;
	types[RES_Level].hdrsize = sizeof(ulevel_t);
	if ( tail.version == RES_FILE_VERSION_084 )
		types[RES_Level].hdrsize += 4;	// dunno why
	types[RES_Level].recsize = 0;
	// RES_Camera
	// RES_Player
	types[RES_VertPool].hdrsize = sizeof(uvertpool_t);
	types[RES_VertPool].recsize = sizeof(vertpool_t);
	// RES_Ambient
	// RES_TransBuffer
	types[RES_MeshMap].hdrsize = sizeof(umeshmap_t);
	types[RES_MeshMap].recsize = 0;
	types[RES_Bounds].hdrsize = sizeof(ubounds_t);
	types[RES_Bounds].recsize = sizeof(boundingrect_t);
	types[RES_Terrain].hdrsize = sizeof(uterrain_t);
	types[RES_Terrain].recsize = sizeof(terrainindex_t);
	types[RES_Enum].hdrsize = sizeof(uenum_t);
	types[RES_Enum].recsize = sizeof(uint32_t);
}

void extract_txt( uint32_t i, utextbuffer_t *txt )
{
	mkdir("Classes",0755);
	fseek(f,txt->db.res.dataofs,SEEK_SET);
	char *rbuf = malloc(txt->db.res.datasize);
	fread(rbuf,1,txt->db.res.datasize-1,f);
	char fname[256];
	snprintf(fname,256,"Classes/%.32s.tcx",exports[i].name);
	FILE *out = fopen(fname,"wb");
	fwrite(rbuf,1,txt->db.res.datasize-1,out);
	fclose(out);
	free(rbuf);
}

void extract_snd( uint32_t i, usound_t *snd )
{
	mkdir("Sounds",0755);
	fseek(f,snd->res.dataofs,SEEK_SET);
	char sig[4];
	fread(sig,4,1,f);
	if ( !strncmp(sig,"CSNX",4) ) fseek(f,248,SEEK_CUR);
	else fseek(f,-4,SEEK_CUR);
	char *rbuf = malloc(snd->res.datasize);
	fread(rbuf,1,snd->res.datasize-1,f);
	char fname[256];
	snprintf(fname,256,"Sounds/%.32s.wav",exports[i].name);
	FILE *out = fopen(fname,"wb");
	fwrite(rbuf,1,snd->res.datasize-1,out);
	fclose(out);
	free(rbuf);
}


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

void extract_pal( uint32_t i, upalette_t *pal )
{
	mkdir("Palettes",0755);
	color_t cpal[256];
	png_color fpal[256];
	uint8_t dat[16384];
	fseek(f,pal->db.res.dataofs,SEEK_SET);
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
	snprintf(fname,256,"Palettes/%.32s.png",exports[i].name);
	writepng(fname,dat,128,128,fpal,256);
}

void extract_tex083( uint32_t i, utexture083_t *tex )
{
	mkdir("Textures",0755);
	uint8_t *pxdata = malloc(tex->res.datasize);
	fseek(f,tex->res.dataofs+tex->mipofs[0],SEEK_SET);
	fread(pxdata,1,tex->res.datasize,f);
	png_color fpal[256];
	if ( tex->palette )
	{
		upalette_t *pal = (upalette_t*)eheaders[tex->palette-1];
		color_t cpal[256];
		fseek(f,pal->db.res.dataofs,SEEK_SET);
		fread(cpal,sizeof(color_t),256,f);
		for ( int i=0; i<256; i++ )
		{
			fpal[i].red = cpal[i].r;
			fpal[i].green = cpal[i].g;
			fpal[i].blue = cpal[i].b;
		}
	}
	else
	{
		// fallback
		for ( int i=0; i<256; i++ )
		{
			fpal[i].red = i;
			fpal[i].green = i;
			fpal[i].blue = i;
		}
	}
	char fname[256];
	snprintf(fname,256,"Textures/%.32s.png",exports[i].name);
	writepng(fname,pxdata,tex->usize,tex->vsize,&fpal[0],256);
	free(pxdata);
}

void extract_tex084( uint32_t i, utexture084_t *tex )
{
	mkdir("Textures",0755);
	uint8_t *pxdata = malloc(tex->res.datasize);
	fseek(f,tex->res.dataofs+tex->mipofs[0],SEEK_SET);
	fread(pxdata,1,tex->res.datasize,f);
	png_color fpal[256];
	if ( tex->palette )
	{
		upalette_t *pal = (upalette_t*)eheaders[tex->palette-1];
		color_t cpal[256];
		fseek(f,pal->db.res.dataofs,SEEK_SET);
		fread(cpal,sizeof(color_t),256,f);
		for ( int i=0; i<256; i++ )
		{
			fpal[i].red = cpal[i].r;
			fpal[i].green = cpal[i].g;
			fpal[i].blue = cpal[i].b;
		}
	}
	else
	{
		// fallback
		for ( int i=0; i<256; i++ )
		{
			fpal[i].red = i;
			fpal[i].green = i;
			fpal[i].blue = i;
		}
	}
	char fname[256];
	snprintf(fname,256,"Textures/%.32s.png",exports[i].name);
	writepng(fname,pxdata,tex->usize,tex->vsize,&fpal[0],256);
	free(pxdata);
}

void extract_msh083( uint32_t i, umesh083_t *msh )
{
	mkdir("Models",0755);
	char fname[256];
	FILE *out;
	// write datafile
	snprintf(fname,256,"Models/%.32s_d.3d",exports[i].name);
	out = fopen(fname,"wb");
	dataheader_t dhead =
	{
		.numpolys = msh->mtris,
		.numverts = msh->mverts
	};
	fwrite(&dhead,sizeof(dataheader_t),1,out);
	fseek(f,msh->res.dataofs,SEEK_SET);
	datapoly_t *dpoly = calloc(sizeof(datapoly_t),dhead.numpolys);
	fread(dpoly,sizeof(datapoly_t),dhead.numpolys,f);
	fwrite(dpoly,sizeof(datapoly_t),dhead.numpolys,out);
	free(dpoly);
	fclose(out);
	// write anivfile
	snprintf(fname,256,"Models/%.32s_a.3d",exports[i].name);
	out = fopen(fname,"wb");
	aniheader_t ahead =
	{
		.numframes = msh->manimframes,
		.framesize = msh->mverts*4
	};
	fwrite(&ahead,sizeof(aniheader_t),1,out);
	uint32_t *uverts = calloc(sizeof(uint32_t),dhead.numverts
		*ahead.numframes);
	fread(uverts,sizeof(uint32_t),dhead.numverts*ahead.numframes,f);
	fwrite(uverts,sizeof(uint32_t),dhead.numverts*ahead.numframes,out);
	free(uverts);
	fclose(out);
}

void extract_msh084( uint32_t i, umesh084_t *msh )
{
	mkdir("Models",0755);
	char fname[256];
	FILE *out;
	// write datafile
	snprintf(fname,256,"Models/%.32s_d.3d",exports[i].name);
	out = fopen(fname,"wb");
	dataheader_t dhead =
	{
		.numpolys = msh->mtris,
		.numverts = msh->mverts
	};
	fwrite(&dhead,sizeof(dataheader_t),1,out);
	fseek(f,msh->res.dataofs,SEEK_SET);
	datapoly_t *dpoly = calloc(sizeof(datapoly_t),dhead.numpolys);
	fread(dpoly,sizeof(datapoly_t),dhead.numpolys,f);
	fwrite(dpoly,sizeof(datapoly_t),dhead.numpolys,out);
	free(dpoly);
	fclose(out);
	// write anivfile
	snprintf(fname,256,"Models/%.32s_a.3d",exports[i].name);
	out = fopen(fname,"wb");
	aniheader_t ahead =
	{
		.numframes = msh->manimframes,
		.framesize = msh->mverts*4
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
		fprintf(stderr,"usage: u083extract <archive>\n");
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
	if ( strncmp(tail.tag,RES_FILE_TAG,32) )
	{
		fprintf(stderr,"Not a valid Unreal Resource file.\n");
		fclose(f);
		return 1;
	}
	if ( (tail.version != RES_FILE_VERSION_083)
		&& (tail.version != RES_FILE_VERSION_084) )
	{
		fprintf(stderr,"Unreal Resource version %04x not supported.\n",
			tail.version);
		fclose(f);
		return 1;
	}
	printf("%s - %u names, %u imports, %u exports, %u bytes of headers\n",
		argv[1],tail.nnames,tail.nimports,tail.nexports,tail.lheaders);
	names = calloc(tail.nnames,sizeof(nameentry_t));
	imports = calloc(tail.nimports,sizeof(resnamefileentry_t));
	exports = calloc(tail.nexports,sizeof(resnamefileentry_t));
	headers = calloc(tail.lheaders,1);
	fseek(f,tail.onames,SEEK_SET);
	fread(names,sizeof(nameentry_t),tail.nnames,f);
	fseek(f,tail.oimports,SEEK_SET);
	fread(imports,sizeof(resnamefileentry_t),tail.nimports,f);
	fseek(f,tail.oexports,SEEK_SET);
	fread(exports,sizeof(resnamefileentry_t),tail.nexports,f);
	fseek(f,tail.oheaders,SEEK_SET);
	fread(headers,1,tail.lheaders,f);
	initialize_types();
	eheaders = calloc(tail.nexports,sizeof(char*));
	char *ptr = headers, *prev = 0;
	for ( int i=0; i<tail.nexports; i++ )
	{
		// for debug
		/*printf("%d %.32s %.32s &%08x (%08x)\n",i,exports[i].name,
			typenames[exports[i].type],
			tail.oheaders+(ptr-headers),*(uint32_t*)ptr);*/
		if ( !types[exports[i].type].hdrsize )
		{
			fprintf(stderr,"Type %.32s at %x not supported, bailing"
				" out\n",typenames[exports[i].type],
				tail.oheaders+(ptr-headers));
			if ( i < (tail.nexports-1) )
			fprintf(stderr,"Next export is %.32s with type %.32s\n",
				exports[i+1].name,typenames[exports[i+1].type]);
			return 128;
		}
		eheaders[i] = ptr;
		prev = ptr;
		ptr += types[exports[i].type].hdrsize;
	}
	for ( int i=0; i<tail.nexports; i++ )
	{
		switch( exports[i].type )
		{
		case RES_TextBuffer:
			extract_txt(i,(utextbuffer_t*)eheaders[i]);
			break;
		case RES_Sound:
			extract_snd(i,(usound_t*)eheaders[i]);
			break;
		case RES_Palette:
			extract_pal(i,(upalette_t*)eheaders[i]);
			break;
		case RES_Texture:
			if ( tail.version == RES_FILE_VERSION_083 )
				extract_tex083(i,(utexture083_t*)eheaders[i]);
			else extract_tex084(i,(utexture084_t*)eheaders[i]);
			break;
		case RES_Mesh:
			if ( tail.version == RES_FILE_VERSION_083 )
				extract_msh083(i,(umesh083_t*)eheaders[i]);
			else extract_msh084(i,(umesh084_t*)eheaders[i]);
			break;
		}
	}
	free(names);
	free(imports);
	free(exports);
	free(headers);
	free(eheaders);
	fclose(f);
	return 0;
}
