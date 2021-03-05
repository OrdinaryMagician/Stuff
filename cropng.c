#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <endian.h>
#include <png.h>

const char* fmtname( int bd, int col )
{
	if ( col == PNG_COLOR_TYPE_PALETTE )
		return " paletted";
	else if ( col == PNG_COLOR_TYPE_GRAY_ALPHA )
		return (bd==16)?" 16bpc grayscale":" grayscale";
	else if ( col == PNG_COLOR_TYPE_RGB_ALPHA )
		return (bd==16)?" 16bpc rgb":" rgb";
	return "";
}

int loadpng( const char *fname, uint8_t **idata, uint32_t *w, uint32_t *h, int32_t *x, int32_t *y, int *pxsize, png_color **plte, int *nplt, uint8_t **trns, int *ntrns, uint8_t *bd, uint8_t *col )
{
	if ( !fname ) return 0;
	png_structp pngp;
	png_infop infp;
	unsigned int sread = 0;
	FILE *pf;
	if ( !(pf = fopen(fname,"rb")) )
	{
		fprintf(stderr," Cannot open for reading: %s.\n",strerror(errno));
		return 0;
	}
	pngp = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if ( !pngp )
	{
		fclose(pf);
		return 0;
	}
	infp = png_create_info_struct(pngp);
	if ( !infp )
	{
		fclose(pf);
		png_destroy_read_struct(&pngp,0,0);
		return 0;
	}
	if ( setjmp(png_jmpbuf(pngp)) )
	{
		png_destroy_read_struct(&pngp,&infp,0);
		fclose(pf);
		return 0;
	}
	png_init_io(pngp,pf);
	png_set_sig_bytes(pngp,sread);
	png_set_keep_unknown_chunks(pngp,PNG_HANDLE_CHUNK_ALWAYS,(uint8_t*)"grAb\0alPh\0",2);
	png_read_png(pngp,infp,PNG_TRANSFORM_PACKING,0);
	*w = png_get_image_width(pngp,infp);
	*h = png_get_image_height(pngp,infp);
	*bd = png_get_bit_depth(pngp,infp);
	if ( (*bd != 8) && (*bd != 16) )
	{
		fprintf(stderr," Unsupported bit depth of '%d' (only 8bpc and 16bpc supported).\n",*bd);
		png_destroy_read_struct(&pngp,&infp,0);
		fclose(pf);
		return 0;
	}
	*col = png_get_color_type(pngp,infp);
	if ( (*col == PNG_COLOR_TYPE_GRAY) || (*col == PNG_COLOR_TYPE_RGB) )
	{
		fprintf(stderr," Image has no alpha channel.\n");
		png_destroy_read_struct(&pngp,&infp,0);
		fclose(pf);
		return 0;
	}
	else if ( *col == PNG_COLOR_TYPE_PALETTE )
	{
		*pxsize = 1;
		if ( !png_get_valid(pngp,infp,PNG_INFO_tRNS) )
		{
			fprintf(stderr," Paletted image has no tRNS chunk.\n");
			png_destroy_read_struct(&pngp,&infp,0);
			fclose(pf);
			return 0;
		}
		// for whatever reason these arrays have to be allocated and memcpy'd
		// otherwise they'll be corrupted when we use them later
		//
		// there's no logical explanation for this
		// the manual doesn't mention anything about it
		uint8_t *trns_local;
		png_get_tRNS(pngp,infp,&trns_local,ntrns,0);
		*trns = malloc(*ntrns);
		memcpy(*trns,trns_local,*ntrns);
		png_color *plte_local;
		png_get_PLTE(pngp,infp,&plte_local,nplt);
		*plte = malloc(sizeof(png_color)*(*nplt));
		memcpy(*plte,plte_local,sizeof(png_color)*(*nplt));
	}
	else if ( *col == PNG_COLOR_TYPE_GRAY_ALPHA ) *pxsize = (*bd>>3)*2;
	else if ( *col == PNG_COLOR_TYPE_RGB_ALPHA ) *pxsize = (*bd>>3)*4;
	else
	{
		fprintf(stderr," Image has unrecognized color type '%d'.\n",*col);
		png_destroy_read_struct(&pngp,&infp,0);
		fclose(pf);
		return 0;
	}
	png_unknown_chunkp unk;
	int nunk = png_get_unknown_chunks(pngp,infp,&unk);
	for ( int i=0; i<nunk; i++ )
	{
		if ( !memcmp(unk[i].name,"alPh",4) )
		{
			fprintf(stderr," Images with alPh chunks are unsupported.\n");
			png_destroy_read_struct(&pngp,&infp,0);
			fclose(pf);
			return 0;
		}
		if ( memcmp(unk[i].name,"grAb",4) ) continue;
		if ( unk[i].size != 8 )
		{
			fprintf(stderr," grAb chunk has invalid size '%lu' ('8' expected).\n",unk[i].size);
			png_destroy_read_struct(&pngp,&infp,0);
			fclose(pf);
			return 0;
		}
		*x = be32toh(*(uint32_t*)unk[i].data);
		*y = be32toh(*(uint32_t*)(unk[i].data+4));
	}
	if ( *x || *y ) printf(" Read %ux%u%s image with offsets %+d,%+d.\n",*w,*h,fmtname(*bd,*col),*x,*y);
	else printf(" Read %ux%u%s image.\n",*w,*h,fmtname(*bd,*col));
	int rbytes = png_get_rowbytes(pngp,infp);
	*idata = calloc(rbytes,*h);
	png_bytepp rptr = png_get_rows(pngp,infp);
	for ( uint32_t i=0; i<(*h); i++ ) memcpy((*idata)+(rbytes*i),rptr[i],rbytes);
	png_destroy_read_struct(&pngp,&infp,0);
	fclose(pf);
	return 1;
}

