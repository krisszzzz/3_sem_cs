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

static void 
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

    chunk->n_internal_points = n_internal_points;
}


static void
join_threads( pthread_t* tids,
              const int n_threads)
{
	for ( int i = 0; i < n_threads; i++ )
	{
		pthread_join( tids[i], NULL);
	}
}

static void*
wrapper_calc_chunk_internal_points( void *const args)
{
	assert( args != NULL);

	Chunk *const chunk = (Chunk*)args;

	calc_chunk_internal_points( chunk);
	
	pthread_exit( NULL);
}

static void
cancel_threads( pthread_t* tids,
                const int n_threads)
{
	for ( int i = 0; i < n_threads; i++ )
	{
		pthread_cancel( tids[i]);
	}
}


static int
calc_internal_points( Chunk* threads_chunk,
                      const int n_threads)
{
    int n_internal_points = 0;
    for ( int i = 0; i < n_threads; i++ )
    {
        n_internal_points += threads_chunk[i].n_internal_points;
    }

    return n_internal_points;
}

ErrEnum_t
servers_start( int n_threads)
{
    srand( time( NULL));
    
    // udp_sockfd for broadcast, tcp_sockfd for connection
    int udp_sockfd = socket( AF_INET, SOCK_DGRAM, 0);

    SYS_CHECKIT(
        udp_sockfd,

        return ESOCKCREATE;
    );
    
    SYS_CHECKIT(
        setsockopt( udp_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)),

        close( udp_sockfd);
        return ESOCKOPT;
    );

    sockaddr_in client_addr =
    {
        .sin_family = AF_INET,
        .sin_addr   = htonl( INADDR_ANY), // Take package from any ip addresses
        .sin_port   = htons( PORT),
        .sin_zero   = {0}
    };

    SYS_CHECKIT(
        bind( udp_sockfd, &client_addr, sizeof( client_addr)),

        return EBIND;
    );

    sockaddr_in recv_addr = { 0 };

    char skip_msg = 0;
    int addr_size = sizeof( client_addr);

    SYS_CHECKIT(
        recvfrom( udp_sockfd, &skip_msg, sizeof( skip_msg), 0, &recv_addr, &addr_size),

        return ERECVFROM;
    );

    // Get thread id of current thread and use this number as port,
    // because we cannot use listen on same port

    ServerInfo_t server_info =
    {
        .port = htons( getpid()),
        .n_cores = n_threads
    };

    client_addr.sin_port = server_info.port;
    
    SYS_CHECKIT(
        sendto( udp_sockfd, &server_info, sizeof( ServerInfo_t),
                0,           &recv_addr,  sizeof( recv_addr)),

        return ESENDTO;
    );

    // close connection and open tcp connection
    close( udp_sockfd);

    int tcp_sockfd = socket( AF_INET, SOCK_STREAM, 0);

    SYS_CHECKIT(
        tcp_sockfd,
        
        return ESOCKCREATE;
    );

    SYS_CHECKIT(
        setsockopt( tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)),

        close( tcp_sockfd);
        return ESOCKOPT;
    );



    SYS_CHECKIT(
        bind( tcp_sockfd, &client_addr, sizeof( client_addr)),

        close( tcp_sockfd);
        return EBIND;
    );

    SYS_CHECKIT(
        listen( tcp_sockfd, 1),
        
        close( tcp_sockfd);
        return ELISTEN;
    );

    int connfd = accept( tcp_sockfd, NULL, NULL);

    SYS_CHECKIT(
        connfd,

        close( tcp_sockfd);
        return EACCEPT;
    );

    Chunk client_chunk = { 0 };

    SYS_CHECKIT(
        read( connfd, &client_chunk, sizeof( Chunk)),

        close( tcp_sockfd);
        close( connfd);
        return EREAD;
    );

    pthread_t* threads = calloc( n_threads, sizeof(pthread_t));
    
    CHECKIT(
        threads == NULL,

        close( tcp_sockfd);
        close( connfd);
        return EALLOC;
    );

    Chunk* threads_chunk = calloc( n_threads, sizeof(Chunk));

    CHECKIT(
        threads_chunk == NULL,

        close( tcp_sockfd);
        close( connfd);
        free( threads);
        return EALLOC;
    );

    // divide number of points for threads
    const double x_range  = client_chunk.x_range / (double)n_threads;
    const int    n_points = client_chunk.n_points / n_threads;

    for ( int i = 0; i < n_threads; i++ )
    {
        threads_chunk[i] = client_chunk;
        threads_chunk[i].n_points = n_points;
        threads_chunk[i].x_range  = x_range;
        // move min for next thread
        threads_chunk[i].x_min   += x_range * i;

        SYS_CHECKIT(
            pthread_create( &threads[i], NULL, wrapper_calc_chunk_internal_points, &threads_chunk[i]),

            close( tcp_sockfd);
            close( connfd);
            cancel_threads( threads, n_threads);
            free( threads);
            free( threads_chunk);
            return EPTHREAD;
        );
    }

    join_threads( threads, n_threads);

    int internal_points = calc_internal_points( threads_chunk, n_threads);
    
    SYS_CHECKIT(
        write( connfd, &internal_points, sizeof( int)),

        close( connfd);
        close( tcp_sockfd);
        return EWRITE;
    );

    free( threads);
    free( threads_chunk);

    close( connfd);
    close( tcp_sockfd);

    return 0;
}
