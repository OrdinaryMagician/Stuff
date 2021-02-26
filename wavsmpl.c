#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

typedef struct
{
	char magic[4];
	uint32_t size;
	char type[4];
} riff_t;

typedef struct
{
	char id[4];
	uint32_t size;
} chunk_t;

typedef struct
{
	uint32_t mf, prod, sp, mnote, mpitch, sfmt, sofs, nloops, sdata;
} smpl_t;

typedef struct
{
	uint32_t id, type, start, end, fraction, count;
} sloop_t;

int main( int argc, char **argv )
{
	if ( argc < 2 ) return 1;
	FILE *fin;
	for ( int i=1; i<argc; i++ )
	{
		if ( !(fin = fopen(argv[i],"rb")) )
		{
			fprintf(stderr,"couldn't open %s: %s\n",argv[i],strerror(errno));
			continue;
		}
		riff_t head;
		if ( fread(&head,1,sizeof(riff_t),fin) != sizeof(riff_t) )
		{
			fprintf(stderr,"%s: premature end of file reached reading file header\n",argv[i]);
			goto skipme;
		}
		if ( strncmp(head.magic,"RIFF",4) )
		{
			fprintf(stderr,"%s: invalid RIFF header\n",argv[i]);
			goto skipme;
		}
		while ( !feof(fin) )
		{
			chunk_t chk;
			if ( fread(&chk,1,sizeof(chunk_t),fin) != sizeof(chunk_t) )
			{
				fprintf(stderr,"%s: premature end of file reached reading chunk header\n",argv[i]);
				goto skipme;
			}
			if ( strncmp(chk.id,"smpl",4) )
			{
				printf("Skipping over %.4s chunk (%u bytes)\n",chk.id,chk.size);
				fseek(fin,chk.size,SEEK_CUR);
				continue;
			}
			printf("smpl chunk found\n");
			smpl_t smpl;
			if ( fread(&smpl,1,sizeof(smpl_t),fin) != sizeof(smpl_t) )
			{
				fprintf(stderr,"%s: premature end of file reached reading smpl data\n",argv[i]);
				goto skipme;
			}
			printf("%u loops contained\n",smpl.nloops);
			for ( uint32_t j=0; j<smpl.nloops; j++ )
			{
				sloop_t loop;
				if ( fread(&loop,1,sizeof(sloop_t),fin) != sizeof(sloop_t) )
				{
					fprintf(stderr,"%s: premature end of file reached reading smpl loop\n",argv[i]);
					goto skipme;
				}
				printf("ID: %u\n type: %u\n start: %u\n end: %u\n fraction: %u\n count: %u\n",
					loop.id,loop.type,loop.start,loop.end,loop.fraction,loop.count);
			}
		}
skipme:
		fclose(fin);
	}
	return 0;
}
