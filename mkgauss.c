#include <stdio.h>
#include <math.h>

#define pi 3.14159265358979323846264338327950288419716939937510
#define e  2.71828182845904523536028747135266249775724709369995

int main( int argc, char **argv )
{
	if ( argc < 3 ) return 1;
	double sigma;
	unsigned range;
	sscanf(argv[1],"%u",&range);
	sscanf(argv[2],"%lf",&sigma);
	double kernel[range];
	unsigned i;
	for ( i=0; i<range; i++ )
	{
		kernel[i] = 1.0/sqrt(2*pi*sigma*sigma)
			*pow(e,-((i*i)/(2*sigma*sigma)));
	}
	double sum = kernel[0];
	for ( i=1; i<range; i++ ) sum += 2*kernel[i];
	printf("sum: %.50lf\n",sum);
	for ( i=0; i<range; i++ ) 
		printf("%.6lf%s",kernel[i]/sum,(i<range-1)?", ":"\n");
	return 0;
}
