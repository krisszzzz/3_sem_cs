#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "checkit.h"
#include "nmp_common.h"


static void
init_thread_seed( unsigned short xsubi[3])
{
	xsubi[0] = rand();
	xsubi[1] = rand();
	xsubi[2] = rand();
}

static int 
calc_chunk_internal_points( Chunk *const chunk)
{ 
	assert( chunk != NULL);

	unsigned short xsubi[3] = { 0 }; // external storage for random function
	init_thread_seed( xsubi);

	const double x_min    = chunk->x_min;
	const double x_range  = chunk->x_range;
	const double y_max    = chunk->y_max;	
	const int n_points    = chunk->n_points;

	int n_internal_points = 0;	

	for ( int i = 0; i < n_points; i++ )
	{
		double x_rand = erand48( xsubi) * x_range + x_min;
		double y_rand = erand48( xsubi) * y_max;
		double y_func = exp_x_2( x_rand);

		if ( y_rand < y_func )
		{
			n_internal_points++;
		}
	}

	return n_internal_points;
}


static void
join_threads( const pthread_t tids[],
              const int n_threads)
{
	for ( int i = 0; i < n_threads; i++ )
	{
		pthread_join( tids[i], NULL);
	}
}

static void
handle_client( void* args)
{
    // udp_sockfds for broadcast, tcp_sockfds for connection
    int udp_sockfds = socket( AF_INET, SOCK_DGRAM, 0);
    int tcp_sockfds = socket( AF_INET, SOCK_STREAM, 0);

    SYS_CHECKIT(
        udp_sockfds,

        pthread_exit( NULL);
    );
    
    SYS_CHECKIT(
        tcp_sockfds,
        
        pthread_exit( NULL);
    );

    SYS_CHECKIT(
        setsockopt( udp_sockfds, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)),

        pthread_exit( NULL);
    );

    sockaddr_in client_addr =
    {
        .sin_family = AF_INET,
        .sin_addr   = htonl( INADDR_ANY), // Take package from any ip addresses
        .sin_port   = htons( PORT),
        .sin_zero   = {0}
    };

    SYS_CHECKIT(
        bind( udp_sockfds, &client_addr, sizeof( client_addr)),

        pthread_exit( NULL);
    );

    sockaddr_in recv_addr = { 0 };

    char skip_msg = 0;
    int addr_size = sizeof( client_addr);

    SYS_CHECKIT(
        recvfrom( udp_sockfds, &skip_msg, sizeof( skip_msg), 0, &recv_addr, &addr_size),

        pthread_exit( NULL);
    );


    // Get thread id of current thread and use this number as port,
    // because we cannot use listen on same port

    uint16_t port = htons( gettid()); 
    client_addr.sin_port = port;
    
    SYS_CHECKIT(
        sendto( udp_sockfds, &port, sizeof( uint16_t), 0, &recv_addr, sizeof( recv_addr)),

        pthread_exit( NULL);
    );

    // close connection and open tcp connection
    close( udp_sockfds);


    SYS_CHECKIT(
        setsockopt( tcp_sockfds, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)),

        pthread_exit( NULL);
    );



    SYS_CHECKIT(
        bind( tcp_sockfds, &client_addr, sizeof( client_addr)),

        pthread_exit( NULL);
    );

    SYS_CHECKIT(
        listen( tcp_sockfds, 1),

        pthread_exit( NULL);
    );

    int connfd = accept( tcp_sockfds, NULL, NULL);

    SYS_CHECKIT(
        connfd,

        pthread_exit( NULL);
    );



    Chunk chunk = { 0 };
    SYS_CHECKIT(
        read( connfd, &chunk, sizeof( Chunk)),

        pthread_exit( NULL);
    );
    
    int internal_points = calc_chunk_internal_points( &chunk);
    
    SYS_CHECKIT(
        write( connfd, &internal_points, sizeof( int)),

        pthread_exit( NULL);
    );

    close( connfd);
    close( tcp_sockfds);
}

ErrEnum_t
servers_start( int n_servers)
{
    srand( time( NULL));
    
    pthread_t* threads = calloc( n_servers, sizeof(pthread_t));
    
    CHECKIT(
        threads == NULL,

        return EALLOC;
    );

    for ( int i = 0; i < n_servers; i++ )
    {
        SYS_CHECKIT(
            pthread_create( &threads[i], NULL, handle_client, NULL),

            free( threads);
            return EPTHREAD;
        );
    }

    join_threads( threads, n_servers);

    free( threads);

    return 0;
}
