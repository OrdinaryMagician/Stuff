/*
	dtexdupes.c : Find textures with identical names in multiple doom mods.
	Supports wad, pk3 and folders. Handles TEXTURE1/2 and TEXTURES.
	(C)2018 Marisa Kirisame, UnSX Team.
	Released under the GNU General Public License version 3 (or later).
	Requires libarchive. Made for *nix platforms.
*/
#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <archive.h>
#include <archive_entry.h>
#include <sys/stat.h>
#include <ftw.h>

// wad stuff
typedef struct
{
	char id[4];
	uint32_t nlumps;
	uint32_t dirpos;
} wad_t;

typedef struct
{
	uint32_t pos;
	uint32_t size;
	char name[8];
} lump_t;

// list of texture names
uint8_t **tex = 0;
size_t ntex = 0, nresv = 0;

void freetex( void )
{
	if ( !tex ) return;
	for ( int i=0; i<ntex; i++ )
		if ( tex[i] ) free(tex[i]);
	free(tex);
}

int findtex( const char *name, uint8_t len )
{
	for ( int i=0; i<ntex; i++ )
		if ( !strncasecmp(tex[i],name,len) )
			return 1;
	return 0;
}

// we don't need a very large chunk size, this isn't a time-critical program
#define CHUNK_SZ 64

int appendtex( const char *name, uint8_t len )
{
	if ( findtex(name,len) )
	{
		printf("  Texture %.*s already defined!\n",len,name);
		return 0;
	}
	if ( !tex )
	{
		tex = malloc(sizeof(uint8_t*)*CHUNK_SZ);
		nresv = CHUNK_SZ;
	}
	else if ( ntex >= nresv )
	{
		nresv += CHUNK_SZ;
		tex = realloc(tex,sizeof(uint8_t*)*nresv);
	}
	// append
	tex[ntex] = malloc(len+1);
	strncpy(tex[ntex],name,len);
	tex[ntex][len] = '\0';	// guarantee null-termination
	ntex++;
}

int processtexture12( uint8_t type, uint8_t *data )
{
	// parse TEXTURE1/2 lumps
	int32_t nent = *(int32_t*)data;
	int32_t *ofs = (int32_t*)(data+4);
	for ( int i=0; i<nent; i++ )
	{
		uint8_t *name = (uint8_t*)(data+ofs[i]);
		appendtex(name,8);
	}
	printf(" TEXTURE%c read (%d entries)\n",type,nent);
	return nent;
}

int processtextures( uint8_t *data )
{
	// parse TEXTURES entries
	printf(" TEXTURES not yet supported, skipping\n");
	return 0;	// TODO
}

int processflats( lump_t *head )
{
	// check everything between F_START and F_END
	int cnt = 0;
	lump_t *cur = head;
	while ( cur )
	{
		cur++;
		if ( !strncmp(cur->name,"F_END",8) ) break;
		if ( cur->size != 4096 ) continue;
		appendtex(cur->name,8);
		cnt++;
	}
	printf(" F_START/END read (%d entries)\n",cnt);
	return cnt;
}

int processtxsection( lump_t *head )
{
	// check everything between TX_START and TX_END
	int cnt = 0;
	lump_t *cur = head;
	while ( cur )
	{
		cur++;
		if ( !strncmp(cur->name,"TX_END",8) ) break;
		appendtex(cur->name,8);
		cnt++;
	}
	printf(" TX_START/END read (%d entries)\n",cnt);
	return cnt;
}

int processwad( uint8_t *w )
{
	lump_t *dir = (lump_t*)(w+((wad_t*)w)->dirpos);
	for ( int i=0; i<((wad_t*)w)->nlumps; i++ )
	{
		if ( !strncmp(dir->name,"TEXTURES",8) )
			processtextures(w+dir->pos);
		else if ( !strncmp(dir->name,"TEXTURE1",8)
			|| !strncmp(dir->name,"TEXTURE2",8) )
			processtexture12(dir->name[7],w+dir->pos);
		else if ( !strncmp(dir->name,"TX_START",8) )
			processtxsection(dir);
		else if ( !strncmp(dir->name,"F_START",8) )
			processflats(dir);
		dir++;
	}
}

int processtxfolder_arc( struct archive *ar )
{
	// libarchive apparently has no concept of "folders" so we have to
	// loop through everything here and check its full path
	return 0;	// TODO
}

static int ncnt = 0, fcnt = 0;

