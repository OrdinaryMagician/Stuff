#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define min(a,b) (((a)<(b))?(a):(b))

int main( int argc, char **argv )
{
	if ( argc < 2 ) return 1;
	FILE *fin, *fout;
	if ( !(fin = fopen(argv[1],"rb")) )
		return 2;
	unsigned char buf[131072];
	size_t nread = 0, pread = 0;
	nread = fread(buf,1,4,fin);
	if ( feof(fin) || (nread < 4) || strncmp(buf,"FUZE",4) )
	{
		fclose(fin);
		return 1;
	}
	// skip to offset of audio data
	// right into RIFF header
	fseek(fin,4,SEEK_CUR);
	nread = fread(buf,1,4,fin);
	fseek(fin,(*(uint32_t*)buf)+12,SEEK_SET);
	nread = fread(buf,1,4,fin);
	if ( feof(fin) || (nread < 4) || strncmp(buf,"RIFF",4) )
	{
		fclose(fin);
		return 4;
	}
	char fname[256];
	char *ext = strrchr(argv[1],'.');
	strcpy(ext,".xwm");
	fout = fopen(argv[1],"wb");
	// write everything until EOF
	fwrite(buf,1,4,fout);
	while ( !feof(fin) )
	{
		nread = fread(buf,1,131072,fin);
		fwrite(buf,1,nread,fout);
	}
	fclose(fout);
	fclose(fin);
	return 0;
}
