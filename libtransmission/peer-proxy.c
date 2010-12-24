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

#include <event2/buffer.h>
#include <event2/buffer_compat.h>
#include <event2/event.h>

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
                 tr_port            peerPort UNUSED )
{
    tr_peerProxy *   proxy;
    tr_address       addr;
    tr_proxy_type    type;
    const char *     proxyIP;

    assert( session != NULL );

    proxyIP = tr_sessionGetPeerProxy( session );
    if( !tr_pton( proxyIP, &addr ) )
    {
        tr_nerr( "Proxy", "Invalid peer proxy address: %s", proxyIP );
        return NULL;
    }

    type = tr_sessionGetPeerProxyType( session );
    if( type == TR_PROXY_SOCKS4 && peerAddr->type != TR_AF_INET )
    {
        tr_nerr( "Proxy", "SOCKS4 Proxy does not support IPv6 peers" );
        return NULL;
    }

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
    assert( proxy != NULL );
    proxy->state = PEER_PROXY_INIT;
}

tr_bool
tr_peerProxyIsAuthEnabled( const tr_peerProxy * proxy )
{
    assert( proxy != NULL );
    return proxy->auth;
}

tr_proxy_type
tr_peerProxyGetType( const tr_peerProxy * proxy )
{
    assert( proxy != NULL );
    return proxy->type;
}

static inline void
tr_peerProxySetState( tr_peerProxy * proxy, PeerProxyState state )
{
    assert( proxy != NULL );
    proxy->state = state;
}


enum
{
    /* SOCKS 4 Protocol Constants */
    SOCKS4_VERSION = 4,

    SOCKS4_CMD_CONNECT = 1,

    SOCKS4_REQUEST_GRANTED = 90,
    SOCKS4_REQUEST_FAILED = 91,
    SOCKS4_REQUEST_REJECTED_IDENTD = 92,
    SOCKS4_REQUEST_REJECTED_USERID = 93,

    /* SOCKS 5 Protocol Constants */
    SOCKS5_VERSION = 5,

    SOCKS5_ADDR_IPV4 = 1,
    SOCKS5_ADDR_IPV6 = 4,

    SOCKS5_CMD_CONNECT = 1,

    SOCKS5_AUTH_NONE = 0,
    SOCKS5_AUTH_USERPASS = 2,
    SOCKS5_AUTH_INVALID = 255,

    SOCKS5_REPLY_SUCCESS = 0,
    SOCKS5_REPLY_GENERAL_FAILURE = 1,
    SOCKS5_REPLY_CONN_NOT_ALLOWED = 2,
    SOCKS5_REPLY_NETWORK_UNREACHABLE = 3,
    SOCKS5_REPLY_HOST_UNREACHABLE = 4,
    SOCKS5_REPLY_CONN_REFUSED = 5,
    SOCKS5_REPLY_TTL_EXPIRED = 6,
    SOCKS5_REPLY_CMD_NOT_SUPPORTED = 7,
    SOCKS5_REPLY_ADDR_NOT_SUPPORTED = 8,
};

static const char *
socksReplyStr( uint8_t code )
{
    switch( code )
    {
        case SOCKS4_REQUEST_GRANTED:           return "Granted";
        case SOCKS4_REQUEST_FAILED:            return "Failed";
        case SOCKS4_REQUEST_REJECTED_IDENTD:   return "Client IDENT server unreachable";
        case SOCKS4_REQUEST_REJECTED_USERID:   return "IDENT user-id mismatch";

        case SOCKS5_REPLY_SUCCESS:             return "Success";
        case SOCKS5_REPLY_GENERAL_FAILURE:     return "General failure";
        case SOCKS5_REPLY_CONN_NOT_ALLOWED:    return "Not allowed";
        case SOCKS5_REPLY_NETWORK_UNREACHABLE: return "Network unreachable";
        case SOCKS5_REPLY_HOST_UNREACHABLE:    return "Host unreachable";
        case SOCKS5_REPLY_CONN_REFUSED:        return "Connection refused";
        case SOCKS5_REPLY_TTL_EXPIRED:         return "TTL expired";
        case SOCKS5_REPLY_CMD_NOT_SUPPORTED:   return "Command not supported";
        case SOCKS5_REPLY_ADDR_NOT_SUPPORTED:  return "Address not supported";
        default: break;
    }
    return "(unknown)";
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

    tr_peerIoWriteBytes( io, buf, len, FALSE );
    tr_peerProxySetState( proxy, PEER_PROXY_CONNECT );
}

static void
writeProxyRequestSOCKS4( tr_peerIo * io )
{
    tr_peerProxy * proxy = tr_peerIoGetProxy( io );
    uint8_t version, command, null;
    tr_port port;
    const tr_address * addr;

    version = SOCKS4_VERSION;
    command = SOCKS4_CMD_CONNECT;
    addr = tr_peerIoGetAddress( io, &port );
    null = 0;

    assert( addr->type == TR_AF_INET );

    tr_peerIoWriteBytes( io, &version, 1, FALSE );
    tr_peerIoWriteBytes( io, &command, 1, FALSE );
    tr_peerIoWriteBytes( io, &port, 2, FALSE );
    tr_peerIoWriteBytes( io, &addr->addr.addr4.s_addr, 4, FALSE );

    if( tr_peerProxyIsAuthEnabled( proxy ) )
    {
        const char * username = tr_peerProxyGetUsername( proxy );
        size_t len = strlen( username );
        tr_peerIoWriteBytes( io, username, len, FALSE );
    }
    tr_peerIoWriteBytes( io, &null, 1, FALSE );
    tr_peerProxySetState( proxy, PEER_PROXY_CONNECT );
}

