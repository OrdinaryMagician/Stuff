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

#define UPKG_MAGIC 0x9E2A83C1

// uncomment if you want full data dumps, helpful if you need to reverse engineer some unsupported format
//#define _DEBUG 1

typedef struct
{
	uint32_t magic;
	uint16_t pkgver, license;
	uint32_t flags, nnames, onames, nexports, oexports, nimports, oimports;
} upkg_header_t;

uint8_t *pkgfile;
upkg_header_t *head;
size_t fpos = 0;

uint8_t readbyte( void )
{
	uint8_t val = pkgfile[fpos];
	fpos++;
	return val;
}

uint16_t readword( void )
{
	uint16_t val = *(uint16_t*)(pkgfile+fpos);
	fpos += 2;
	return val;
}

uint32_t readdword( void )
{
	uint32_t val = *(uint32_t*)(pkgfile+fpos);
	fpos += 4;
	return val;
}

float readfloat( void )
{
	float val = *(float*)(pkgfile+fpos);
	fpos += 4;
	return val;
}

#define READSTRUCT(x,y) {memcpy(&x,pkgfile+fpos,sizeof(y));fpos+=sizeof(y);}

// reads a compact index value
int32_t readindex( void )
{
	uint8_t byte[5] = {0};
	byte[0] = readbyte();
	if ( !byte[0] ) return 0;
	if ( byte[0]&0x40 )
	{
		for ( int i=1; i<5; i++ )
		{
			byte[i] = readbyte();
			if ( !(byte[i]&0x80) ) break;
		}
	}
	int32_t tf = byte[0]&0x3f;
	tf |= (int32_t)(byte[1]&0x7f)<<6;
	tf |= (int32_t)(byte[2]&0x7f)<<13;
	tf |= (int32_t)(byte[3]&0x7f)<<20;
	tf |= (int32_t)(byte[4]&0x7f)<<27;
	if ( byte[0]&0x80 ) tf *= -1;
	return tf;
}

// reads a name table entry
size_t readname( int *olen )
{
	size_t pos = fpos;
	if ( head->pkgver >= 64 )
	{
		int32_t len = readindex();
		pos = fpos;
		if ( olen ) *olen = len;
		if ( len <= 0 ) return pos;
		fpos += len;
	}
	else
	{
		int c, p = 0;
		while ( (c = readbyte()) ) p++;
		if ( olen ) *olen = p;
	}
	fpos += 4;
	return pos;
}

size_t getname( int index, int *olen )
{
	size_t prev = fpos;
	fpos = head->onames;
	size_t npos = 0;
	for ( int i=0; i<=index; i++ )
		npos = readname(olen);
	fpos = prev;
	return npos;
}

// checks if a name exists
int hasname( const char *needle )
{
	if ( !needle ) return 0;
	size_t prev = fpos;
	fpos = head->onames;
	int found = 0;
	int nlen = strlen(needle);
	for ( uint32_t i=0; i<head->nnames; i++ )
	{
		int32_t len = 0;
		if ( head->pkgver >= 64 )
		{
			len = readindex();
			if ( len <= 0 ) continue;
		}
		int c = 0, p = 0, match = 1;
		while ( (c = readbyte()) )
		{
			if ( (p >= nlen) || (needle[p] != c) ) match = 0;
			p++;
			if ( len && (p > len) ) break;
		}
		if ( match )
		{
			found = 1;
			break;
		}
		fpos += 4;
	}
	fpos = prev;
	return found;
}

// read import table entry and return index of its object name
int32_t readimport( void )
{
	readindex();
	readindex();
	if ( head->pkgver >= 55 ) fpos += 4;
	else readindex();
	return readindex();
}

int32_t getimport( int index )
{
	size_t prev = fpos;
	fpos = head->oimports;
	int32_t iname = 0;
	for ( int i=0; i<=index; i++ )
		iname = readimport();
	fpos = prev;
	return iname;
}

