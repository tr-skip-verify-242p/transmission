/******************************************************************************
 * $Id$
 *
 * Copyright (c) Transmission authors and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#ifndef TR_PEER_PROXY_H
#define TR_PEER_PROXY_H

/**
***
**/

struct tr_peerIo;
struct evbuffer;

/**
 * @addtogroup networked_io Networked IO
 * @{
 */

typedef struct tr_peerProxy tr_peerProxy;

tr_peerProxy * tr_peerProxyNew( const tr_session * session,
                                const tr_address * peerAddr,
                                tr_port            peerPort);

void tr_peerProxyFree( tr_peerProxy * proxy );

const tr_address * tr_peerProxyGetAddress( const tr_peerProxy * proxy );

tr_port tr_peerProxyGetPort( const tr_peerProxy * proxy );

const char * tr_peerProxyGetUsername( const tr_peerProxy * proxy );

const char * tr_peerProxyGetPassword( const tr_peerProxy * proxy );

void tr_peerProxyResetConnectionState( tr_peerProxy * proxy );

tr_bool tr_peerProxyIsAuthEnabled( const tr_peerProxy * proxy );

tr_proxy_type tr_peerProxyGetType( const tr_peerProxy * proxy );


static inline tr_peerProxy * tr_peerIoGetProxy( const tr_peerIo * io )
{
    return io->proxy;
}

static inline tr_bool tr_peerIoIsProxied( const tr_peerIo * io )
{
    return io->proxy != NULL;
}

void   tr_peerIoWriteProxyRequest( tr_peerIo * io );

int    tr_peerIoReadProxyResponse( tr_peerIo * io,
                                   struct evbuffer * inbuf );

/* @} */

#endif
