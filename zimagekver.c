#include <stdio.h>
#include <string.h>
#include <errno.h>

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		fprintf(stderr,"No file provided!\n");
		return 1;
	}
	FILE *k;
	if ( !(k = fopen(argv[1],"r")) )
	{
		fprintf(stderr,"Could not open file: %s\n",strerror(errno));
		return 2;
	}
	/* seek to fields */
	if ( fseek(k,0x24,SEEK_SET) == -1 )
	{
		fprintf(stderr,"Could not seek in file for header: %s\n",
			strerror(errno));
		fclose(k);
		return 4;
	}
	/* read important info */
	unsigned sig = 0, start = 0, end = 0;
	fread(&sig,4,1,k);
	fread(&start,4,1,k);
	fread(&end,4,1,k);
	if ( sig != 0x016f2818 )
	{
		fprintf(stderr,"Bad magic %08x, not a valid zImage\n",sig);
		fclose(k);
		return 4;
	}
	/*
	   Search for Linux version at end, we need to find "DTOK" followed
	   by "Linux version". This normally starts 64 bytes after the zImage.
	*/
	if ( fseek(k,end+0x40,SEEK_SET) == -1 )
	{
		fprintf(stderr,
			"Could not seek in file for version string: %s\n",
			strerror(errno));
		fclose(k);
		return 8;
	}
	char dtok[4] = "";
	fread(dtok,1,4,k);
	if ( strncmp(dtok,"DTOK",4) )
	{
		fprintf(stderr,"Expected \"DTOK\", got \"%.4s\"\n",dtok);
		fclose(k);
		return 16;
	}
	char kver[128] = "";
	fgets(kver,128,k);
	fclose(k);
	if ( strncmp(kver,"Linux version ",14) )
	{
		fprintf(stderr,"Expected \"Linux version \", got \"%.14s\"\n",
			kver);
		return 32;
	}
	/* get the actual version string and print it */
	char kernver[128] = "";
	sscanf(kver,"Linux version %s",kernver);
	printf("%s\n",kernver);
	return 0;
}
