#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define frand() ((double)rand()/(double)(RAND_MAX))

int main( int argc, char **argv )
{
	double samp[3];
	unsigned i, max = 16, domult = 0, hemi = 0;
	if ( argc > 1 ) sscanf(argv[1],"%u",&max);
	if ( argc > 2 ) sscanf(argv[2],"%u",&domult);
	if ( argc > 3 ) sscanf(argv[3],"%u",&hemi);
	double sum, mult;
	srand(time(0));
	for ( i=0; i<max; i++ )
	{
		mult = domult?((double)(i+1)/(double)max):1.;
		if ( domult > 1 ) mult = pow(mult,domult);
		samp[0] = frand()*2.-1.;
		samp[1] = frand()*2.-1.;
		samp[2] = (hemi)?(frand()):(frand()*2.-1.);
		sum = sqrt(samp[0]*samp[0]+samp[1]*samp[1]+samp[2]*samp[2]);
		samp[0] = (samp[0]/sum)*mult;
		samp[1] = (samp[1]/sum)*mult;
		samp[2] = (samp[2]/sum)*mult;
		printf("float3(% .4lf,% .4lf,% .4lf)%s",samp[0],samp[1],
			samp[2],(i%2)?(i<max-1)?",\n":"\n":",");
	}
	return 0;
}
