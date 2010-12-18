/*
Copyright (c) 2010 by David Artoise Ijux

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <assert.h>
#include <string.h>

#include <event.h>

#include "transmission.h"
#include "session.h"
#include "peer-io.h"
#include "peer-proxy.h"
#include "utils.h"


typedef enum
{
    PEER_PROXY_INIT,
    PEER_PROXY_AUTH,
    PEER_PROXY_CONNECT,
    PEER_PROXY_ESTABLISHED
} PeerProxyState;

struct tr_peerProxy
{
    tr_address       address;
    tr_port          port;
    tr_proxy_type    type;
    tr_bool          auth;
    char *           username;
    char *           password;
    PeerProxyState   state;
};


tr_peerProxy *
tr_peerProxyNew( const tr_session * session,
                 const tr_address * peerAddr,
                 tr_port            peerPort)
{
    tr_peerProxy *   proxy;
    tr_address       addr;
    tr_proxy_type    type;

    assert( session != NULL );

    if( !tr_pton( tr_sessionGetPeerProxy( session ), &addr ) )
        return NULL;

    type = tr_sessionGetPeerProxyType( session );
    if( type == TR_PROXY_SOCKS4 && peerAddr->type != TR_AF_INET )
        return NULL;

    proxy = tr_new0( tr_peerProxy, 1 );
    proxy->address = addr;
    proxy->port = htons( tr_sessionGetPeerProxyPort( session ) );
    proxy->type = type;
    if( tr_sessionIsPeerProxyAuthEnabled( session ) )
    {
        proxy->auth = TRUE;
        proxy->username = tr_strdup( tr_sessionGetPeerProxyUsername( session ) );
        proxy->password = tr_strdup( tr_sessionGetPeerProxyPassword( session ) );
    }
    proxy->state = PEER_PROXY_INIT;

    return proxy;
}

void
tr_peerProxyFree( tr_peerProxy * proxy )
{
    if( proxy == NULL )
        return;
    if( proxy->username != NULL )
    {
        tr_free( proxy->username );
        proxy->username = NULL;
    }
    if( proxy->password != NULL )
    {
        tr_free( proxy->password );
        proxy->password = NULL;
    }
    tr_free( proxy );
}

const tr_address *
tr_peerProxyGetAddress( const tr_peerProxy * proxy )
{
    assert( proxy != NULL );
    return &proxy->address;
}

tr_port
tr_peerProxyGetPort( const tr_peerProxy * proxy )
{
    assert( proxy != NULL );
    return proxy->port;
}

const char *
tr_peerProxyGetUsername( const tr_peerProxy * proxy )
{
    assert( proxy != NULL );
    return proxy->username;
}

const char *
tr_peerProxyGetPassword( const tr_peerProxy * proxy )
{
    assert( proxy != NULL );
    return proxy->password;
}

void
tr_peerProxyResetConnectionState( tr_peerProxy * proxy )
{
    proxy->state = PEER_PROXY_INIT;
}

tr_bool
tr_peerProxyIsAuthEnabled( const tr_peerProxy * proxy )
{
    return proxy->auth;
}

tr_proxy_type
tr_peerProxyGetType( const tr_peerProxy * proxy )
{
    return proxy->type;
}

static inline void
tr_peerProxySetState( tr_peerProxy * proxy, PeerProxyState state )
{
    proxy->state = state;
}


static void
writeProxyRequestHTTP( tr_peerIo * io )
{
    static const int http_minor_version = 1;

    tr_peerProxy * proxy = tr_peerIoGetProxy( io );
    char buf[1024], hostHdr[256], authHdr[256], peerHost[64];
    const tr_address * peerAddr;
    tr_port peerPort;
    int len;

    if( http_minor_version > 0 )
    {
        char proxyHost[64];
        int proxyPort;
        tr_ntop( tr_peerProxyGetAddress( proxy ), proxyHost, sizeof( proxyHost ) );
        proxyPort = ntohs( tr_peerProxyGetPort( proxy ) );
        tr_snprintf( hostHdr, sizeof( hostHdr ), "Host: %s:%d\015\012",
                     proxyHost, proxyPort );
    }
    else
        hostHdr[0] = '\0';

    if( tr_peerProxyIsAuthEnabled( proxy ) )
    {
        char auth[128], *enc;
        tr_snprintf( auth, sizeof( auth ), "%s:%s",
                     tr_peerProxyGetUsername( proxy ),
                     tr_peerProxyGetPassword( proxy ) );
        enc = tr_base64_encode( auth, -1, NULL );
        tr_snprintf( authHdr, sizeof( authHdr ),
                     "Proxy-Authorization: Basic %s\015\012",
                     enc );
        tr_free( enc );
    }
    else
        authHdr[0] = '\0';

    peerAddr = tr_peerIoGetAddress( io, &peerPort );
    tr_ntop( peerAddr, peerHost, sizeof( peerHost ) );

    len = tr_snprintf( buf, sizeof( buf ),
                       "CONNECT %s:%d HTTP/1.%d\015\012%s%s\015\012",
                       peerHost, peerPort, http_minor_version,
                       hostHdr, authHdr );

    tr_peerIoWrite( io, buf, len, FALSE );
    tr_peerProxySetState( proxy, PEER_PROXY_CONNECT );
}

static void
writeProxyRequestSOCKS4( tr_peerIo * io )
{
    tr_peerProxy * proxy = tr_peerIoGetProxy( io );
    uint8_t version, command, null;
    tr_port port;
    const tr_address * addr;

    version = 4;
    command = 1;
    addr = tr_peerIoGetAddress( io, &port );
    null = 0;

    assert( addr->type == TR_AF_INET );

    tr_peerIoWrite( io, &version, 1, FALSE );
    tr_peerIoWrite( io, &command, 1, FALSE );
    tr_peerIoWrite( io, &port, 2, FALSE );
    tr_peerIoWrite( io, &addr->addr.addr4.s_addr, 4, FALSE );

    if( tr_peerProxyIsAuthEnabled( proxy ) )
    {
        const char * username = tr_peerProxyGetUsername( proxy );
        size_t len = strlen( username );
        tr_peerIoWrite( io, username, len, FALSE );
    }
    tr_peerIoWrite( io, &null, 1, FALSE );
    tr_peerProxySetState( proxy, PEER_PROXY_CONNECT );
}

static void
writeProxyRequestSOCKS5( tr_peerIo * io )
{
    tr_peerProxy * proxy = tr_peerIoGetProxy( io );

    if( tr_peerProxyIsAuthEnabled( proxy ) )
    {
        uint8_t packet[4] = { 5, 2, 0x00, 0x02 };
        tr_peerIoWrite( io, packet, sizeof(packet), FALSE );
    }
    else
    {
        uint8_t packet[3] = { 5, 1, 0x00 };
        tr_peerIoWrite( io, packet, sizeof(packet), FALSE );
    }

    tr_peerProxySetState( proxy, PEER_PROXY_INIT );
}

void
tr_peerIoWriteProxyRequest( tr_peerIo * io )
{
    tr_peerProxy * proxy = tr_peerIoGetProxy( io );

    assert( proxy != NULL );
    assert( io->isIncoming == FALSE );
    assert( io->encryptionMode == PEER_ENCRYPTION_NONE );

    switch( tr_peerProxyGetType( proxy ) )
    {
        case TR_PROXY_HTTP:
            writeProxyRequestHTTP( io );
            break;
        case TR_PROXY_SOCKS4:
            writeProxyRequestSOCKS4( io );
            break;
        case TR_PROXY_SOCKS5:
            writeProxyRequestSOCKS5( io );
            break;
    }
}

static int
readProxyResponseHTTP( tr_peerIo * io, struct evbuffer * inbuf )
{
    const void * data = EVBUFFER_DATA( inbuf );
    size_t datalen = EVBUFFER_LENGTH( inbuf );
    const char * eom = tr_memmem( data, datalen, "\015\012\015\012", 4 );
    char * line;
    tr_bool success;

    if( eom == NULL )
        return READ_LATER;

    line = evbuffer_readline( inbuf );
    if( line == NULL )
        return READ_ERR;
    success = ( strstr( line, " 200 " ) != NULL );
    tr_free( line );
    evbuffer_drain( inbuf, EVBUFFER_LENGTH( inbuf ) );

    if (success)
    {
        tr_peerProxySetState( io->proxy, PEER_PROXY_ESTABLISHED );
        return READ_NOW;
    }

    return READ_ERR;
}

static int
readProxyResponseSOCKS4( tr_peerIo * io, struct evbuffer * inbuf )
{
    if( EVBUFFER_LENGTH( inbuf ) < 8 )
        return READ_LATER;
    if( EVBUFFER_DATA( inbuf )[1] != 90 )
        return READ_ERR;
    evbuffer_drain( inbuf, 8 );
    tr_peerProxySetState( io->proxy, PEER_PROXY_ESTABLISHED );
    return READ_NOW;
}

static void
writeSOCKS5ConnectCommand( tr_peerIo * io )
{
    const tr_address * addr;
    tr_port port;
    uint8_t version, command, reserved, address_type;

    addr = tr_peerIoGetAddress( io, &port );

    version = 5;
    command = 1;
    reserved = 0;
    tr_peerIoWrite( io, &version, 1, FALSE );
    tr_peerIoWrite( io, &command, 1, FALSE );
    tr_peerIoWrite( io, &reserved, 1, FALSE );

    if( addr->type == TR_AF_INET6 )
    {
        address_type = 4;
        tr_peerIoWrite( io, &address_type, 1, FALSE );
        tr_peerIoWrite( io, &addr->addr.addr6, 16, FALSE );
    }
    else
    {
        assert( addr->type == TR_AF_INET );
        address_type = 1;
        tr_peerIoWrite( io, &address_type, 1, FALSE );
        tr_peerIoWrite( io, &addr->addr.addr4.s_addr, 4, FALSE );
    }
    tr_peerIoWrite( io, &port, 2, FALSE );

    tr_peerProxySetState( io->proxy, PEER_PROXY_CONNECT );
}

static int
processSOCKS5Greeting( tr_peerIo * io, struct evbuffer * inbuf )
{
    tr_peerProxy * proxy = tr_peerIoGetProxy( io );
    uint8_t method;

    if( EVBUFFER_LENGTH( inbuf ) < 2 )
        return READ_LATER;

    method = EVBUFFER_DATA( inbuf )[1];
    evbuffer_drain( inbuf, 2 );

    if( method != 0x00 && method != 0x02 )
        return READ_ERR;
    if( method == 0x02 && !tr_peerProxyIsAuthEnabled( proxy ) )
        return READ_ERR;

    if( method == 0x02 )
    {
        uint8_t version, length;
        const char *username = tr_peerProxyGetUsername( proxy );
        const char *password = tr_peerProxyGetPassword( proxy );
        version = 5;
        tr_peerIoWrite( io, &version, 1, FALSE );
        length = MAX( strlen( username ), 255 );
        tr_peerIoWrite( io, &length, 1, FALSE );
        tr_peerIoWrite( io, username, length, FALSE );
        length = MAX( strlen( password ), 255 );
        tr_peerIoWrite( io, &length, 1, FALSE );
        tr_peerIoWrite( io, password, length, FALSE );

        tr_peerProxySetState( proxy, PEER_PROXY_AUTH );
        return READ_LATER;
    }

    writeSOCKS5ConnectCommand( io );
    return READ_LATER;
}

static int
processSOCKS5AuthResponse( tr_peerIo * io, struct evbuffer * inbuf )
{
    uint8_t status;
    if( EVBUFFER_LENGTH( inbuf ) < 2 )
        return READ_LATER;

    status = EVBUFFER_DATA( inbuf )[1];
    evbuffer_drain( inbuf, 2 );

    if( status != 0 )
        return READ_ERR;

    writeSOCKS5ConnectCommand( io );
    return READ_LATER;
}

static int
processSOCKS5CmdResponse( tr_peerIo * io, struct evbuffer * inbuf )
{
    uint8_t status, address_type;

    if( EVBUFFER_LENGTH( inbuf ) < 4 )
        return READ_LATER;

    status = EVBUFFER_DATA( inbuf )[1];
    address_type = EVBUFFER_DATA( inbuf )[3];
    evbuffer_drain( inbuf, 4 );

    if( status != 0 )
        return READ_ERR;

    if( address_type == 1 )
        evbuffer_drain( inbuf, 4 + 2 );
    else if( address_type == 4 )
        evbuffer_drain( inbuf, 16 + 2 );
    else
        return READ_ERR;

    tr_peerProxySetState( io->proxy, PEER_PROXY_ESTABLISHED );
    return READ_NOW;
}

static int
readProxyResponseSOCKS5( tr_peerIo * io, struct evbuffer * inbuf )
{
    switch( io->proxy->state )
    {
        case PEER_PROXY_INIT:
            return processSOCKS5Greeting( io, inbuf );
        case PEER_PROXY_AUTH:
            return processSOCKS5AuthResponse( io, inbuf );
        case PEER_PROXY_CONNECT:
            return processSOCKS5CmdResponse( io, inbuf );
        default:
            break;
    }
    return READ_ERR;
}

/**
 * @brief Reads and removes the proxy response from the buffer
 * @return Returns READ_NOW if the proxy request succeeded and
 * and the connection is now ready to be used for peer communication,
 * READ_LATER if the buffer does not yet contain the complete
 * response, or READ_ERR if an error occured.
 * @note The proxy's complete response is removed from the buffer.
 */
int
tr_peerIoReadProxyResponse( tr_peerIo * io, struct evbuffer * inbuf )
{
    assert( io->proxy != NULL );
    assert( io->isIncoming == FALSE );
    assert( io->encryptionMode == PEER_ENCRYPTION_NONE );

    switch( tr_peerProxyGetType( io->proxy ) )
    {
        case TR_PROXY_HTTP:
            return readProxyResponseHTTP( io, inbuf );
        case TR_PROXY_SOCKS4:
            return readProxyResponseSOCKS4( io, inbuf );
        case TR_PROXY_SOCKS5:
            return readProxyResponseSOCKS5( io, inbuf );
    }

    return READ_ERR;
}