// fully read import table entry
void readimport2( int32_t *cpkg, int32_t *cname, int32_t *pkg, int32_t *name )
{
	*cpkg = readindex();
	*cname = readindex();
	if ( head->pkgver >= 55 ) *pkg = readdword();
	else *pkg = readindex();
	*name = readindex();
}

void getimport2( int index, int32_t *cpkg, int32_t *cname, int32_t *pkg,
	int32_t *name )
{
	size_t prev = fpos;
	fpos = head->oimports;
	for ( int i=0; i<=index; i++ )
		readimport2(cpkg,cname,pkg,name);
	fpos = prev;
}

// read export table entry
void readexport( int32_t *class, int32_t *ofs, int32_t *siz, int32_t *name )
{
	*class = readindex();
	readindex();
	if ( head->pkgver >= 55 ) fpos += 4;
	*name = readindex();
	fpos += 4;
	*siz = readindex();
	if ( *siz > 0 ) *ofs = readindex();
}

void getexport( int index, int32_t *class, int32_t *ofs, int32_t *siz,
	int32_t *name )
{
	size_t prev = fpos;
	fpos = head->oexports;
	for ( int i=0; i<=index; i++ )
		readexport(class,ofs,siz,name);
	fpos = prev;
}

// fully read export table entry
void readexport2( int32_t *class, int32_t *super, int32_t *pkg, int32_t *name,
	uint32_t *flags, int32_t *siz, int32_t *ofs )
{
	*class = readindex();
	*super = readindex();
	if ( head->pkgver >= 55 ) *pkg = readdword();
	else *pkg = 0;
	*name = readindex();
	*flags = readdword();
	*siz = readindex();
	if ( *siz > 0 ) *ofs = readindex();
}

void getexport2( int index, int32_t *class, int32_t *super, int32_t *pkg,
	int32_t *name, uint32_t *flags, int32_t *siz, int32_t *ofs )
{
	size_t prev = fpos;
	fpos = head->oexports;
	for ( int i=0; i<=index; i++ )
		readexport2(class,super,pkg,name,flags,siz,ofs);
	fpos = prev;
}

int writepng( const char *filename, unsigned char *fdata, int fw, int fh,
	png_color *fpal, int fpalsiz, int masked )
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
		if ( masked )
		{
			unsigned char t = 0;
			png_set_tRNS(pngp,infp,&t,1,0);
		}
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
} __attribute__((packed)) color_t;

void readpalette( uint32_t pal, int32_t *num, color_t **col )
{
	size_t prev = fpos;
	int32_t class, ofs, siz, name;
	getexport(pal-1,&class,&ofs,&siz,&name);
	int32_t l = 0;
	char *n = (char*)(pkgfile+getname(name,&l));
	// begin reading data
	fpos = ofs;
	if ( head->pkgver < 45 ) fpos += 4;
	if ( head->pkgver < 55 ) fpos += 16;
	if ( head->pkgver <= 44 ) fpos -= 6;	// ???
	if ( head->pkgver == 45 ) fpos -= 2;	// ???
	if ( head->pkgver <= 35 ) fpos += 8;	// ???
	// process properties
	int32_t prop = readindex();
	if ( (uint32_t)prop >= head->nnames )
	{
		printf("Unknown property %d, skipping\n",prop);
		fpos = prev;
		return;
	}
	char *pname = (char*)(pkgfile+getname(prop,&l));
retrypal:
	if ( strncasecmp(pname,"none",l) )
	{
		uint8_t info = readbyte();
		int array = info&0x80;
		int type = info&0xf;
		int psiz = (info>>4)&0x7;
		switch ( psiz )
		{
		case 0:
			psiz = 1;
			break;
		case 1:
			psiz = 2;
			break;
		case 2:
			psiz = 4;
			break;
		case 3:
			psiz = 12;
			break;
		case 4:
			psiz = 16;
			break;
		case 5:
			psiz = readbyte();
			break;
		case 6:
			psiz = readword();
			break;
		case 7:
			psiz = readdword();
			break;
		}
		if ( type == 10 )
			readindex();	// skip struct name
		fpos += psiz;
		prop = readindex();
		pname = (char*)(pkgfile+getname(prop,&l));
		goto retrypal;
	}
	if ( head->pkgver < 55 )
	{
		// group
		fpos++;
		readindex();
	}
	*num = readindex();
	//printf(" palette: %u colors\n",*num);
	*col = calloc(sizeof(color_t),*num);
	memcpy(*col,pkgfile+fpos,*num*sizeof(color_t));
	fpos = prev;
}

