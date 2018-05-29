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

uint32_t readdword( void )
{
	uint32_t val = *(uint32_t*)(pkgfile+fpos);
	fpos += 4;
	return val;
}

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
		while ( c = readbyte() ) p++;
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
	for ( int i=0; i<head->nnames; i++ )
	{
		int32_t len = 0;
		if ( head->pkgver >= 64 )
		{
			len = readindex();
			if ( len <= 0 ) continue;
		}
		int c = 0, p = 0, match = 1;
		while ( c = readbyte() )
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

void savemusic( uint8_t *mdata, int32_t msize, int32_t name, int32_t ext )
{
	char fname[512] = {0};
	int32_t l = 0;
	char *tname = pkgfile+getname(name,&l);
	strncpy(fname,tname,l);
	// guess format from signature
	if ( !strncmp(mdata,"IMPM",4) )
	{
		printf("Format: Impulse Tracker\n");
		strcat(fname,".it");
	}
	else if ( !strncmp(mdata+44,"SCRM",4) )
	{
		printf("Format: Scream Tracker\n");
		strcat(fname,".s3m");
	}
	else if ( !strncmp(mdata,"Extended Module: ",17) )
	{
		printf("Format: Extended Module\n");
		strcat(fname,".xm");
	}
	else if ( !strncmp(mdata,"OggS",4) )
	{
		printf("Format: Ogg Vorbis\n");
		strcat(fname,".ogg");
	}
	else
	{
		// fall back to what I ASSUME is an embedded file extension
		// in the name table, but I could be very wrong
		char *ename = pkgfile+getname(ext,&l);
		printf("Guessed extension: %.*s\n",l,ename);
		strcat(fname,".");
		strncat(fname,ename,l);
	}
	int fd = open(fname,O_CREAT|O_TRUNC|O_WRONLY,0644);
	if ( !fd )
	{
		printf("Cannot open %s for writing: %s\n",fname,
			strerror(errno));
		return;
	}
	int r = 0, wn = msize;
	do
	{
		r = write(fd,mdata,wn);
		if ( r == -1 )
		{
			close(fd);
			printf("Write failed for file %s: %s\n",fname,
				strerror(errno));
			return;
		}
		mdata += r;
		wn -= r;
	}
	while ( wn > 0 );
	close(fd);
}

int main( int argc, char **argv )
{
	if ( argc < 2 ) return 1;
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
	if ( !hasname("Music") )
	{
		printf("Package %s does not contain music\n",argv[1]);
		free(pkgfile);
		return 4;
	}
	// loop through exports and search for music
	fpos = head->oexports;
	for ( int i=0; i<head->nexports; i++ )
	{
		int32_t class, ofs, siz, name;
		readexport(&class,&ofs,&siz,&name);
		if ( (siz <= 0) || (class >= 0) ) continue;
		// get the class name
		int ismusic = 0;
		class = -class-1;
		if ( class > head->nimports ) continue;
		int32_t l = 0;
		char *n = pkgfile+getname(getimport(class),&l);
		if ( strncmp(n,"Music",l) ) continue;
		char *trk = pkgfile+getname(name,&l);
		printf("Track found: %.*s\n",l,trk);
		// begin reading data
		size_t prev = fpos;
		fpos = ofs;
		if ( head->pkgver < 40 ) fpos += 8;
		if ( head->pkgver < 60 ) fpos += 16;
		int32_t prop = readindex();
		if ( prop >= head->nnames ) continue;
		char *pname = pkgfile+getname(prop,&l);
		if ( strncasecmp(pname,"none",l) ) continue;
		int32_t ext;
		if ( head->pkgver >= 120 )
		{
			ext = readindex();
			fpos += 8;
		}
		else if ( head->pkgver >= 100 )
		{
			fpos += 4;
			ext = readindex();
			fpos += 4;
		}
		else if ( head->pkgver >= 62 )
		{
			ext = readindex();
			fpos += 4;
		}
		else ext = readindex();
		int32_t msize = readindex();
		savemusic(pkgfile+fpos,msize,name,ext);
		fpos = prev;
	}
	free(pkgfile);
	return 0;
}
