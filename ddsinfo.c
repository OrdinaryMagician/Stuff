#include <stdio.h>
#include <errno.h>
#include <string.h>

#define dword __UINT32_TYPE__
#define byte __UINT8_TYPE__

typedef struct
{
	char magic[4];
	dword size, flags, height, width, pitch, depth, mipmaps;
	char reserved1[44];
	dword pf_size, pf_flags;
	char pf_fourcc[4];
	dword pf_bitcount, pf_rmask, pf_gmask, pf_bmask, pf_amask, caps[4],
		reserved2;
} __attribute__((packed)) ddsheader_t;

// flags
#define DDSD_CAPS        0x1
#define DDSD_HEIGHT      0x2
#define DDSD_WIDTH       0x4
#define DDSD_PITCH       0x8
#define DDSD_PIXELFORMAT 0x1000
#define DDSD_MIPMAPCOUNT 0x20000
#define DDSD_LINEARSIZE  0x80000
#define DDSD_DEPTH       0x800000

// caps[0]
#define DDSCAPS_COMPLEX 0x8
#define DDSCAPS_MIPMAP  0x400000
#define DDSCAPS_TEXTURE 0x1000

// caps[1]
#define DDSCAPS2_CUBEMAP           0x200
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x1000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x2000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x4000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x8000
#define DDSCAPS2_VOLUME            0x200000

// pf_flags
#define DDPF_ALPHAPIXELS 0x1
#define DDPF_ALPHA       0x2
#define DDPF_FOURCC      0x4
#define DDPF_RGB         0x40
#define DDPF_YUV         0x200
#define DDPF_LUMINANCE   0x20000