int savepng( const char *fname, uint8_t *odata, uint32_t w, uint32_t h, int32_t x, int32_t y, int pxsize, png_color *plte, int nplt, uint8_t *trns, int ntrns, uint8_t bd, uint8_t col )
{
	if ( !fname ) return 0;
	png_structp pngp;
	png_infop infp;
	FILE *pf;
	if ( !(pf = fopen(fname,"wb")) )
	{
		fprintf(stderr," Cannot open for writing: %s.\n",strerror(errno));
		return 0;
	}
	pngp = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if ( !pngp )
	{
		fclose(pf);
		return 0;
	}
	infp = png_create_info_struct(pngp);
	if ( !infp )
	{
		fclose(pf);
		png_destroy_write_struct(&pngp,0);
		return 0;
	}
	if ( setjmp(png_jmpbuf(pngp)) )
	{
		png_destroy_write_struct(&pngp,&infp);
		fclose(pf);
		return 0;
	}
	png_init_io(pngp,pf);
	png_set_IHDR(pngp,infp,w,h,bd,col,PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,PNG_FILTER_TYPE_BASE);
	if ( col == PNG_COLOR_TYPE_PALETTE )
	{
		if ( trns ) png_set_tRNS(pngp,infp,trns,ntrns,0);
		png_set_PLTE(pngp,infp,plte,nplt);
	}
	if ( x || y )
	{
		uint32_t grabs[2];
		grabs[0] = htobe32(x);
		grabs[1] = htobe32(y);
		png_unknown_chunk grab =
		{
			.name = "grAb",
			.data = (uint8_t*)grabs,
			.size = 8,
			.location = PNG_HAVE_IHDR
		};
		png_set_unknown_chunks(pngp,infp,&grab,1);
	}
	png_set_compression_level(pngp,9);
	png_write_info(pngp,infp);
	for ( uint32_t i=0; i<h; i++ ) png_write_row(pngp,odata+(w*pxsize*i));
	png_write_end(pngp,infp);
	png_destroy_write_struct(&pngp,&infp);
	fclose(pf);
	return 1;
}

