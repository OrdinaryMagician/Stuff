#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

int main( int argc, char** argv )
{
	if ( argc < 3 )
	{
		fputs("usage: memseek <pid> <range> <size>\n",stderr);
		return 1;
	}
	char fn[256];
	snprintf(fn,256,"/proc/%s/mem",argv[1]);
	int fd = open(fn,O_RDONLY);
	if ( !fd )
	{
		perror("could not open process");
		return 2;
	}
	off_t memofs[2] = {0,0};
	off_t siz = 0, rsiz;
	sscanf(argv[2],"%llx-%llx",&memofs[0],&memofs[1]);
	sscanf(argv[3],"%lld",&siz);
	unsigned char *seekme = malloc(siz);
	if ( !seekme ) return 4;
	rsiz = read(0,seekme,siz);
	if ( rsiz <= 0 ) return 8;
	off_t seeksiz = memofs[1]-memofs[0];
	lseek(fd,memofs[0],SEEK_SET);
	unsigned char *seekarea = malloc(seeksiz);
	if ( !seekarea ) return 16;
	read(fd,seekarea,seeksiz);
	close(fd);
	off_t i;
	for ( i=0; i<seeksiz-rsiz; i++ ) if ( !memcmp(seekme,seekarea+i,rsiz) )
		printf("%x\n",memofs[0]+i);
	return 0;
}
