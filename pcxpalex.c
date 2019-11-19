#include <stdio.h>
#include <stdint.h>

int main( int argc, char **argv )
{
	FILE *fin = fopen(argv[1],"rb");
	uint8_t head[4];
	fread(&head[0],4,1,fin);
	if ( (head[0] != 0x0A) || (head[2] != 0x01) || (head[3] != 0x08) )
	{
		fclose(fin);
		return 1;
	}
	uint8_t pal[768];
	fseek(fin,-769,SEEK_END);
	if ( fgetc(fin) != 0x0C )
	{
		fclose(fin);
		return 2;
	}
	fread(&pal[0],768,1,fin);
	fclose(fin);
	fwrite(&pal[0],768,1,stdout);
	return 0;
}
