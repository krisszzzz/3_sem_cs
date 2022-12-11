#ifndef CHECKIT_H
#define CHECKIT_H

#include <errno.h>
#include <stdio.h>

// checker for all types of statement and errors
#define CHECKIT( statement, ...)                        \
    if ( statement )                                    \
    {                                                   \
        printf( "%s\n"                                  \
                "%s:%d in %s\n",                        \
                #statement,                             \
              __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        __VA_ARGS__                                     \
    }


// checking linux system functions
#define SYS_CHECKIT( statement, ...) \
    CHECKIT( statement < 0, perror(#statement); __VA_ARGS__)

#define RE_CHECKIT( statement, ...)             \
    do                                          \
    {                                           \
       int tmp_err = 0;                         \
       CHECKIT( (tmp_err = statement),          \
                    __VA_ARGS__                 \
                    return tmp_err;);           \
    } while ( 0)

#endif

