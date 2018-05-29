/* needed for usleep(), feature test macros are annoying */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <linux/input.h>

int fd;

/* beep boop */
void beep( int freq )
{
	struct input_event e =
	{
		.type = EV_SND,
		.code = SND_TONE,
		.value = freq,
	};
	write(fd,&e,sizeof(struct input_event));
}

void sighnd( int signum )
{
	switch ( signum )
	{
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		if ( fd >= 0 )
		{
			/* better clean up or this is going to beep forever */
			beep(0);
			close(fd);
		}
		exit(signum);
	}
}

int main( int argc, char **argv )
{
	signal(SIGINT,sighnd);
	signal(SIGTERM,sighnd);
	signal(SIGQUIT,sighnd);
	if ( argc < 2 )
		return fprintf(stderr,"usage: %s <device path>\n",argv[0])||1;
	fd = open(argv[1],O_RDWR);
	if ( fd < 0 )
		return fprintf(stderr,"failed to open %s: %s\n",argv[1],
			strerror(errno))||1;
	uint8_t ver[4] = {0};
	if ( ioctl(fd,EVIOCGVERSION,ver) == -1 )
		return fprintf(stderr,"ioctl failed: %s\n",strerror(errno))||1;
	printf("Driver ver: %hhu.%hhu.%hhu\n",ver[2],ver[1],ver[0]);
	struct input_id id = {0};
	ioctl(fd,EVIOCGID,&id);
	printf("ID: bus %#06hx vendor %#06hx product %#06hx ver %#06hx\n",
		id.bustype,id.vendor,id.product,id.version);
	char name[256] = "Unknown device";
	ioctl(fd,EVIOCGNAME(256),name);
	printf("Name: %s\n",name);
	uint64_t evbits, sndbits;
	if ( ioctl(fd,EVIOCGBIT(0,8),&evbits) == -1 )
		return fprintf(stderr,"ioctl failed: %s\n",strerror(errno))||1;
	if ( !((evbits>>EV_SND)&1) )
		return fprintf(stderr,"No sound support\n")||1;
	if ( ioctl(fd,EVIOCGBIT(EV_SND,8),&sndbits) == -1 )
		return fprintf(stderr,"ioctl failed: %s\n",strerror(errno))||1;
	if ( !((sndbits>>SND_TONE)&1) )
		return fprintf(stderr,"No tone event support\n")||1;
	int freq = 200, time = 200;
	printf("Testing %dHz tone for %dms\n",freq,time);
	beep(200);
	usleep(time*1000UL);
	beep(0);
	close(fd);
	return 0;
}
