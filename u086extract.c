/*
	should support anything from 0.86~0.87, when the arctive format changed
	to something that's kind of an intermediate point between the early and
	final formats
	fortunately this one is more third party tool friendly as you don't
	need to know the structure of all object types to read the data

	known issues:
	 - SBase.utx from some versions (e.g. 0.874d) has some incongruencies
	   in its internal texture struct that I have yet to reverse engineer.
	   It's a feckin' mess, that's for sure. It's not even the same as
	   other version 25 packages.
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

#define RES_FILE_TAG	"Unrealfile\x1A"

typedef struct
{
	char tag[32];
	uint32_t version, nobjects, nnames, onames, oobjects, ndeps;
} unrealheader_t;

typedef struct
{
	char name[32];
	uint32_t flags;
} unrealname_t;

typedef struct
{
	int16_t name, pkg, class;
	uint32_t flags, headerofs, crc, dataofs, headersiz, datasiz;
} unrealobject_t;

FILE *f;
unrealheader_t head;
unrealname_t *names;
unrealobject_t *objects;

void readstr( char *dest, int max, FILE *f )
{
	int cnt = 0;
	while ( cnt < max )
	{
		fread(&dest[cnt],1,1,f);
		if ( !dest[cnt] ) return;
		cnt++;
	}
}

void debug_printheader( uint32_t i )
{
	uint8_t *header = malloc(objects[i].headersiz);
	fseek(f,objects[i].headerofs,SEEK_SET);
	fread(header,objects[i].headersiz,1,f);
	int j = 0;
	printf("%.32s\'%.32s\' header (%u):\n",names[objects[i].class-1].name,
		names[objects[i].name-1].name,objects[i].headersiz);
	for ( j=0; j<objects[i].headersiz; j++ )
		printf("%02x%c",header[j],((j+1)%8)?' ':'\n');
	if ( j%8 ) printf("\n");
	free(header);
}

void extract_txt( uint32_t i )
{
	if ( !objects[i].dataofs || !objects[i].datasiz ) return;
	mkdir("Classes",0755);
	fseek(f,objects[i].dataofs,SEEK_SET);
	char *rbuf = malloc(objects[i].datasiz);
	fread(rbuf,1,objects[i].datasiz-1,f);
	char fname[256];
	snprintf(fname,256,"Classes/%.32s.uc",names[objects[i].name-1].name);
	FILE *out = fopen(fname,"wb");
	fwrite(rbuf,1,objects[i].datasiz-1,out);
	fclose(out);
	free(rbuf);
}

void extract_snd( uint32_t i )
{
	if ( !objects[i].dataofs || !objects[i].datasiz ) return;
	/*debug_printheader(i);
	return;*/
	mkdir("Sounds",0755);
	// skip to what we need
	int fofs = 20;
	if ( head.version >= 27 ) fofs = 34;
	else if ( head.version >= 25 ) fofs = 0;
	fseek(f,objects[i].headerofs+fofs,SEEK_SET);
	int16_t family;
	fread(&family,2,1,f);
	fseek(f,objects[i].dataofs,SEEK_SET);
	char sig[4];
	fread(sig,4,1,f);
	if ( !strncmp(sig,"CSNX",4) ) fseek(f,248,SEEK_CUR);
	else fseek(f,-4,SEEK_CUR);
	char *rbuf = malloc(objects[i].datasiz);
	fread(rbuf,1,objects[i].datasiz-1,f);
	char fname[256];
	if ( family > 0 ) snprintf(fname,256,"Sounds/%.32s.%.32s.wav",
		names[family-1].name,names[objects[i].name-1].name);
	else snprintf(fname,256,"Sounds/%.32s.wav",
		names[objects[i].name-1].name);
	FILE *out = fopen(fname,"wb");
	fwrite(rbuf,1,objects[i].datasiz-1,out);
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

typedef struct
{
	uint8_t r, g, b, x;
} color_t;

void extract_pal( uint32_t i )
{
	if ( !objects[i].dataofs || !objects[i].datasiz ) return;
	mkdir("Palettes",0755);
	color_t cpal[256];
	png_color fpal[256];
	uint8_t dat[16384];
	fseek(f,objects[i].dataofs,SEEK_SET);
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
	snprintf(fname,256,"Palettes/%.32s.png",names[objects[i].name-1].name);
	writepng(fname,dat,128,128,fpal,256);
}

