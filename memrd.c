#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFSZ 8192

int main( int argc, char** argv )
{
	if ( argc < 3 )
	{
		fputs("usage: memread <pid> <offset> <size>\n",stderr);
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
	off_t memofs = 0;
	off_t siz = 0, srd;
	sscanf(argv[2],"%llx",&memofs);
	sscanf(argv[3],"%lld",&siz);
	lseek(fd,memofs,SEEK_SET);
	unsigned char buf[BUFSZ];
	while ( siz > 0 )
	{
		srd = read(fd,buf,(siz>BUFSZ)?BUFSZ:siz);
		write(1,buf,srd);
		siz -= srd;
	}
	close(fd);
	return 0;
}
