#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct
{
	union
	{
		uint8_t type[4];
		uint32_t typehex;
	};
	uint32_t ver, len;
	uint8_t *data;
} __attribute__((packed)) skse_chunk_t;

typedef struct
{
	union
	{
		uint8_t magic[4];
		uint32_t magichex;
	};
	uint32_t nchunks, len;
	skse_chunk_t *chunks;
} __attribute__((packed)) skse_plugin_t;

typedef struct
{
	union
	{
		uint8_t magic[4];
		uint32_t magichex;
	};
	uint32_t fmtver;
	uint32_t sksever;
	uint32_t rtver;
	uint32_t nplugins;
	skse_plugin_t *plugins;
} __attribute__((packed)) skse_t;

void rawdatadump( const skse_chunk_t *chnk )
{
	int i = 0, len = chnk->len;
	while ( i<len )
	{
		int j, jmax = ((len-i)<16)?(len-i):16;
		for ( j=0; j<jmax; j++ ) printf("%02x ",chnk->data[i+j]);
		for ( ; j<16; j++ ) printf("   ");
		for ( j=0; j<jmax; j++ ) if ( (chnk->data[i+j] >= 0x20)
			&& (chnk->data[i+j] < 0x7f) )
				putchar(chnk->data[i+j]);
			else putchar('.');
		for ( ; j<16; j++ ) putchar(' ');
		putchar('\n');
		i += jmax;
	}
}

void getmodlist( const uint8_t *dat )
{
	int nmods = dat[0];
	printf("Listing %u plugin files:\n",dat[0]);
	int i, p = 1;
	for ( i=0; i<nmods; i++ )
	{
		int sl, ps = 0;
		sl = dat[p++]|((dat[p++]<<8)&0xFF00);
		printf("  %02X - ",i);
		while ( ps++ < sl ) putchar(dat[p++]);
		putchar('\n');
	};
}

int main( int argc, char **argv )
{
	if ( argc < 1 ) return 1;
	FILE *sf = fopen(argv[1],"r");
	if ( !sf ) return 2;
	skse_t skse = {0};
	fread(&skse,20,1,sf);
	skse.plugins = malloc(sizeof(skse_plugin_t)*skse.nplugins);
	printf("Magic: %08x \"%.4s\"\n",skse.magichex,skse.magic);
	printf("Format version: %u\n",skse.fmtver);
	int maj, min, beta, sub;
	maj = (skse.sksever>>24)&0xFF;
	min = (skse.sksever>>16)&0xFF;
	beta = (skse.sksever>>4)&0xFFF;
	printf("SKSE version: %d.%d.%d\n",maj,min,beta);
	maj = (skse.rtver>>24)&0xFF;
	min = (skse.rtver>>16)&0xFF;
	beta = (skse.rtver>>4)&0xFFF;
	printf("Runtime version: 1.%d.%d.%d\n",maj,min,beta);
	printf("Number of plugins: %u\n",skse.nplugins);
	skse_plugin_t *plug;
	int i;
	for ( i=0; i<skse.nplugins; i++ )
	{
		plug = &skse.plugins[i];
		fread(plug,12,1,sf);
		printf("------ %08x \"%c%c%c%c\" ",plug->magichex,
			plug->magic[3],plug->magic[2],plug->magic[1],
			plug->magic[0]);
		printf("%u chunks ",plug->nchunks);
		printf("%u bytes\n",plug->len);
		plug->chunks = malloc(sizeof(skse_chunk_t)*plug->nchunks);
		skse_chunk_t *chnk;
		int j;
		for ( j=0; j<plug->nchunks; j++ )
		{
			chnk = &plug->chunks[j];
			fread(chnk,12,1,sf);
			chnk->data = malloc(chnk->len);
			fread(chnk->data,chnk->len,1,sf);
			printf("--- %08x \"%c%c%c%c\" ",chnk->typehex,
				chnk->type[3],chnk->type[2],chnk->type[1],
				chnk->type[0]);
			printf("v%u ",chnk->ver);
			printf("%u bytes\n",chnk->len);
			if ( chnk->typehex == 0x4d4f4453 )
				getmodlist(chnk->data);
			else rawdatadump(chnk);
		}
	}
	fclose(sf);
	return 0;
}
