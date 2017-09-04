#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define dword __UINT32_TYPE__
#define byte __UINT8_TYPE__

typedef struct
{
	char magic[4];
	dword size, flags, height, width, pitch, depth, mipmaps, reserved1[11],
	pf_size, pf_flags;
	char pf_fourcc[4];
	dword pf_bitcount, pf_rmask, pf_gmask, pf_bmask, pf_amask, caps[4],
		reserved2;
} __attribute__((packed)) ddsheader_t;

ddsheader_t head =
{
	.magic = "DDS ", .size = 124,
	.flags = 0x0000100F, /* caps|height|width|pitch|pixelformat */
	.height = 0, .width = 0, .pitch = 0, .depth = 0, /* set later */
	.mipmaps = 0, .reserved1 = {0,0,0,0,0,0,0,0,0,0,0}, .pf_size = 32,
	.pf_flags = 0x41 /* uncompressed|alpha */, .pf_fourcc = {0}, /* N/A */
	.pf_bitcount = 32, .pf_rmask = 0xff, .pf_gmask = 0xff00,
	.pf_bmask = 0xff0000, .pf_amask = 0xff000000, /* RGBA mask */
	.caps = {0x1000,0,0,0}, /* texture */ .reserved2 = 0
};

typedef struct { byte r,g,b,a; } __attribute__((packed)) pixel_t;

unsigned w = 0, h = 0;
pixel_t *tex = 0, *ttex = 0;
FILE *obj, *dds, *tmap;

typedef struct { float x,y,z; } vect_t;
typedef struct { float u,v; } uv_t;
typedef struct { int v,c,n; } vert_t;
typedef struct { vert_t a,b,c; } tri_t;
typedef struct { int x,y; } vec2_t;
typedef struct { pixel_t c[2]; vec2_t p[2]; } edge_t;

vect_t *verts = 0, *norms = 0, *tangents = 0, *binormals = 0;
uv_t *uvs = 0;
tri_t *tris = 0;
int nvert = 0, nnorm = 0, ncoord = 0, nface = 0;

void cross( vect_t *a, vect_t b, vect_t c )
{
	a->x = b.y*c.z-b.z*c.y;
	a->y = b.z*c.x-b.x*c.z;
	a->z = b.x*c.y-b.y*c.x;
}

void normalize( vect_t *a )
{
	float scale = sqrtf(powf(a->x,2.f)+powf(a->y,2.f)+powf(a->z,2.f));
	a->x /= scale;
	a->y /= scale;
	a->z /= scale;
}

void normalize2( uv_t *a )
{
	float scale = sqrtf(powf(a->u,2.f)+powf(a->v,2.f));
	a->u /= scale;
	a->v /= scale;
}

float dot( vect_t a, vect_t b )
{
	return a.x*b.x+a.y*b.y+a.z*b.z;
}

void tangent( vect_t *t, vect_t *b, int n )
{
	vect_t t1 = {0,0,0}, t2 = {0,0,0};
	for ( int j=0; j<nface; j++ )
	{
		if ( (tris[j].a.n != n) && (tris[j].b.n != n)
			&& (tris[j].c.n != n) ) continue;
	}
}

