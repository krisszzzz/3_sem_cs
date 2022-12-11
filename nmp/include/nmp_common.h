#ifndef MONTE_H
#define MONTE_H

#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>

#include "checkit.h"

#define LOCAL_BROADCAST "127.255.255.255"
#define PORT 9993

typedef double (*math_func)(const double x);
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;


typedef enum
{
    ENULL = -15,
    ELIMIT,
    EPTHREAD,
    ETIMEOUT,
    EALLOC,
    EPOLL,
    ERECVFROM,
    ESENDTO,
    ESOCKCREATE,
    ESOCKOPT,
    ECONNECT,
    EACCEPT,
    EWRITE,
    EBIND,
    ELISTEN
    
} ErrEnum_t;

typedef struct Chunk
{
	double x_min;
	double x_range;
	double y_max;
	
	int n_points;
} Chunk;

#define exp_x_2(x) exp((x)*(x))

int servers_start( int n_servers);

ErrEnum_t
get_server_list( sockaddr_in** server_list,
                 int max_n_server);

ErrEnum_t
integral_exp_x_2( double* result,
                  int n_servers,
                  int n_points,
                  const double x_min,
                  const double x_max);

#endif

