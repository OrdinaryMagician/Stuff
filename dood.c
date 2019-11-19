#include <stdio.h>
#include <errno.h>
#include <string.h>

int nspace( unsigned short ch )
{
	return ((ch>0x2F)&&(ch<0x3A))||((ch>0x40)&&(ch<0x5B))
		||((ch>0x60)&&(ch<0x7B));
}

int main( int argc, char **argv )
{
	if ( argc < 3 )
	{
		fprintf(stderr,"usage: dood <input> <output>\n");
		return 0;
	}
	unsigned short endoom[2000];
	FILE *fin = fopen(argv[1],"rb");
	if ( !fin )
	{
		fprintf(stderr,"failed to open input: %s.\n",strerror(errno));
		return 1;
	}
	int nch = 0;
	if ( (nch = fread(endoom,sizeof(short),2000,fin)) < 2000 )
	{
		fprintf(stderr,"short read, expected 2000 chars got %d.\n",nch);
		fclose(fin);
		return 2;
	}
	fclose(fin);
	int c = 0;
	do
	{
		if ( nspace(endoom[c]&0xFF) )
		{
			int s = c;
			while ( (c++ < 2000 ) && nspace(endoom[c]&0xFF) );
			int e = c;
			for ( int i=0; i<(e-s)/2; i++ )
				endoom[e-i-1] = endoom[s+i];
		}
		else c++;
	}
	while ( c < 2000 );
	FILE *fout = fopen(argv[2],"wb");
	if ( !fout )
	{
		fprintf(stderr,"failed to open output: %s.\n",strerror(errno));
		return 4;
	}
	fwrite(endoom,sizeof(short),2000,fout);
	close(fout);
	return 0;
}
