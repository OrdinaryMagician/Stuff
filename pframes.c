#include <stdio.h>
#include <string.h>

int main( int argc, char **argv )
{
	if ( argc < 5 ) return 1;
	char s[4] = "TNT1";
	char f = 'A';
	int fi = 0;
	int sp = 1;
	int max = 0;
	int sk = 1;
	strncpy(s,argv[1],4);
	sscanf(argv[2],"%d",&fi);
	sscanf(argv[3],"%d",&max);
	sscanf(argv[4],"%d",&sk);
	for ( int i=0; i<max; i+=sk )
	{
		printf("\tFrameIndex %.4s %c 0 %d\n",s,f,fi);
		fi+=sk;
		f++;
		if ( f > 'Z' )
		{
			sp++;
			if ( sp == 2 )
				s[2] = s[3];
			s[3] = '0'+sp;
			f = 'A';
		}
	}
	return 0;
}
