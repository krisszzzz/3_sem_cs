#include <assert.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>

#include "nmp_common.h"
#include "checkit.h"


ErrEnum_t
get_server_list( sockaddr_in** available_addrs,
                 int** n_cores,
                 int max_n_server)
{
    int client_socket = socket( AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in servers_addr =
    {
        .sin_family = AF_INET,
        .sin_addr   = inet_addr( LOCAL_BROADCAST),
        .sin_port   = htons( PORT),
        .sin_zero   = { 0 }
    };
    
    // client_socket is used for broadcast
    SYS_CHECKIT(
        setsockopt( client_socket, SOL_SOCKET, SO_BROADCAST, &(int) { 1 }, sizeof(int)),

        close( client_socket);
        return ESOCKOPT;
    );

    // send 1 byte to init udp servers
    char init_udp = 0;

    SYS_CHECKIT(
        sendto( client_socket, &init_udp, sizeof(init_udp), 0,
                (sockaddr*)&servers_addr, sizeof(servers_addr)),

        close( client_socket);
        return ESENDTO;
    );
        
    
    // get servers addresses
    sockaddr_in* tmp_addr_ptr = calloc( max_n_server, sizeof( sockaddr_in));
    
    CHECKIT(
        tmp_addr_ptr == NULL,

        return EALLOC;
    );

    int* tmp_n_cores = calloc( max_n_server, sizeof( sockaddr_in));

    CHECKIT(
       tmp_n_cores == NULL,
       
       free( tmp_addr_ptr);
       return EALLOC;
    );

    struct pollfd pollin = { 0 };
    pollin.events = POLLIN;
    pollin.fd = client_socket;
    
    for ( int n_server = 0; n_server < max_n_server; n_server++ )
    {
        // wait for servers response for 15 seconds
        int poll_ret = poll( &pollin, 1, 15000);

        // pol_ret < 0
        SYS_CHECKIT(
            poll_ret,

            close( client_socket);
            free( tmp_addr_ptr);
            free( tmp_n_cores);
            
            return EPOLL;
        );

        if ( poll_ret == 0 )
        {
            printf( "Servers timeout\n");

            close( client_socket);
            free( tmp_addr_ptr);
            free( tmp_n_cores);

            return ETIMEOUT;
        };
            
        pollin.revents = 0;

        sockaddr_in tmp_addr = { 0 };
        ServerInfo_t server_info = { 0 };

        SYS_CHECKIT(
            recvfrom( client_socket, &server_info, sizeof( server_info), 0,
                      &tmp_addr, &(int) { sizeof(sockaddr_in) }),

            close( client_socket);
            free( tmp_addr_ptr);

            return ERECVFROM;
        );

        tmp_n_cores[n_server] = server_info.n_cores;

        // get server port through socket 
        tmp_addr.sin_port = server_info.port;
        tmp_addr_ptr[n_server] = tmp_addr;
    }

    *available_addrs = tmp_addr_ptr;
    *n_cores = tmp_n_cores;
   
    close( client_socket);
    
    return 0;
}

static ErrEnum_t
init_sockets( int** socketfds,
              int n_servers)
{
    int* tmp_fds = calloc( n_servers, sizeof( int));
    
    CHECKIT(
       tmp_fds == NULL,

       return EALLOC;
    );
    
    for ( int i = 0; i < n_servers; i++ )
    {
        tmp_fds[i] = socket( AF_INET, SOCK_STREAM, 0);

        SYS_CHECKIT(
            tmp_fds[i],

            return ESOCKCREATE;
        );
    }

    *socketfds = tmp_fds;

    return 0;
}

static ErrEnum_t
connect_servers( sockaddr_in* server_list,
                 int* sockfds,
                 const int n_servers)
{
    for ( int i = 0; i < n_servers; i++ )
    {
        SYS_CHECKIT(
            connect( sockfds[i], &server_list[i], sizeof( server_list[i])),

            return ECONNECT;
        );
    }

    return 0;
}


static int
calc_internal_points( int* sockfds,
                      const int n_servers)
{
	int n_internal_points = 0;

	for ( int i = 0; i < n_servers; i++ )
	{
        int chunk_internal_points = 0;
		read( sockfds[i], &chunk_internal_points, sizeof( int));
        n_internal_points += chunk_internal_points;
	}

	return n_internal_points;
}


static void 
close_all( int* sockfds,
           const int n_servers)
{
    for ( int i = 0; i < n_servers; i++ )
    {
        close( sockfds[i]);
    }
}

// get all avaialble server cores count
static int
get_all_cores( int* n_cores,
               int n_servers)
{
    int all_cores = 0;

    for ( int i = 0; i < n_servers; i++ )
    {
        all_cores += n_cores[i];
    }

    return all_cores;
}

ErrEnum_t
integral_exp_x_2( double* result,
                  int n_servers,
                  int n_points,
                  const double x_min,
                  const double x_max)
{
    CHECKIT(
        result == NULL,

        return ENULL;
    );
    
    // Currently we use Monte-Carlo method only for monitonically growing
    // function. exp( x^2 ) grow from x => [0, +INF]
    CHECKIT(
        x_min < 0,

        return ELIMIT;
    );
    
    int* sockfds = NULL;
    RE_CHECKIT( init_sockets( &sockfds, n_servers));

	Chunk* chunks = calloc( n_servers, sizeof(Chunk));

    CHECKIT(
       chunks == NULL,

       free( sockfds);
       return EALLOC;
    );

    sockaddr_in* server_list = NULL;
    int* n_cores = NULL;
    
    RE_CHECKIT( get_server_list( &server_list, &n_cores, n_servers),
                close_all( sockfds, n_servers);
                free( chunks);
                free( sockfds); );

    RE_CHECKIT( connect_servers( server_list, sockfds, n_servers),
                close_all( sockfds, n_servers);
                free( chunks);
                free( sockfds); );

	const double   x_range     = (x_max - x_min);
	const double   y_max       = exp_x_2( x_max);
    const int      n_all_cores = get_all_cores( n_cores, n_servers);
    
    double   x_chunk_min    = x_min;

    for ( int i = 0; i < n_servers; i++ )
    {
        // divide range and number of points dependeping of how much cores avaialble
        // on server
        double x_chunk_range  = n_cores[i] * x_range / (double)n_all_cores;

		Chunk* const chunk = chunks + i;

		chunk->x_min             = x_chunk_min;
		chunk->x_range           = x_chunk_range;	
		chunk->y_max             = y_max;
		chunk->n_points          = ( n_cores[i] * n_points) / n_all_cores;

       SYS_CHECKIT(
          write( sockfds[i], chunk, sizeof( Chunk)),

          close_all( sockfds, n_servers);
          free( chunks);
          free( server_list);
          free( sockfds);
          return EWRITE;
       );

       // move min for next server
       x_chunk_min += x_chunk_range;
    }

    int n_internal_points = calc_internal_points( sockfds, n_servers);

	const double square   = x_range * y_max;
    const double coverage = (double)n_internal_points / n_points;

    *result = coverage * square;

    free( chunks);
    free( server_list);
    free( sockfds);
    free( n_cores);
    close_all( sockfds, n_servers);

    return 0;
}
