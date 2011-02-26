#include <stdio.h>
#include <string.h>

#include "net-interfaces.h"
#include "utils.h"

#define VERBOSE 1
// #undef VERBOSE

#ifdef VERBOSE
  #define check( A ) \
    { \
        ++test; \
        if( A ){ \
            fprintf( stderr, "PASS test #%d (%s, %d)\n", test, __FILE__, __LINE__ ); \
        } else { \
            fprintf( stderr, "FAIL test #%d (%s, %d)\n", test, __FILE__, __LINE__ ); \
            return test; \
        } \
    }
#else
  #define check( A ) \
    { \
        ++test; \
        if( !( A ) ){ \
            fprintf( stderr, "FAIL test #%d (%s, %d)\n", test, __FILE__, __LINE__ ); \
            return test; \
        } \
    }
#endif

#define info( ... ) \
    do { \
        tr_msg( __FILE__, __LINE__, TR_MSG_INF, NULL, __VA_ARGS__ ); \
    } while( 0 )


static void tr_list_interfaces( tr_interface ** interfaces );
static void tr_list_interface( tr_interface * interface );

static void tr_list_interface( tr_interface * interface )
{
    char buf[INET6_ADDRSTRLEN];

	info("%s:",interface->name);
	info("  name = %s",interface->name);

	if (interface->af4)
	{
	    tr_ntop(&interface->ipv4, buf, sizeof(buf));
        info("  ipv4 = %s", buf);
	}
	if (interface->af6)
	{
	    tr_ntop(&interface->ipv6, buf, sizeof(buf));
        info("  ipv6 = %s", buf);
	}
	info(" ");
}

static void tr_list_interfaces( tr_interface ** interfaces )
{
    if (interfaces)
    {
        int index;
        for( index = 0; interfaces[index]; index++ )
        {
            tr_interface * interface = interfaces[index];
            tr_list_interface( interface );
        }
    }
	return;
}

static int
test1( void )
{
	tr_interface ** interfaces;

	info("Network interfaces test...");
	info(" ");
	interfaces = tr_net_interfaces();
	tr_list_interfaces(interfaces);
	tr_interfacesFree(interfaces);
	info("Done.");
	return 0;
}

int
main( void )
{
    int i;

    if( ( i = test1( ) ) )
        return i;

#ifdef VERBOSE
    fprintf( stderr, "net-interfaces-test passed\n" );
#endif
    return 0;
}