void processpic( const char *fname )
{
	printf("Processing '%s'...\n",fname);
	uint8_t *idata = 0;
	png_color *plte = 0;
	uint8_t *trns = 0;
	uint32_t w = 0, h = 0;
	int32_t x = 0, y = 0;
	int pxsize = 0;
	int nplt = 0;
	int ntrns = 0;
	uint8_t bd = 0, col = 0;
	if ( !loadpng(fname,&idata,&w,&h,&x,&y,&pxsize,&plte,&nplt,&trns,&ntrns,&bd,&col) )
		return;
	// crop spots
	uint32_t cropx = w, cropy = h, cropw = 0, croph = 0;
	for ( uint32_t cy=0; cy<h; cy++ ) for ( uint32_t cx=0; cx<w; cx++ )
	{
		switch ( col )
		{
		case PNG_COLOR_TYPE_PALETTE:
			{
				uint8_t i = idata[cy*w+cx];
				if ( (i < ntrns) && !trns[i] ) continue;
			}
			break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			if ( bd == 16 )
			{
				uint16_t a = ((uint16_t*)idata)[(cy*w+cx)*2+1];
				if ( !a ) continue;
			}
			else
			{
				uint8_t a = idata[(cy*w+cx)*2+1];
				if ( !a ) continue;
			}
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			if ( bd == 16 )
			{
				uint16_t a = ((uint16_t*)idata)[(cy*w+cx)*4+3];
				if ( !a ) continue;
			}
			else
			{
				uint8_t a = idata[(cy*w+cx)*4+3];
				if ( !a ) continue;
			}
			break;
		}
		if ( cx < cropx ) cropx = cx;
		if ( cy < cropy ) cropy = cy;
		if ( cx > cropw ) cropw = cx;
		if ( cy > croph ) croph = cy;
	}
	printf(" Crop region: (%u,%u) (%u,%u).\n",cropx,cropy,cropw,croph);
	cropw -= cropx-1;
	croph -= cropy-1;
	uint8_t *odata = calloc(cropw*croph,pxsize);
	uint32_t ow = cropw, oh = croph;
	int32_t ox = x-cropx, oy = y-cropy;
	for ( uint32_t cy=0; cy<croph; cy++ )
	{
		uint32_t ccy = cy+cropy;
		uint32_t ccx = cropx;
		memcpy(odata+cy*ow*pxsize,idata+(ccy*w+ccx)*pxsize,ow*pxsize);
	}
	char bname[256];
	snprintf(bname,256,"%s.bak",fname);
	rename(fname,bname);
	if ( ox || oy ) printf(" Save %ux%u%s image with offsets %+d,%+d.\n",ow,oh,fmtname(bd,col),ox,oy);
	else printf(" Save %ux%u%s image.\n",ow,oh,fmtname(bd,col));
	if ( !savepng(fname,odata,ow,oh,ox,oy,pxsize,plte,nplt,trns,ntrns,bd,col) )
		rename(bname,fname);
	else remove(bname);
	free(idata);
	if ( plte ) free(plte);
	if ( trns ) free(trns);
	free(odata);
}

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		fprintf(stderr,
"usage: cropng <file> ...\n"
"\n"
"cropng will trim excess transparency from PNG images while updating grAb\n"
"offsets to compensate (existing ones will be re-used as a base)\n"
"\n"
"files are updated in-place, but a backup will be kept in case errors happen.\n"
"\n"
"cropng supports paletted, color and grayscale PNGs at both 8bpc and 16bpc.\n"
"it cannot handle files with alPh chunks, nor bit depths other than 8 and 16.\n"
"\n"
"note: any extra chunks such as comments or exif data will be stripped.\n"
		);
		return 1;
	}
	for ( int i=1; i<argc; i++ ) processpic(argv[i]);
	return 0;
}
