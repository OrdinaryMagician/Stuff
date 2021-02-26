#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

int main( int argc, char **argv )
{
	if ( argc < 2 ) return 1;
	char tname[256] = {0};
	unsigned cell = 0;
	int x = 0, y = 0, w = 0, h = 0;
	char cropargs[256] = {0};
	char cname[256] = {0};
	char* hargs[7] =
	{
		"convert",
		tname,
		"-crop",
		cropargs,
		"+repage",
		cname,
		0
	};
	mkdir(argv[1],S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
	while ( !feof(stdin) )
	{
		int ret = scanf("0x%x: %s (%d,%d)-(%d,%d)\n",&cell,tname,&x,&y,&w,&h);
		if ( ret != 6 )
		{
			printf("%d != 6\n",ret);
			return 2;
		}
		strcat(tname,".png");
		if ( (w <= 0) || (h <= 0) ) continue;
		printf("%u, %s %+d%+d,%d,%d\n",cell,tname,x,y,w,h);
		sprintf(cname,"%s/%04x.png",argv[1],cell);
		sprintf(cropargs,"%dx%d%+d%+d",w,h,x,y);
		int pid = fork();
		if ( !pid ) execvp(hargs[0],hargs);
		else waitpid(pid,0,0);
	}
	return 0;
}
