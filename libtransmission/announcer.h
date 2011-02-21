/*
 * This file Copyright (C) Mnemosyne LLC
 *
 * This file is licensed by the GPL version 2. Works owned by the
 * Transmission project are granted a special exemption to clause 2(b)
 * so that the bulk of its code can remain under the MIT license.
 * This exemption does not extend to derived works not owned by
 * the Transmission project.
 *
 * $Id$
 */

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#ifndef _TR_ANNOUNCER_H_
#define _TR_ANNOUNCER_H_

#include "transmission.h"

struct tr_announcer;
struct tr_torrent_tiers;

/**
 * ***  Tracker Publish / Subscribe
 * **/

typedef enum
{
    TR_TRACKER_WARNING,
    TR_TRACKER_ERROR,
    TR_TRACKER_ERROR_CLEAR,
    TR_TRACKER_PEERS
}
TrackerEventType;

struct tr_pex;

/** @brief Notification object to tell listeners about announce or scrape occurences */
typedef struct
{
    /* what type of event this is */
    TrackerEventType    messageType;

    /* for TR_TRACKER_WARNING and TR_TRACKER_ERROR */
    const char * text;
    const char * tracker;

    /* for TR_TRACKER_PEERS */
    const struct tr_pex * pex;
    size_t pexCount;

    /* [0...100] for probability a peer is a seed. calculated by the leecher/seeder ratio */
    int8_t           seedProbability;
}
tr_tracker_event;

typedef void tr_tracker_callback ( tr_torrent              * tor,
                                   const tr_tracker_event  * event,
                                   void                    * client_data );

/**
***  Session ctor/dtor
**/

void tr_announcerInit( tr_session * );

void tr_announcerClose( tr_session * );

/**
***  For torrent customers
**/

struct tr_torrent_tiers * tr_announcerAddTorrent( tr_torrent          * torrent,
                                                  tr_tracker_callback * cb,
                                                  void                * cbdata );

tr_bool tr_announcerHasBacklog( const struct tr_announcer * );

void tr_announcerResetTorrent( struct tr_announcer*, tr_torrent* );

void tr_announcerRemoveTorrent( struct tr_announcer * ,
                                tr_torrent          * );

void tr_announcerChangeMyPort( tr_torrent * );

tr_bool tr_announcerCanManualAnnounce( const tr_torrent * );

void tr_announcerManualAnnounce( tr_torrent * );

void tr_announcerTorrentStarted( tr_torrent * );
void tr_announcerTorrentStopped( tr_torrent * );
void tr_announcerTorrentCompleted( tr_torrent * );

enum { TR_ANN_UP, TR_ANN_DOWN, TR_ANN_CORRUPT };
void tr_announcerAddBytes( tr_torrent *, int up_down_or_corrupt, uint32_t byteCount );

time_t tr_announcerNextManualAnnounce( const tr_torrent * );

tr_tracker_stat * tr_announcerStats( const tr_torrent * torrent,
                                     int              * setmeTrackerCount );

void tr_announcerStatsFree( tr_tracker_stat * trackers,
                            int               trackerCount );


struct tr_ptrArray;

/**
 * Add all verified trackers for the given torrent to the pointer
 * array @a fillmeTrackers, which is assumed to be empty.
 *
 * The added elements of the array will be newly allocated NULL-
 * terminated strings containing the normalized announce URLs and
 * will be sorted in ascending lexicographical order (as per BEP 28).
 *
 * @note The pointer array @a fillmeTrackers must be empty prior to
 *       the invocation of this function.
 *
 * @note You must destroy the pointer array and free the strings when
 *       they are no longer needed, e.g. by
 *       @code tr_ptrArrayDestruct( fillmeTrackers, tr_free ); @endcode
 *
 * @see tr_normalizeURL
 * @see tr_torrentGetTexHash
 */
void tr_announcerGetVerifiedTrackers( const tr_torrent   * torrent,
                                      struct tr_ptrArray * fillmeTrackers );

/**
 * Adds the trackers in @a trackers to the torrent's tiers.
 *
 * @note Only the @a tr_tracker_info.announce field is used,
 *       and it is assumed to be a NULL-terminated string.
 *
 * @see parseLtTex
 */
void tr_announcerAddTex( tr_torrent            * tor,
                         const tr_tracker_info * trackers,
                         int                     tracker_count );

#endif /* _TR_ANNOUNCER_H_ */