void calctangents( void )
{
	if ( tangents ) free(tangents);
	if ( binormals ) free(binormals);
	tangents = malloc(sizeof(vect_t)*nnorm);
	binormals = malloc(sizeof(vect_t)*nnorm);
	memset(tangents,0,sizeof(vect_t)*nnorm);
	memset(binormals,0,sizeof(vect_t)*nnorm);
	vect_t *t1, *t2;
	t1 = malloc(sizeof(vect_t)*nnorm);
	t2 = malloc(sizeof(vect_t)*nnorm);
	memset(t1,0,sizeof(vect_t)*nnorm);
	memset(t2,0,sizeof(vect_t)*nnorm);
	for ( int j=0; j<nface; j++ )
	{
		vect_t abv, acv;
		uv_t abc, acc;
		float r;
		vect_t ud, vd;
		abv.x = verts[tris[j].b.v].x-verts[tris[j].a.v].x;
		abv.y = verts[tris[j].b.v].y-verts[tris[j].a.v].y;
		abv.z = verts[tris[j].b.v].z-verts[tris[j].a.v].z;
		acv.x = verts[tris[j].c.v].x-verts[tris[j].a.v].x;
		acv.y = verts[tris[j].c.v].y-verts[tris[j].a.v].y;
		acv.z = verts[tris[j].c.v].z-verts[tris[j].a.v].z;
		abc.u = uvs[tris[j].b.c].u-uvs[tris[j].a.c].u;
		abc.v = uvs[tris[j].b.c].v-uvs[tris[j].a.c].v;
		acc.u = uvs[tris[j].c.c].u-uvs[tris[j].a.c].u;
		acc.v = uvs[tris[j].c.c].v-uvs[tris[j].a.c].v;
		r = 1.f/(abc.u*acc.v-acc.u*abc.v);
		ud.x = (acc.v*abv.x-abc.v-acv.x)*r;
		ud.y = (acc.v*abv.y-abc.v-acv.y)*r;
		ud.z = (acc.v*abv.z-abc.v-acv.z)*r;
		vd.x = (abc.u*acv.x-acc.u-abv.x)*r;
		vd.y = (abc.u*acv.y-acc.u-abv.y)*r;
		vd.z = (abc.u*acv.z-acc.u-abv.z)*r;
		t1[tris[j].a.n].x += ud.x;
		t1[tris[j].a.n].y += ud.y;
		t1[tris[j].a.n].z += ud.z;
		t1[tris[j].b.n].x += ud.x;
		t1[tris[j].b.n].y += ud.y;
		t1[tris[j].b.n].z += ud.z;
		t1[tris[j].c.n].x += ud.x;
		t1[tris[j].c.n].y += ud.y;
		t1[tris[j].c.n].z += ud.z;
		t2[tris[j].a.n].x += vd.x;
		t2[tris[j].a.n].y += vd.y;
		t2[tris[j].a.n].z += vd.z;
		t2[tris[j].b.n].x += vd.x;
		t2[tris[j].b.n].y += vd.y;
		t2[tris[j].b.n].z += vd.z;
		t2[tris[j].c.n].x += vd.x;
		t2[tris[j].c.n].y += vd.y;
		t2[tris[j].c.n].z += vd.z;
	}
	for ( int i=0; i<nnorm; i++ )
	{
		float dotnt = dot(norms[i],t1[i]);
		tangents[i].x = t1[i].x-norms[i].x*dotnt;
		tangents[i].y = t1[i].y-norms[i].y*dotnt;
		tangents[i].z = t1[i].z-norms[i].z*dotnt;
		normalize(&tangents[i]);
		cross(&binormals[i],norms[i],t1[i]);
		float hand = (dot(binormals[i],t2[i])<0.f)?-1.f:1.f;
		binormals[i].x *= hand;
		binormals[i].y *= hand;
		binormals[i].z *= hand;
	}
	free(t1);
	free(t2);
}

void calcnormals( void )
{
	if ( norms ) free(norms);
	norms = malloc(sizeof(vect_t)*nvert);
	memset(norms,0,sizeof(vect_t)*nvert);
	nnorm = nvert;
	for ( int i=0; i<nface; i++ )
	{
		tris[i].a.n = tris[i].a.v;
		tris[i].b.n = tris[i].b.v;
		tris[i].c.n = tris[i].c.v;
	}
	for ( int i=0; i<nvert; i++ )
	{
		for ( int j=0; j<nface; j++ )
		{
			if ( (tris[j].a.v != i) && (tris[j].b.v != i)
				&& (tris[j].c.v != i) ) continue;
			vect_t facet, ab, ac;
			ab.x = verts[tris[j].b.v].x-verts[tris[j].a.v].x;
			ab.y = verts[tris[j].b.v].y-verts[tris[j].a.v].y;
			ab.z = verts[tris[j].b.v].z-verts[tris[j].a.v].z;
			ac.x = verts[tris[j].c.v].x-verts[tris[j].a.v].x;
			ac.y = verts[tris[j].c.v].y-verts[tris[j].a.v].y;
			ac.z = verts[tris[j].c.v].z-verts[tris[j].a.v].z;
			cross(&facet,ab,ac);
			normalize(&facet);
			norms[i].x += facet.x;
			norms[i].y += facet.y;
			norms[i].z += facet.z;
		}
		normalize(&norms[i]);
	}
}