static int ftw_callback( const char *path, const struct stat *st,
	const int type, struct FTW *info )
{
	const char *bname = path+info->base;
	if ( info->level == 1 )
	{
		if ( (type == FTW_D) && strcasecmp(bname,"textures") && strcasecmp(bname,"flats") )
			return FTW_SKIP_SUBTREE;
		if ( type != FTW_F ) return FTW_CONTINUE;
		FILE *f;
		if ( !(f = fopen(path,"rb")) )
		{
			fprintf(stderr,"cannot open %s: %s\n",bname,
				strerror(errno));
			return FTW_CONTINUE;
		}
		char magic[4];
		if ( fread(magic,1,4,f) == 4 )
		{
			if ( !strncmp(magic,"IWAD",4)
				|| !strncmp(magic,"PWAD",4) )
			{
				uint8_t *w = malloc(st->st_size);
				if ( !w )
				{
					fprintf(stderr,"cannot allocate memory"
						" for %s: %s\n",bname,
						strerror(errno));
					fclose(f);
					return FTW_CONTINUE;
				}
				fseek(f,0,SEEK_SET);
				fread(w,1,st->st_size,f);
				fclose(f);
				printf("Processing %.4s %s\n",magic,bname);
				ncnt += processwad(w);
				free(w);
				return FTW_CONTINUE;
			}
		}
		fseek(f,0,SEEK_SET);
		if ( !strncasecmp(bname,"TEXTURES",8) )
		{
			uint8_t *d;
			if ( !(d = malloc(st->st_size)) )
			{
				fprintf(stderr,"cannot allocate memory for %s:"
					" %s\n",bname,strerror(errno));
				fclose(f);
				return FTW_CONTINUE;
			}
			fread(d,1,st->st_size,f);
			ncnt += processtextures(d);
			free(d);
		}
		else if ( !strncasecmp(bname,"TEXTURE1",8)
			|| !strncasecmp(bname,"TEXTURE2",8) )
		{
			uint8_t *d;
			if ( !(d = malloc(st->st_size)) )
			{
				fprintf(stderr,"cannot allocate memory for %s:"
					" %s\n",bname,strerror(errno));
				fclose(f);
				return FTW_CONTINUE;
			}
			fread(d,1,st->st_size,f);
			ncnt += processtexture12(bname[7],d);
			free(d);
		}
		fclose(f);
		return FTW_CONTINUE;
	}
	if ( type == FTW_F )
	{
		// strip extension
		char *ext = strchr(bname,'.');
		if ( ext ) *ext = '\0';
		appendtex(bname,8);
		ncnt++;
		fcnt++;
	}
	return FTW_CONTINUE;
}

int processtxfolder( const char *path )
{
	ncnt = 0, fcnt = 0;
	// recurse through specified directory and check any and all files
	nftw(path,ftw_callback,15,FTW_ACTIONRETVAL|FTW_PHYS);
	printf(" Directory read (%d entries)\n",fcnt);
	return ncnt;
}

int main( int argc, char **argv )
{
	struct stat st;
	FILE *f;
	int tot = 0;
	// sift through all parameters
	for ( int i=1; i<argc; i++ )
	{
		if ( stat(argv[i],&st) )
		{
			fprintf(stderr,"cannot stat %s: %s\n",argv[i],
				strerror(errno));
			continue;
		}
		if ( S_ISDIR(st.st_mode) )
		{
			printf("Processing directory %s\n",argv[i]);
			tot += processtxfolder(argv[i]);
			continue;
		}
		else if ( !(f = fopen(argv[i],"rb")) )
		{
			fprintf(stderr,"cannot open file %s: %s\n",argv[i],
				strerror(errno));
			continue;
		}
		/* check for wad */
		char magic[4];
		if ( fread(magic,1,4,f) == 4 )
		{
			if ( !strncmp(magic,"IWAD",4)
				|| !strncmp(magic,"PWAD",4) )
			{
				uint8_t *w = malloc(st.st_size);
				if ( !w )
				{
					fprintf(stderr,"cannot allocate memory"
						" for %s: %s\n",argv[i],
						strerror(errno));
					fclose(f);
					continue;
				}
				fseek(f,0,SEEK_SET);
				fread(w,1,st.st_size,f);
				fclose(f);
				printf("Processing %.4s %s\n",magic,argv[i]);
				tot = processwad(w);
				free(w);
				continue;
			}
		}
		fclose(f);
	}
	freetex();
	return 0;
}
