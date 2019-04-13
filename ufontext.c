#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

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
	if ( head->pkgver >= 60 ) fpos += 4;
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
	if ( head->pkgver >= 60 ) *pkg = readdword();
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
	if ( head->pkgver >= 60 ) fpos += 4;
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
	if ( head->pkgver >= 60 ) *pkg = readdword();
	else *pkg = readindex();
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

// construct full name for object
// shamelessly recycled from my old upackage project
void imprefix( FILE *f, int32_t i );
void exprefix( FILE *f, int32_t i );
void imprefix( FILE *f, int32_t i )
{
	int32_t cpkg, cnam, pkg, nam;
	getimport2(i,&cpkg,&cnam,&pkg,&nam);
	if ( pkg < 0 ) imprefix(f,-pkg-1);
	else if ( pkg > 0 ) exprefix(f,pkg-1);
	if ( pkg ) fprintf(f,".");
	int32_t l;
	char *pname = (char*)(pkgfile+getname(nam,&l));
	fprintf(f,"%.*s",l,pname);
}
void exprefix( FILE *f, int32_t i )
{
	int32_t cls, sup, pkg, nam, siz, ofs;
	uint32_t fl;
	getexport2(i,&cls,&sup,&pkg,&nam,&fl,&siz,&ofs);
	if ( pkg > 0 )
	{
		exprefix(f,pkg-1);
		fprintf(f,".");
	}
	int32_t l;
	char *pname = (char*)(pkgfile+getname(nam,&l));
	fprintf(f,"%.*s",l,pname);
}
void construct_fullname( FILE *f, int32_t i )
{
	if ( i > 0 ) exprefix(f,i-1);
	else if ( i < 0 ) imprefix(f,-i-1);
	else fprintf(f,"None");
}

typedef struct
{
	int32_t x, y, w, h;
} ufontchar_t;

typedef struct
{
	int32_t texture;
	uint32_t charcount;
	ufontchar_t *chars;
} ufonttex_t;

typedef struct
{
	uint8_t texcount;
	ufonttex_t *tex;
} ufonthdr_t;

void savefont( int32_t namelen, char *name )
{
	char fname[256] = {0};
	FILE *f;
	if ( head->license == 2 )
	{
		printf(" FAIL: Postal 2 fonts not yet supported\n");
		return;
	}
	ufonthdr_t fhead;
	memset(&fhead,0,sizeof(ufonthdr_t));
	fhead.texcount = readbyte();
	fhead.tex = calloc(fhead.texcount,sizeof(ufonttex_t));
	for ( int i=0; i<fhead.texcount; i++ )
	{
		fhead.tex[i].texture = readindex();
		fhead.tex[i].charcount = readindex();
		fhead.tex[i].chars = calloc(fhead.tex[i].charcount,
			sizeof(ufontchar_t));
		for ( int j=0; j<fhead.tex[i].charcount; j++ )
		{
			fhead.tex[i].chars[j].x = readdword();
			fhead.tex[i].chars[j].y = readdword();
			fhead.tex[i].chars[j].w = readdword();
			fhead.tex[i].chars[j].h = readdword();
		}
	}
	// save to text
	snprintf(fname,256,"%.*s.txt",namelen,name);
	f = fopen(fname,"w");
	printf(" Dumping Font to %s in UTPT format\n",fname);
	fprintf(f,"Character: TextureName (X,Y)-(Width,Height)\n"
		"-------------------------------------------\n");
	int cc = 0;
	for ( int i=0; i<fhead.texcount; i++ )
	for ( int j=0; j<fhead.tex[i].charcount; j++ )
	{
		fprintf(f,"0x%02x: ",cc);
		construct_fullname(f,fhead.tex[i].texture);
		fprintf(f," (%d,%d)-(%d,%d)\n",fhead.tex[i].chars[j].x,
			fhead.tex[i].chars[j].y,fhead.tex[i].chars[j].w,
			fhead.tex[i].chars[j].h);
		cc++;
	}
	fclose(f);
	/*
	   TODO also fetch the textures referenced and extract the characters
	   from them into a folder (removing the need to use mkfont)
	*/
	// cleanup
	for ( int i=0; i<fhead.texcount; i++ ) free(fhead.tex[i].chars);
	free(fhead.tex);
}

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		printf("Usage: ufontext <archive>\n");
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
	if ( !hasname("Font") )
	{
		printf("Package %s does not contain Fonts\n",argv[1]);
		free(pkgfile);
		return 4;
	}
	// loop through exports and search for fonts
	fpos = head->oexports;
	for ( uint32_t i=0; i<head->nexports; i++ )
	{
		int32_t class, ofs, siz, name;
		readexport(&class,&ofs,&siz,&name);
		if ( (siz <= 0) || (class >= 0) ) continue;
		// get the class name
		class = -class-1;
		if ( (uint32_t)class > head->nimports ) continue;
		int32_t l = 0;
		char *n = (char*)(pkgfile+getname(getimport(class),&l));
		if ( strncmp(n,"Font",l) ) continue;
		char *fnt = (char*)(pkgfile+getname(name,&l));
		printf("Font found: %.*s\n",l,fnt);
		int32_t fntl = l;
#ifdef _DEBUG
		char fname[256] = {0};
		snprintf(fname,256,"%.*s.object",fntl,fnt);
		printf(" Dumping full object data to %s\n",fname);
		FILE *f = fopen(fname,"wb");
		fwrite(pkgfile+ofs,siz,1,f);
		fclose(f);
#endif
		// begin reading data
		size_t prev = fpos;
		fpos = ofs;
		if ( head->pkgver < 40 ) fpos += 8;
		if ( head->pkgver < 60 ) fpos += 16;
		int32_t prop = readindex();
		if ( (uint32_t)prop >= head->nnames )
		{
			printf("Unknown property %d, skipping\n",prop);
			fpos = prev;
			continue;
		}
		char *pname = (char*)(pkgfile+getname(prop,&l));
retry:
		if ( strncasecmp(pname,"none",l) )
		{
			uint8_t info = readbyte();
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
			fpos += psiz;
			printf(" Skipping property %.*s\n",l,pname);
			prop = readindex();
			pname = (char*)(pkgfile+getname(prop,&l));
			goto retry;
		}
#ifdef _DEBUG
		snprintf(fname,256,"%.*s.ufnt",fntl,fnt);
		printf(" Dumping full font struct to %s\n",fname);
		f = fopen(fname,"wb");
		fwrite(pkgfile+fpos,siz-(fpos-ofs),1,f);
		fclose(f);
#endif
		savefont(fntl,fnt);
		fpos = prev;
	}
	free(pkgfile);
	return 0;
}