void loadobj( void )
{
	char rbuf[131072];
	char *txt = 0;
	size_t nread, len = 0, pos = 0;
	fseek(obj,0,SEEK_END);
	len = ftell(obj);
	fseek(obj,0,SEEK_SET);
	txt = malloc(len+1);
	do
	{
		nread = fread(rbuf,1,131072,obj);
		memcpy(txt+pos,rbuf,nread);
		pos += nread;
	}
	while ( !feof(obj) );
	txt[pos] = 0;
	char *line = txt;
	do
	{
		if ( !strncmp(line,"v ",2) ) nvert++;
		else if ( !strncmp(line,"vt ",3) ) ncoord++;
		else if ( !strncmp(line,"vn ",3) ) nnorm++;
		else if ( !strncmp(line,"f ",2) ) nface++;
		line = strchr(line,'\n');
	}
	while ( line && *(line++) );
	verts = malloc(sizeof(vect_t)*nvert);
	norms = malloc(sizeof(vect_t)*nnorm);
	uvs = malloc(sizeof(uv_t)*ncoord);
	tris = malloc(sizeof(tri_t)*nface);
	memset(verts,0,sizeof(vect_t)*nvert);
	memset(norms,0,sizeof(vect_t)*nnorm);
	memset(uvs,0,sizeof(uv_t)*ncoord);
	memset(tris,0,sizeof(tri_t)*nface);
	int cv = 0, cc = 0, cn = 0, cf = 0;
	line = strtok(txt,"\n");
	do
	{
		if ( !strncmp(line,"v ",2) )
		{
			sscanf(line,"v %f %f %f",&verts[cv].x,&verts[cv].y,
				&verts[cv].z);
			cv++;
		}
		else if ( !strncmp(line,"vt ",3) )
		{
			sscanf(line,"vt %f %f",&uvs[cc].u,&uvs[cc].v);
			uvs[cc].v = 1.0-uvs[cc].v;
			cc++;
		}
		else if ( !strncmp(line,"vn ",3) )
		{
			sscanf(line,"vn %f %f %f",&norms[cn].x,&norms[cn].y,
				&norms[cn].z);
			cn++;
		}
		else if ( !strncmp(line,"f ",2) )
		{
			if ( cn <= 0 )
			{
				sscanf(line,"f %d/%d %d/%d %d/%d",
					&tris[cf].a.v,&tris[cf].a.c,
					&tris[cf].b.v,&tris[cf].b.c,
					&tris[cf].c.v,&tris[cf].c.c);
			}
			else
			{
				sscanf(line,"f %d/%d/%d %d/%d/%d %d/%d/%d",
					&tris[cf].a.v,&tris[cf].a.c,
					&tris[cf].a.n,&tris[cf].b.v,
					&tris[cf].b.c,&tris[cf].b.n,
					&tris[cf].c.v,&tris[cf].c.c,
					&tris[cf].c.n);
			}
			tris[cf].a.v--;
			tris[cf].a.c--;
			tris[cf].a.n--;
			tris[cf].b.v--;
			tris[cf].b.c--;
			tris[cf].b.n--;
			tris[cf].c.v--;
			tris[cf].c.c--;
			tris[cf].c.n--;
			cf++;
		}
	}
	while ( (line = strtok(0,"\n")) );
	free(txt);
}

void mkedge( edge_t *e, pixel_t c1, vec2_t p1, pixel_t c2, vec2_t p2 )
{
	if ( p1.y < p2.y )
	{
		e->c[0] = c1;
		e->p[0] = p1;
		e->c[1] = c2;
		e->p[1] = p2;
	}
	else
	{
		e->c[0] = c2;
		e->p[0] = p2;
		e->c[1] = c1;
		e->p[1] = p1;
	}
}

void lerpcolor( pixel_t *o, pixel_t a, pixel_t b, float f )
{
	f = (f>0.f)?(f>1.f)?1.f:f:0.f;
	o->r = a.r*(1.f-f)+b.r*f;
	o->g = a.g*(1.f-f)+b.g*f;
	o->b = a.b*(1.f-f)+b.b*f;
	o->a = a.a*(1.f-f)+b.a*f;
}

