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
	int i = 0;
keepreading:
	nread = fread(buf,1,4,fin);
	if ( feof(fin) || (nread < 4) )
	{
		fclose(fin);
		return 0;
	}
	if ( strncmp(buf,"RIFF",4) )
	{
		fseek(fin,-3,SEEK_CUR);
		goto keepreading;
	}
	char fname[256];
	snprintf(fname,256,"SND%d.wav",i);
	fout = fopen(fname,"wb");
	fwrite(buf,1,4,fout);
	nread = fread(buf,4,1,fin);
	fwrite(buf,1,4,fout);
	pread = (*(uint32_t*)buf)-4;
	while ( pread > 0 )
	{
		nread = fread(buf,1,min(131072,pread),fin);
		fwrite(buf,1,nread,fout);
		pread -= nread;
	}
	fclose(fout);
	i++;
	if ( !feof(fin) ) goto keepreading;
	fclose(fin);
	return 0;
}