enum DXGI_FORMAT
{
	DXGI_FORMAT_UNKNOWN                    = 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS      = 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT         = 2,
	DXGI_FORMAT_R32G32B32A32_UINT          = 3,
	DXGI_FORMAT_R32G32B32A32_SINT          = 4,
	DXGI_FORMAT_R32G32B32_TYPELESS         = 5,
	DXGI_FORMAT_R32G32B32_FLOAT            = 6,
	DXGI_FORMAT_R32G32B32_UINT             = 7,
	DXGI_FORMAT_R32G32B32_SINT             = 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS      = 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT         = 10,
	DXGI_FORMAT_R16G16B16A16_UNORM         = 11,
	DXGI_FORMAT_R16G16B16A16_UINT          = 12,
	DXGI_FORMAT_R16G16B16A16_SNORM         = 13,
	DXGI_FORMAT_R16G16B16A16_SINT          = 14,
	DXGI_FORMAT_R32G32_TYPELESS            = 15,
	DXGI_FORMAT_R32G32_FLOAT               = 16,
	DXGI_FORMAT_R32G32_UINT                = 17,
	DXGI_FORMAT_R32G32_SINT                = 18,
	DXGI_FORMAT_R32G8X24_TYPELESS          = 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT       = 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS   = 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT    = 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS       = 23,
	DXGI_FORMAT_R10G10B10A2_UNORM          = 24,
	DXGI_FORMAT_R10G10B10A2_UINT           = 25,
	DXGI_FORMAT_R11G11B10_FLOAT            = 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS          = 27,
	DXGI_FORMAT_R8G8B8A8_UNORM             = 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB        = 29,
	DXGI_FORMAT_R8G8B8A8_UINT              = 30,
	DXGI_FORMAT_R8G8B8A8_SNORM             = 31,
	DXGI_FORMAT_R8G8B8A8_SINT              = 32,
	DXGI_FORMAT_R16G16_TYPELESS            = 33,
	DXGI_FORMAT_R16G16_FLOAT               = 34,
	DXGI_FORMAT_R16G16_UNORM               = 35,
	DXGI_FORMAT_R16G16_UINT                = 36,
	DXGI_FORMAT_R16G16_SNORM               = 37,
	DXGI_FORMAT_R16G16_SINT                = 38,
	DXGI_FORMAT_R32_TYPELESS               = 39,
	DXGI_FORMAT_D32_FLOAT                  = 40,
	DXGI_FORMAT_R32_FLOAT                  = 41,
	DXGI_FORMAT_R32_UINT                   = 42,
	DXGI_FORMAT_R32_SINT                   = 43,
	DXGI_FORMAT_R24G8_TYPELESS             = 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT          = 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS      = 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT       = 47,
	DXGI_FORMAT_R8G8_TYPELESS              = 48,
	DXGI_FORMAT_R8G8_UNORM                 = 49,
	DXGI_FORMAT_R8G8_UINT                  = 50,
	DXGI_FORMAT_R8G8_SNORM                 = 51,
	DXGI_FORMAT_R8G8_SINT                  = 52,
	DXGI_FORMAT_R16_TYPELESS               = 53,
	DXGI_FORMAT_R16_FLOAT                  = 54,
	DXGI_FORMAT_D16_UNORM                  = 55,
	DXGI_FORMAT_R16_UNORM                  = 56,
	DXGI_FORMAT_R16_UINT                   = 57,
	DXGI_FORMAT_R16_SNORM                  = 58,
	DXGI_FORMAT_R16_SINT                   = 59,
	DXGI_FORMAT_R8_TYPELESS                = 60,
	DXGI_FORMAT_R8_UNORM                   = 61,
	DXGI_FORMAT_R8_UINT                    = 62,
	DXGI_FORMAT_R8_SNORM                   = 63,
	DXGI_FORMAT_R8_SINT                    = 64,
	DXGI_FORMAT_A8_UNORM                   = 65,
	DXGI_FORMAT_R1_UNORM                   = 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP         = 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM            = 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM            = 69,
	DXGI_FORMAT_BC1_TYPELESS               = 70,
	DXGI_FORMAT_BC1_UNORM                  = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB             = 72,
	DXGI_FORMAT_BC2_TYPELESS               = 73,
	DXGI_FORMAT_BC2_UNORM                  = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB             = 75,
	DXGI_FORMAT_BC3_TYPELESS               = 76,
	DXGI_FORMAT_BC3_UNORM                  = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB             = 78,
	DXGI_FORMAT_BC4_TYPELESS               = 79,
	DXGI_FORMAT_BC4_UNORM                  = 80,
	DXGI_FORMAT_BC4_SNORM                  = 81,
	DXGI_FORMAT_BC5_TYPELESS               = 82,
	DXGI_FORMAT_BC5_UNORM                  = 83,
	DXGI_FORMAT_BC5_SNORM                  = 84,
	DXGI_FORMAT_B5G6R5_UNORM               = 85,
	DXGI_FORMAT_B5G5R5A1_UNORM             = 86,
	DXGI_FORMAT_B8G8R8A8_UNORM             = 87,
	DXGI_FORMAT_B8G8R8X8_UNORM             = 88,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
	DXGI_FORMAT_B8G8R8A8_TYPELESS          = 90,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB        = 91,
	DXGI_FORMAT_B8G8R8X8_TYPELESS          = 92,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB        = 93,
	DXGI_FORMAT_BC6H_TYPELESS              = 94,
	DXGI_FORMAT_BC6H_UF16                  = 95,
	DXGI_FORMAT_BC6H_SF16                  = 96,
	DXGI_FORMAT_BC7_TYPELESS               = 97,
	DXGI_FORMAT_BC7_UNORM                  = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB             = 99,
	DXGI_FORMAT_AYUV                       = 100,
	DXGI_FORMAT_Y410                       = 101,
	DXGI_FORMAT_Y416                       = 102,
	DXGI_FORMAT_NV12                       = 103,
	DXGI_FORMAT_P010                       = 104,
	DXGI_FORMAT_P016                       = 105,
	DXGI_FORMAT_420_OPAQUE                 = 106,
	DXGI_FORMAT_YUY2                       = 107,
	DXGI_FORMAT_Y210                       = 108,
	DXGI_FORMAT_Y216                       = 109,
	DXGI_FORMAT_NV11                       = 110,
	DXGI_FORMAT_AI44                       = 111,
	DXGI_FORMAT_IA44                       = 112,
	DXGI_FORMAT_P8                         = 113,
	DXGI_FORMAT_A8P8                       = 114,
	DXGI_FORMAT_B4G4R4A4_UNORM             = 115,
	DXGI_FORMAT_P208                       = 130,
	DXGI_FORMAT_V208                       = 131,
	DXGI_FORMAT_V408                       = 132,
	DXGI_FORMAT_FORCE_UINT                 = 0xffffffff
};

enum D3D10_RESOURCE_DIMENSION
{
	D3D10_RESOURCE_DIMENSION_UNKNOWN   = 0,
	D3D10_RESOURCE_DIMENSION_BUFFER    = 1,
	D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
	D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
	D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
};

typedef struct
{
	dword dxgiformat, resourcedimension, miscflag, arraysize, miscflags2;
} __attribute__((packed)) ddsheader_dx10_t;

// miscflag
#define DDS_RESOURCE_MISC_TEXTURECUBE 0x4