void extract_tex( uint32_t i )
{
	if ( !objects[i].dataofs || !objects[i].datasiz ) return;
	mkdir("Textures",0755);
	int oofs = 40;
	if ( objects[i].headersiz == 138 ) oofs = 24;
	if ( head.version >= 27 ) oofs += 8;
	else if ( head.version >= 25 ) oofs += 2;
	int32_t pal;
	fseek(f,objects[i].headerofs+oofs,SEEK_SET);
	fread(&pal,4,1,f);
	uint32_t w, h;
	if ( head.version >= 27 ) oofs += 21;
	else if ( head.version >= 25 ) oofs += 8;
	fseek(f,objects[i].headerofs+oofs+38,SEEK_SET);
	fread(&w,4,1,f);
	fread(&h,4,1,f);
	uint32_t mipofs;
	fseek(f,objects[i].headerofs+oofs+66,SEEK_SET);
	fread(&mipofs,4,1,f);
	if ( mipofs == 0xffffffff ) mipofs = 0;	// why
	uint8_t *pxdata = malloc(w*h);
	fseek(f,objects[i].dataofs+mipofs,SEEK_SET);
	fread(pxdata,1,w*h,f);
	png_color fpal[256];
	if ( pal )
	{
		color_t cpal[256];
		fseek(f,objects[pal-1].dataofs,SEEK_SET);
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
	snprintf(fname,256,"Textures/%.32s.png",names[objects[i].name-1].name);
	writepng(fname,pxdata,w,h,&fpal[0],256);
	free(pxdata);
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
	mkdir("Models",0755);
	int dofs = 16;
	int oofs = 57;
	if ( head.version >= 27 )
	{
		oofs += 20;
		dofs += 8;
	}
	else if ( head.version >= 25 )
	{
		dofs += 2;
		oofs += 2;
	}
	int32_t verts, tris, seqs;
	uint32_t nverts, nframes, tverts, npolys;
	fseek(f,objects[i].headerofs+oofs,SEEK_SET);
	fread(&verts,4,1,f);
	fread(&tris,4,1,f);
	fread(&seqs,4,1,f);
	fseek(f,objects[i].headerofs+oofs+32,SEEK_SET);
	fread(&nverts,4,1,f);
	fread(&nframes,4,1,f);
	fseek(f,objects[verts-1].headerofs+dofs,SEEK_SET);
	fread(&tverts,4,1,f);
	fseek(f,objects[tris-1].headerofs+dofs,SEEK_SET);
	fread(&npolys,4,1,f);
	char fname[256];
	FILE *out;
	// write datafile
	snprintf(fname,256,"Models/%.32s_d.3d",names[objects[i].name-1].name);
	out = fopen(fname,"wb");
	dataheader_t dhead =
	{
		.numpolys = npolys,
		.numverts = nverts,
	};
	fwrite(&dhead,sizeof(dataheader_t),1,out);
	fseek(f,objects[tris-1].dataofs,SEEK_SET);
	datapoly_t *dpoly = calloc(sizeof(datapoly_t),dhead.numpolys);
	fread(dpoly,sizeof(datapoly_t),dhead.numpolys,f);
	fwrite(dpoly,sizeof(datapoly_t),dhead.numpolys,out);
	free(dpoly);
	fclose(out);
	// write anivfile
	snprintf(fname,256,"Models/%.32s_a.3d",names[objects[i].name-1].name);
	out = fopen(fname,"wb");
	aniheader_t ahead =
	{
		.numframes = nframes,
		.framesize = nverts*4,
	};
	fwrite(&ahead,sizeof(aniheader_t),1,out);
	fseek(f,objects[verts-1].dataofs,SEEK_SET);
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
		fprintf(stderr,"usage: u086extract <archive>\n");
		return 0;
	}
	f = fopen(argv[1],"rb");
	if ( !f )
	{
		fprintf(stderr,"Cannot open %s: %s\n",argv[1],strerror(errno));
		return 1;
	}
	fread(&head,sizeof(unrealheader_t),1,f);
	if ( strncmp(head.tag,RES_FILE_TAG,32) )
	{
		fprintf(stderr,"Not a valid Unreal Resource file.\n");
		fclose(f);
		return 1;
	}
	printf("%s - version %u, %u names (&%x), %u objects (&%x),"
		" %u dependencies\n",argv[1],head.version,head.nnames,
		head.onames,head.nobjects,head.oobjects,head.ndeps);
	names = calloc(head.nnames,sizeof(unrealname_t));
	fseek(f,head.onames,SEEK_SET);
	for ( int i=0; i<head.nnames; i++ )
	{
		readstr(names[i].name,32,f);
		fread(&names[i].flags,4,1,f);
	}
	fseek(f,head.oobjects,SEEK_SET);
	objects = calloc(head.nobjects,sizeof(unrealobject_t));
	for ( int i=0; i<head.nobjects; i++ )
	{
		fread(&objects[i].name,2,1,f);
		if ( head.version >= 25 ) fread(&objects[i].pkg,2,1,f);
		fread(&objects[i].class,2,1,f);
		fread(&objects[i].flags,4,2,f);
		if ( objects[i].headerofs != 0 ) fread(&objects[i].crc,4,4,f);
		if ( (head.version >= 27)
			&& !strcmp(names[objects[i].class-1].name,"Class") )
			fseek(f,14,SEEK_CUR);	// haven't figured this out yet
	}
	/*for ( int i=0; i<head.nnames; i++ )
		printf("%x %.32s\n",i,names[i].name);*/
	for ( int i=0; i<head.nobjects; i++ )
	{
		/*printf("%.32s (%.32s)\n",names[objects[i].name-1].name,
			names[objects[i].class-1].name);
		fflush(stdout);*/
		if ( !strcmp(names[objects[i].class-1].name,"TextBuffer") )
			extract_txt(i);
		else if ( !strcmp(names[objects[i].class-1].name,"Sound") )
			extract_snd(i);
		else if ( !strcmp(names[objects[i].class-1].name,"Palette") )
			extract_pal(i);
		else if ( !strcmp(names[objects[i].class-1].name,"Texture") )
			extract_tex(i);
		else if ( !strcmp(names[objects[i].class-1].name,"Mesh") )
			extract_msh(i);
	}
	free(names);
	free(objects);
	fclose(f);
	return 0;
}
