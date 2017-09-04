#include <stdio.h>
#include <libgen.h>
#include <string.h>

int main( int argc, char **argv )
{
        if ( argc < 2 ) return 1;
        FILE *ckm = fopen(argv[1],"rb");
        char *ckmfil = basename(argv[1]);
        char bsafil[256];
        char espfil[256];
        strcpy(bsafil,ckmfil);
        strcpy(espfil,ckmfil);
        strcpy(strchr(bsafil,'.'),".bsa");
        strcpy(strchr(espfil,'.'),".esp");
        int siz;
        fread(&siz,1,4,ckm);
        FILE *bsa = fopen(bsafil,"wb");
        while ( (siz > 0) )
        {
                char buf[8192];
                int rd = fread(buf,1,(siz>8192)?8192:siz,ckm);
                fwrite(buf,1,rd,bsa);
                siz -= rd;
        }
        fclose(bsa);
        fread(&siz,1,4,ckm);
        FILE *esp = fopen(espfil,"wb");
        while ( !feof(ckm) )
        {
                char buf[8192];
                int rd = fread(buf,1,8192,ckm);
                fwrite(buf,1,rd,esp);
        }
        fclose(esp);
        return 0;
}