// miscflags2
#define DDS_ALPHA_MODE_UNKNOWN      0x0
#define DDS_ALPHA_MODE_STRAIGHT     0x1
#define DDS_ALPHA_MODE_PREMULTILIED 0x2
#define DDS_ALPHA_MODE_OPAQUE       0x3
#define DDS_ALPHA_MODE_CUSTOM       0x4

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		fprintf(stderr,"usage: %s <ddsfile>\n",argv[0]);
		return 1;
	}
	FILE *ddsf = fopen(argv[1],"r");
	if ( !ddsf )
	{
		fprintf(stderr,"%s: %s\n",argv[0],strerror(errno));
		return 2;
	}
	ddsheader_t head;
	ddsheader_dx10_t head10;
	fread(&head,sizeof(ddsheader_t),1,ddsf);
	if ( strncmp(head.magic,"DDS ",4) )
	{
		fprintf(stderr,"%s: %s not a dds file (bad magic)",argv[0],
			argv[1]);
		fclose(ddsf);
		return 4;
	}
	if ( !strncmp(head.pf_fourcc,"DX10",4) )
		fread(&head10,sizeof(ddsheader_dx10_t),1,ddsf);
	fclose(ddsf);
	/* print out header */
	printf("%.4s\n%u bytes\n",head.magic,head.size);
	printf("flags:");
	if ( head.flags&DDSD_CAPS ) printf(" CAPS");
	if ( head.flags&DDSD_HEIGHT ) printf(" HEIGHT");
	if ( head.flags&DDSD_WIDTH ) printf(" WIDTH");
	if ( head.flags&DDSD_PITCH ) printf(" PITCH");
	if ( head.flags&DDSD_PIXELFORMAT ) printf(" PIXELFORMAT");
	if ( head.flags&DDSD_MIPMAPCOUNT ) printf(" MIPMAPCOUNT");
	if ( head.flags&DDSD_LINEARSIZE ) printf(" LINEARSIZE");
	if ( head.flags&DDSD_DEPTH ) printf(" DEPTH");
	if ( head.flags&DDSD_DEPTH )
		printf("\n%u x %u x %u\n",head.width,head.height,head.depth);
	else printf("\n%u x %u\n",head.width,head.height);
	if ( head.flags&DDSD_PITCH ) printf("pitch: %u bytes\n",head.pitch);
	if ( head.flags&DDSD_MIPMAPCOUNT ) printf("%u mipmaps\n",head.mipmaps);
	if ( head.reserved1[0] ) printf("comment: %.44s\n",head.reserved1);
	printf("pixelformat: %u bytes\n",head.pf_size);
	printf("flags:");
	if ( head.pf_flags&DDPF_ALPHAPIXELS ) printf(" ALPHAPIXELS");
	if ( head.pf_flags&DDPF_ALPHA ) printf(" ALPHA");
	if ( head.pf_flags&DDPF_FOURCC ) printf(" FOURCC");
	if ( head.pf_flags&DDPF_RGB ) printf(" RGB");
	if ( head.pf_flags&DDPF_YUV ) printf(" YUV");
	if ( head.pf_flags&DDPF_LUMINANCE ) printf(" LUMINANCE");
	if ( head.pf_flags&DDPF_FOURCC )
		printf("\nfourcc: %.4s",head.pf_fourcc);
	printf("\n%u bits (mask: R %08x G %08x B %08x A %08x)\n",
		head.pf_bitcount,head.pf_rmask,head.pf_gmask,head.pf_bmask,
		head.pf_amask);
	printf("caps[0]:");
	if ( head.caps[0]&DDSCAPS_COMPLEX ) printf(" COMPLEX");
	if ( head.caps[0]&DDSCAPS_MIPMAP ) printf(" MIPMAP");
	if ( head.caps[0]&DDSCAPS_TEXTURE ) printf(" TEXTURE");
	printf("\ncaps[1]:");
	if ( head.caps[1]&DDSCAPS2_CUBEMAP ) printf(" CUBEMAP");
	if ( head.caps[1]&DDSCAPS2_CUBEMAP_POSITIVEX ) printf(" POSITIVEX");
	if ( head.caps[1]&DDSCAPS2_CUBEMAP_NEGATIVEX ) printf(" NEGATIVEX");
	if ( head.caps[1]&DDSCAPS2_CUBEMAP_POSITIVEY ) printf(" POSITIVEY");
	if ( head.caps[1]&DDSCAPS2_CUBEMAP_NEGATIVEY ) printf(" NEGATIVEY");
	if ( head.caps[1]&DDSCAPS2_CUBEMAP_POSITIVEZ ) printf(" POSITIVEZ");
	if ( head.caps[1]&DDSCAPS2_CUBEMAP_NEGATIVEZ ) printf(" NEGATIVEZ");
	if ( head.caps[1]&DDSCAPS2_VOLUME ) printf(" VOLUME");
	printf("\n");
	return 0;
}
