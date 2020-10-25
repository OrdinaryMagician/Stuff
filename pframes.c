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
	int mdl = 0;
	strncpy(s,argv[1],4);
	sscanf(argv[2],"%d",&fi);
	sscanf(argv[3],"%d",&max);
	sscanf(argv[4],"%d",&sk);
	if ( argc > 5 ) sscanf(argv[5],"%d",&mdl);
	if ( argc > 6 ) sscanf(argv[6],"%c",&f);
	for ( int i=0; i<max; i+=sk )
	{
		printf("\tFrameIndex %.4s %c %d %d\n",s,f,mdl,fi);
		fi+=sk;
		f++;
		if ( f > 'Z' )
		{
			if ( s[3] == '9' ) s[3] = 'A';
			else if ( s[3] == 'Z' )
			{
				s[2]++;
				s[3] = '0';
			}
			else s[3]++;
			f = 'A';
		}
	}
	return 0;
}