static void
writeProxyRequestSOCKS5( tr_peerIo * io )
{
    tr_peerProxy * proxy = tr_peerIoGetProxy( io );

    if( tr_peerProxyIsAuthEnabled( proxy ) )
    {
        uint8_t packet[4] = { SOCKS5_VERSION, 2, SOCKS5_AUTH_NONE, SOCKS5_AUTH_USERPASS };
        tr_peerIoWriteBytes( io, packet, sizeof( packet ), FALSE );
    }
    else
    {
        uint8_t packet[3] = { SOCKS5_VERSION, 1, SOCKS5_AUTH_NONE };
        tr_peerIoWriteBytes( io, packet, sizeof( packet ), FALSE );
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
    {
        /* Unlikely, but just in case. */
        tr_nerr( "Proxy", "HTTP peer proxy sent malformed response" );
        return READ_ERR;
    }
    success = ( strstr( line, " 200 " ) != NULL );
    if( !success )
        tr_nerr( "Proxy", "HTTP request rejected: %s", line );
    tr_free( line );
    evbuffer_drain( inbuf, EVBUFFER_LENGTH( inbuf ) );

    if( success )
    {
        tr_peerProxySetState( io->proxy, PEER_PROXY_ESTABLISHED );
        return READ_NOW;
    }

    return READ_ERR;
}

static int
readProxyResponseSOCKS4( tr_peerIo * io, struct evbuffer * inbuf )
{
    uint8_t reply;
    if( EVBUFFER_LENGTH( inbuf ) < 8 )
        return READ_LATER;
    reply = EVBUFFER_DATA( inbuf )[1];
    if( reply != SOCKS4_REQUEST_GRANTED )
    {
        tr_nerr( "Proxy", "SOCKS4 request rejected: %s", socksReplyStr( reply ) );
        return READ_ERR;
    }
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

    version = SOCKS5_VERSION;
    command = SOCKS5_CMD_CONNECT;
    reserved = 0;
    tr_peerIoWriteBytes( io, &version, 1, FALSE );
    tr_peerIoWriteBytes( io, &command, 1, FALSE );
    tr_peerIoWriteBytes( io, &reserved, 1, FALSE );

    if( addr->type == TR_AF_INET6 )
    {
        address_type = SOCKS5_ADDR_IPV6;
        tr_peerIoWriteBytes( io, &address_type, 1, FALSE );
        tr_peerIoWriteBytes( io, &addr->addr.addr6, 16, FALSE );
    }
    else
    {
        assert( addr->type == TR_AF_INET );
        address_type = SOCKS5_ADDR_IPV4;
        tr_peerIoWriteBytes( io, &address_type, 1, FALSE );
        tr_peerIoWriteBytes( io, &addr->addr.addr4.s_addr, 4, FALSE );
    }
    tr_peerIoWriteBytes( io, &port, 2, FALSE );

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

    if( method == SOCKS5_AUTH_INVALID )
    {
        tr_nerr( "Proxy", "SOCKS5 authentication method rejected" );
        return READ_ERR;
    }
    if( method == SOCKS5_AUTH_USERPASS && !tr_peerProxyIsAuthEnabled( proxy ) )
    {
        tr_nerr( "Proxy", "SOCKS5 authentication required" );
        return READ_ERR;
    }

    if( method == SOCKS5_AUTH_USERPASS )
    {
        uint8_t version, length;
        const char *username = tr_peerProxyGetUsername( proxy );
        const char *password = tr_peerProxyGetPassword( proxy );
        version = SOCKS5_VERSION;
        tr_peerIoWriteBytes( io, &version, 1, FALSE );
        length = MAX( strlen( username ), 255 );
        tr_peerIoWriteBytes( io, &length, 1, FALSE );
        tr_peerIoWriteBytes( io, username, length, FALSE );
        length = MAX( strlen( password ), 255 );
        tr_peerIoWriteBytes( io, &length, 1, FALSE );
        tr_peerIoWriteBytes( io, password, length, FALSE );

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

    if( status != SOCKS5_REPLY_SUCCESS )
    {
        tr_nerr( "Proxy", "SOCKS5 authentication failed" );
        return READ_ERR;
    }

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

    if( status != SOCKS5_REPLY_SUCCESS )
    {
        tr_nerr( "Proxy", "SOCKS5 request rejected: %s", socksReplyStr( status ) );
        return READ_ERR;
    }

    if( address_type == SOCKS5_ADDR_IPV4 )
        evbuffer_drain( inbuf, 4 + 2 );
    else if( address_type == SOCKS5_ADDR_IPV6 )
        evbuffer_drain( inbuf, 16 + 2 );
    else
    {
        /* Unlikely, but just in case. */
        tr_nerr( "Proxy", "SOCKS5 unsupported address type %d", address_type );
        return READ_ERR;
    }

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
 * the connection is now ready to be used for peer communication,
 * READ_LATER if more data is expected to be read from the proxy,
 * or READ_ERR if an error occured.
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
