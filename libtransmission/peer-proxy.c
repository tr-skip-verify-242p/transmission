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

#include <event.h>

#include "transmission.h"
#include "session.h"
#include "peer-io.h"
#include "peer-proxy.h"
#include "utils.h"


static void
writeProxyRequestHTTP( tr_peerIo * io )
{
}

static void
writeProxyRequestSOCKS4( tr_peerIo * io )
{
}

static void
writeProxyRequestSOCKS5( tr_peerIo * io )
{
}

void
tr_peerIoWriteProxyRequest( tr_peerIo * io )
{
    assert( io->isIncoming == FALSE );
    assert( io->isProxied == TRUE );
    assert( io->session != NULL );
    assert( io->encryptionMode == PEER_ENCRYPTION_NONE );

    switch( tr_sessionGetPeerProxyType( io->session ) )
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

/**
* @brief Reads and removes the proxy response from the buffer
* @return Returns READ_NOW if the proxy request succeeded and
* and the connection is now ready to be used for peer communication,
* READ_LATER if the buffer does not yet contain the complete
* response, or READ_ERR if an error occured.
**/
int    tr_peerIoReadProxyResponse( tr_peerIo * io,
                                   struct evbuffer * inbuf )
{
    return READ_ERR;
}