void savetexture( int32_t namelen, char *name, int32_t pal, int masked,
	int version, int32_t grouplen, char *group )
{
	uint32_t ncolors = 256;
	color_t *paldata = 0;
	readpalette(pal,&ncolors,&paldata);
	if ( version < 55 )
	{
		// group
		fpos++;
		int32_t grp = readindex();
		group = (char*)(pkgfile+getname(grp,&grouplen));
	}
	uint32_t mipcnt = readbyte();
	//printf(" %u mips\n",mipcnt);
	uint32_t ofs = 0;
	if ( version >= 63 ) ofs = readdword();
	uint32_t datasiz = readindex();
	//printf(" %u size\n",datasiz);
	uint8_t *imgdata = malloc(datasiz);
	memcpy(imgdata,pkgfile+fpos,datasiz);
	imgdata = malloc(datasiz);
	memcpy(imgdata,pkgfile+fpos,datasiz);
	if ( version >= 63 ) fpos = ofs;
	else fpos += datasiz;
	uint32_t w = readdword();
	uint32_t h = readdword();
	png_color fpal[256] = {{0}};
	if ( !paldata || (ncolors <= 0) )
	{
		ncolors = 256;
		// generate dummy palette
		for ( int i=0; i<256; i++ )
		{
			fpal[i].red = i;
			fpal[i].green = i;
			fpal[i].blue = i;
		}
	}
	else
	{
		for ( int i=0; i<ncolors; i++ )
		{
			fpal[i].red = paldata[i].r;
			fpal[i].green = paldata[i].g;
			fpal[i].blue = paldata[i].b;
		}
	}
	char fname[256];
	if ( group && strncmp(group,"None",grouplen) )
	{
		snprintf(fname,256,"Textures/%.*s",grouplen,group);
		mkdir(fname,0775);
		snprintf(fname,256,"Textures/%.*s/%.*s.png",grouplen,group,
			namelen,name);
	}
	else snprintf(fname,256,"Textures/%.*s.png",namelen,name);
	writepng(fname,imgdata,w,h,fpal,ncolors,masked);
}

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		printf("Usage: utxextract <archive>\n");
		return 1;
	}
	int fd = open(argv[1],O_RDONLY);
	if ( fd == -1 )
	{
		printf("Failed to open file %s: %s\n",argv[1],strerror(errno));
		return 1;
	}
	struct stat st;
	fstat(fd,&st);
	pkgfile = malloc(st.st_size);
	memset(pkgfile,0,st.st_size);
	head = (upkg_header_t*)pkgfile;
	int r = 0;
	do
	{
		r = read(fd,pkgfile+fpos,131072);
		if ( r == -1 )
		{
			close(fd);
			free(pkgfile);
			printf("Read failed for file %s: %s\n",argv[1],
				strerror(errno));
			return 4;
		}
		fpos += r;
	}
	while ( r > 0 );
	close(fd);
	fpos = 0;
	if ( head->magic != UPKG_MAGIC )
	{
		printf("File %s is not a valid unreal package!\n",argv[1]);
		free(pkgfile);
		return 2;
	}
	// loop through exports and search for textures
	fpos = head->oexports;
	for ( uint32_t i=0; i<head->nexports; i++ )
	{
		int32_t class, super, pkg, name, flags, siz, ofs;
		readexport2(&class,&super,&pkg,&name,&flags,&siz,&ofs);
		if ( (siz <= 0) || (class >= 0) ) continue;
		// get the class name
		class = -class-1;
		if ( (uint32_t)class > head->nimports ) continue;
		int32_t l = 0;
		char *n = (char*)(pkgfile+getname(getimport(class),&l));
		int istex = !strncasecmp(n,"Texture",l);
		if ( !istex ) continue;
		mkdir("Textures",0775);
		// get the highest group name (must be an export)
		char *pkgn = 0;
		int32_t pkgl = 0;
		while ( pkg > 0 )
		{
			int32_t pclass, psuper, ppkg, pname, pflags, psiz, pofs;
			getexport2(pkg-1,&pclass,&psuper,&ppkg,&pname,&pflags,&psiz,&pofs);
			pkgn = (char*)(pkgfile+getname(pname,&pkgl));
			pkg = ppkg;
		}
		char *tex = (char*)(pkgfile+getname(name,&l));
		if ( pkgn && strncmp(pkgn,"None",pkgl) )
			printf("Texture found: %.*s.%.*s\n",pkgl,pkgn,l,tex);
		else printf("Texture found: %.*s\n",l,tex);
		int32_t texl = l;
#ifdef _DEBUG
		char fname[256] = {0};
		snprintf(fname,256,"%.*s.object",texl,tex);
		printf(" Dumping full object data to %s\n",fname);
		FILE *f = fopen(fname,"wb");
		fwrite(pkgfile+ofs,siz,1,f);
		fclose(f);
		continue;
#endif
		// begin reading data
		size_t prev = fpos;
		fpos = ofs;
		if ( head->pkgver < 45 ) fpos += 4;
		if ( head->pkgver < 55 ) fpos += 16;
		if ( head->pkgver <= 44 ) fpos -= 6;	// ???
		if ( head->pkgver == 45 ) fpos -= 2;	// ???
		if ( head->pkgver <= 35 ) fpos += 8;	// ???
		// process properties
		int32_t prop = readindex();
		if ( (uint32_t)prop >= head->nnames )
		{
			printf("Unknown property %d, skipping\n",prop);
			fpos = prev;
			continue;
		}
		int32_t pal = 0;	// we need to fetch this
		char *pname = (char*)(pkgfile+getname(prop,&l));
		int masked = 0;	// and this (which may not be available)
retry:
		if ( strncasecmp(pname,"None",l) )
		{
			uint8_t info = readbyte();
			int array = info&0x80;
			int type = info&0xf;
			int psiz = (info>>4)&0x7;
			switch ( psiz )
			{
			case 0:
				psiz = 1;
				break;
			case 1:
				psiz = 2;
				break;
			case 2:
				psiz = 4;
				break;
			case 3:
				psiz = 12;
				break;
			case 4:
				psiz = 16;
				break;
			case 5:
				psiz = readbyte();
				break;
			case 6:
				psiz = readword();
				break;
			case 7:
				psiz = readdword();
				break;
			}
			//printf(" prop %.*s (%u, %u, %u, %u)\n",l,pname,array,type,(info>>4)&7,psiz);
			if ( array && (type != 3) )
			{
				int idx = readindex();
				//printf(" index: %d\n",idx);
			}
			if ( !strncasecmp(pname,"Palette",l) )
				pal = readindex();
			else if ( !strncasecmp(pname,"bMasked",l) )
				masked = array;
			else
			{
				if ( type == 10 )
				{
					int32_t tl, sn;
					sn = readindex();
					char *sname = (char*)(pkgfile+getname(sn,&tl));
					//printf(" struct: %.*s\n",tl,sname);
				}
				fpos += psiz;
			}
			prop = readindex();
			pname = (char*)(pkgfile+getname(prop,&l));
			goto retry;
		}
		if ( pal )
			savetexture(texl,tex,pal,masked,head->pkgver,pkgl,pkgn);
		fpos = prev;
	}
	free(pkgfile);
	return 0;
}
