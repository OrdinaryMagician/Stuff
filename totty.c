#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main( int argc, char **argv )
{
	char *tty = "/dev/tty1";
	if ( argc > 1 ) tty = argv[1];
	int fd = open(tty,O_RDWR);
	if ( !fd == -1 )
	{
		perror("open tty failed");
		return 1;
	}
	int mchar = 0;
	while ( (mchar = getchar()) != EOF )
		ioctl(fd,TIOCSTI,&mchar);
	close(fd);
	return 0;
}
