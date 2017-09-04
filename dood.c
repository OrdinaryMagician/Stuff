#include <stdio.h>

unsigned short endoom[80*25];

int val( unsigned short ch )
{
	return ((ch>0x2F)&&(ch<0x3A))||((ch>0x40)&&(ch<0x5B))
		||((ch>0x60)&&(ch<0x7B));
}

int main( void )
{
	fread(endoom,sizeof(short),80*25,stdin);
	int c = 0;
	do
	{
		if ( val(endoom[c]&0xFF) )
		{
			int s = c;
			while ( (c++ < 80*25 ) && val(endoom[c]&0xFF) );
			int e = c;
			for ( int i=0; i<(e-s)/2; i++ )
				endoom[e-i-1] = endoom[s+i];
		}
		else c++;
	}
	while ( c < 80*25 );
	fwrite(endoom,sizeof(short),80*25,stdout);
	return 0;
}
