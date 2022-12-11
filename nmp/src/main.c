#include <string.h>
#include <stdlib.h>

#include "checkit.h"
#include "nmp_common.h"

int main( int argc, char* argv[])
{
    if ( argc < 3 )
    {
        printf( "inccorect argument count");
        return -1;
    }
    
    if ( !strcmp( argv[1], "--client") )
    {
        double res = 0;
        integral_exp_x_2( &res,
                          atoi( argv[2]),
                          100000000,
                          0,
                          2);

        printf( "result = %lf\n", res);

    } else if ( !strcmp( argv[1], "--server") )
    {
        servers_start( atoi( argv[2]));
    } else
    {
        printf( "Unknown option %s\n", argv[1]);
    }
    
}