void drawspans( edge_t a, edge_t b )
{
	float e1 = a.p[1].y-a.p[0].y;
	if ( e1 == 0.f ) return;
	float e2 = b.p[1].y-b.p[0].y;
	if ( e2 == 0.f ) return;
	float is1 = (float)(a.p[1].x-a.p[0].x);
	float is2 = (float)(b.p[1].x-b.p[0].x);
	float f1 = (float)(b.p[0].y-a.p[0].y)/e1;
	float fs1 = 1.f/e1;
	float f2 = 0.f;
	float fs2 = 1.f/e2;
	for ( int y=b.p[0].y; y<b.p[1].y; y++ )
	{
		pixel_t c1, c2, c3;
		lerpcolor(&c1,a.c[0],a.c[1],f1);
		lerpcolor(&c2,b.c[0],b.c[1],f2);
		int x1 = a.p[0].x+(int)(is1*f1);
		int x2 = b.p[0].x+(int)(is2*f2);
		if ( x1>x2 )
		{
			int tmp = x1;
			x1 = x2;
			x2 = tmp;
			pixel_t tmpc = c1;
			c1 = c2;
			c2 = tmpc;
		}
		int xd = x2-x1;
		if ( xd != 0 )
		{
			float fx = 0.f, fsx = 1.f/(float)xd;
			for ( int x=x1; x<x2; x++ )
			{
				lerpcolor(&c3,c1,c2,fx);
				if ( (x >= 0) && (x < w) && (y >= 0)
					&& (y < h) )
					if ( !tex[x+y*w].a ) tex[x+y*w] = c3;
				fx += fsx;
			}
		}
		f1 += fs1;
		f2 += fs2;
	}
}

void drawtriangle( int t1, int t2, int t3, int n1, int n2, int n3 )
{
	pixel_t col[3] =
	{
		{(norms[n1].x*0.5f+0.5f)*255,(norms[n1].z*0.5f+0.5f)*255,
			(norms[n1].y*0.5+0.5)*255,255},
		{(norms[n2].x*0.5f+0.5f)*255,(norms[n2].z*0.5f+0.5f)*255,
			(norms[n2].y*0.5+0.5)*255,255},
		{(norms[n3].x*0.5f+0.5f)*255,(norms[n3].z*0.5f+0.5f)*255,
			(norms[n3].y*0.5f+0.5f)*255,255},
	};
	vec2_t pts[3] =
	{
		{uvs[t1].u*w,uvs[t1].v*h},
		{uvs[t2].u*w,uvs[t2].v*h},
		{uvs[t3].u*w,uvs[t3].v*h}
	};
	edge_t edges[3];
	mkedge(&edges[0],col[0],pts[0],col[1],pts[1]);
	mkedge(&edges[1],col[1],pts[1],col[2],pts[2]);
	mkedge(&edges[2],col[2],pts[2],col[0],pts[0]);
	float ml = 0;
	int longest = 0, short1 = 0, short2 = 0;
	for ( int i=0; i<3; i++ )
	{
		float l = edges[i].p[1].y-edges[i].p[0].y;
		if ( l > ml )
		{
			ml = l;
			longest = i;
		}
	}
	short1 = (longest+1)%3;
	short2 = (longest+2)%3;
	drawspans(edges[longest],edges[short1]);
	drawspans(edges[longest],edges[short2]);
}

void plotnormals( void )
{
	int x1, y1, x2, y2, x3, y3;
	for ( int i=0; i<nface; i++ )
		drawtriangle(tris[i].a.c,tris[i].b.c,tris[i].c.c,
			tris[i].a.n,tris[i].b.n,tris[i].c.n);
}

int main( int argc, char **argv )
{
	if ( argc < 4 ) return 1;
	sscanf(argv[3],"%ux%u",&w,&h);
	head.width = w;
	head.height = h;
	head.pitch = w*4;
	/* load model */
	if ( !(obj = fopen(argv[1],"r")) ) return 2;
	loadobj();
	fclose(obj);
	/* plot pixels */
	tex = malloc(sizeof(pixel_t)*w*h);
	memset(tex,0,sizeof(pixel_t)*w*h);
	printf("calculate normals\n");
	calcnormals();
	printf("calculate tangents and binormals\n");
	calctangents();
	printf("plot normals\n");
	plotnormals();
	/* clean up */
	if ( !verts ) free(verts);
	if ( !norms ) free(norms);
	if ( !tangents ) free(tangents);
	if ( !binormals ) free(binormals);
	if ( !uvs ) free(uvs);
	if ( !tris ) free(tris);
	/* save texture */
	if ( !(dds = fopen(argv[2],"w")) ) { fclose(obj); return 4; }
	fwrite(&head,sizeof(ddsheader_t),1,dds);
	fwrite(tex,sizeof(pixel_t),w*h,dds);
	free(tex);
	fclose(dds);
	return 0;
}
