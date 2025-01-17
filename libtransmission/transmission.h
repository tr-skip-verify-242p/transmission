/******************************************************************************
 * $Id$
 *
 * Copyright (c) 2005-2008 Transmission authors and contributors
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

/*
 * This file defines the public API for the libtransmission library.
 *
 * Other headers with a public API are bencode.h and utils.h.
 * Most of the remaining headers in libtransmission are private.
 */
#ifndef TR_TRANSMISSION_H
#define TR_TRANSMISSION_H 1

#ifdef __cplusplus
extern "C" {
#endif

/***
****
****  Basic Types
****
***/

#include <inttypes.h> /* uintN_t */
#include <time.h> /* time_t */

#ifndef PRId64
 #define PRId64 "lld"
#endif
#ifndef PRIu64
 #define PRIu64 "llu"
#endif
#ifndef PRIu32
 #define PRIu32 "lu"
#endif

#if defined(WIN32) && defined(_MSC_VER)
 #define inline __inline
#endif

#define SHA_DIGEST_LENGTH 20
#define TR_INET6_ADDRSTRLEN 46

typedef uint32_t tr_file_index_t;
typedef uint32_t tr_piece_index_t;
/* assuming a 16 KiB block, a 32-bit block index gives us a maximum torrent size of 63 TiB.
 * if we ever need to grow past that, change this to uint64_t ;) */
typedef uint32_t tr_block_index_t;
typedef uint16_t tr_port;
typedef uint8_t tr_bool;

typedef struct tr_ctor tr_ctor;
typedef struct tr_info tr_info;
typedef struct tr_torrent tr_torrent;
typedef struct tr_session tr_session;

struct tr_benc;

typedef int8_t tr_priority_t;

#define TR_RPC_SESSION_ID_HEADER "X-Transmission-Session-Id"

typedef enum
{
    TR_PREALLOCATE_NONE   = 0,
    TR_PREALLOCATE_SPARSE = 1,
    TR_PREALLOCATE_FULL   = 2
}
tr_preallocation_mode;

typedef enum
{
    TR_PROXY_HTTP,
    TR_PROXY_SOCKS4,
    TR_PROXY_SOCKS5
}
tr_proxy_type;

typedef enum
{
    TR_CLEAR_PREFERRED,
    TR_ENCRYPTION_PREFERRED,
    TR_ENCRYPTION_REQUIRED
}
tr_encryption_mode;


/***
****
****  Startup & Shutdown
****
***/

/**
 * @addtogroup tr_session Session
 *
 * A libtransmission session is created by calling tr_sessionInit().
 * libtransmission creates a thread for itself so that it can operate
 * independently of the caller's event loop. The session will continue
 * until tr_sessionClose() is called.
 *
 * @{
 */

/**
 * @brief returns Transmission's default configuration file directory.
 *
 * The default configuration directory is determined this way:
 * -# If the TRANSMISSION_HOME environment variable is set, its value is used.
 * -# On Darwin, "${HOME}/Library/Application Support/${appname}" is used.
 * -# On Windows, "${CSIDL_APPDATA}/${appname}" is used.
 * -# If XDG_CONFIG_HOME is set, "${XDG_CONFIG_HOME}/${appname}" is used.
 * -# ${HOME}/.config/${appname}" is used as a last resort.
 */
const char* tr_getDefaultConfigDir( const char * appname );

/**
 * @brief returns Transmisson's default download directory.
 *
 * The default download directory is determined this way:
 * -# If the HOME environment variable is set, "${HOME}/Downloads" is used.
 * -# On Windows, "${CSIDL_MYDOCUMENTS}/Downloads" is used.
 * -# Otherwise, getpwuid(getuid())->pw_dir + "/Downloads" is used.
 */
const char* tr_getDefaultDownloadDir( void );


#define TR_DEFAULT_BIND_ADDRESS_IPV4        "0.0.0.0"
#define TR_DEFAULT_BIND_ADDRESS_IPV6             "::"
#define TR_DEFAULT_OPEN_FILE_LIMIT_STR           "32"
#define TR_DEFAULT_RPC_WHITELIST          "127.0.0.1"
#define TR_DEFAULT_RPC_PORT_STR                "9091"
#define TR_DEFAULT_RPC_URL_STR       "/transmission/"
#define TR_DEFAULT_PEER_PORT_STR              "51413"
#define TR_DEFAULT_PEER_SOCKET_TOS_STR            "0"
#define TR_DEFAULT_PEER_LIMIT_GLOBAL_STR        "240"
#define TR_DEFAULT_PEER_LIMIT_TORRENT_STR        "60"

#define TR_PREFS_KEY_ALT_SPEED_ENABLED             "alt-speed-enabled"
#define TR_PREFS_KEY_ALT_SPEED_UP_KBps             "alt-speed-up"
#define TR_PREFS_KEY_ALT_SPEED_DOWN_KBps           "alt-speed-down"
#define TR_PREFS_KEY_ALT_SPEED_TIME_BEGIN          "alt-speed-time-begin"
#define TR_PREFS_KEY_ALT_SPEED_TIME_ENABLED        "alt-speed-time-enabled"
#define TR_PREFS_KEY_ALT_SPEED_TIME_END            "alt-speed-time-end"
#define TR_PREFS_KEY_ALT_SPEED_TIME_DAY            "alt-speed-time-day"
#define TR_PREFS_KEY_BIND_ADDRESS_IPV4             "bind-address-ipv4"
#define TR_PREFS_KEY_BIND_ADDRESS_IPV6             "bind-address-ipv6"
#define TR_PREFS_KEY_BLOCKLIST_ENABLED             "blocklist-enabled"
#define TR_PREFS_KEY_BLOCKLIST_URL                 "blocklist-url"
#define TR_PREFS_KEY_MAX_CACHE_SIZE_MB             "cache-size-mb"
#define TR_PREFS_KEY_DHT_ENABLED                   "dht-enabled"
#define TR_PREFS_KEY_LPD_ENABLED                   "lpd-enabled"
#define TR_PREFS_KEY_DOWNLOAD_DIR                  "download-dir"
#define TR_PREFS_KEY_ENCRYPTION                    "encryption"
#define TR_PREFS_KEY_IDLE_LIMIT                    "idle-seeding-limit"
#define TR_PREFS_KEY_IDLE_LIMIT_ENABLED            "idle-seeding-limit-enabled"
#define TR_PREFS_KEY_INCOMPLETE_DIR                "incomplete-dir"
#define TR_PREFS_KEY_INCOMPLETE_DIR_ENABLED        "incomplete-dir-enabled"
#define TR_PREFS_KEY_LAZY_BITFIELD                 "lazy-bitfield-enabled"
#define TR_PREFS_KEY_MSGLEVEL                      "message-level"
#define TR_PREFS_KEY_OPEN_FILE_LIMIT               "open-file-limit"
#define TR_PREFS_KEY_PEER_LIMIT_GLOBAL             "peer-limit-global"
#define TR_PREFS_KEY_PEER_LIMIT_TORRENT            "peer-limit-per-torrent"
#define TR_PREFS_KEY_PEER_PORT                     "peer-port"
#define TR_PREFS_KEY_PEER_PORT_RANDOM_ON_START     "peer-port-random-on-start"
#define TR_PREFS_KEY_PEER_PORT_RANDOM_LOW          "peer-port-random-low"
#define TR_PREFS_KEY_PEER_PORT_RANDOM_HIGH         "peer-port-random-high"
#define TR_PREFS_KEY_PEER_SOCKET_TOS               "peer-socket-tos"
#define TR_PREFS_KEY_PEER_CONGESTION_ALGORITHM     "peer-congestion-algorithm"
#define TR_PREFS_KEY_PEX_ENABLED                   "pex-enabled"
#define TR_PREFS_KEY_PORT_FORWARDING               "port-forwarding-enabled"
#define TR_PREFS_KEY_PROXY_AUTH_ENABLED            "proxy-auth-enabled"
#define TR_PREFS_KEY_PREALLOCATION                 "preallocation"
#define TR_PREFS_KEY_PROXY_ENABLED                 "proxy-enabled"
#define TR_PREFS_KEY_PROXY_PASSWORD                "proxy-auth-password"
#define TR_PREFS_KEY_PROXY_PORT                    "proxy-port"
#define TR_PREFS_KEY_PROXY                         "proxy"
#define TR_PREFS_KEY_PROXY_TYPE                    "proxy-type"
#define TR_PREFS_KEY_PROXY_USERNAME                "proxy-auth-username"
#define TR_PREFS_KEY_RATIO                         "ratio-limit"
#define TR_PREFS_KEY_RATIO_ENABLED                 "ratio-limit-enabled"
#define TR_PREFS_KEY_RENAME_PARTIAL_FILES          "rename-partial-files"
#define TR_PREFS_KEY_RPC_AUTH_REQUIRED             "rpc-authentication-required"
#define TR_PREFS_KEY_RPC_BIND_ADDRESS              "rpc-bind-address"
#define TR_PREFS_KEY_RPC_ENABLED                   "rpc-enabled"
#define TR_PREFS_KEY_RPC_PASSWORD                  "rpc-password"
#define TR_PREFS_KEY_RPC_PORT                      "rpc-port"
#define TR_PREFS_KEY_RPC_USERNAME                  "rpc-username"
#define TR_PREFS_KEY_RPC_URL                       "rpc-url"
#define TR_PREFS_KEY_RPC_WHITELIST_ENABLED         "rpc-whitelist-enabled"
#define TR_PREFS_KEY_SCRIPT_TORRENT_DONE_FILENAME  "script-torrent-done-filename"
#define TR_PREFS_KEY_SCRIPT_TORRENT_DONE_ENABLED   "script-torrent-done-enabled"
#define TR_PREFS_KEY_RPC_WHITELIST                 "rpc-whitelist"
#define TR_PREFS_KEY_DSPEED_KBps                   "speed-limit-down"
#define TR_PREFS_KEY_DSPEED_ENABLED                "speed-limit-down-enabled"
#define TR_PREFS_KEY_USPEED_KBps                   "speed-limit-up"
#define TR_PREFS_KEY_USPEED_ENABLED                "speed-limit-up-enabled"
#define TR_PREFS_KEY_UMASK                         "umask"
#define TR_PREFS_KEY_UPLOAD_SLOTS_PER_TORRENT      "upload-slots-per-torrent"
#define TR_PREFS_KEY_START                         "start-added-torrents"
#define TR_PREFS_KEY_TRASH_ORIGINAL                "trash-original-torrent-files"


/**
 * Add libtransmission's default settings to the benc dictionary.
 *
 * Example:
 * @code
 *     tr_benc settings;
 *     int64_t i;
 *
 *     tr_bencInitDict( &settings, 0 );
 *     tr_sessionGetDefaultSettings( &settings );
 *     if( tr_bencDictFindInt( &settings, TR_PREFS_KEY_PEER_PORT, &i ) )
 *         fprintf( stderr, "the default peer port is %d\n", (int)i );
 *     tr_bencFree( &settings );
 * @endcode
 *
 * @param initme pointer to a tr_benc dictionary
 * @see tr_sessionLoadSettings()
 * @see tr_sessionInit()
 * @see tr_getDefaultConfigDir()
 */
void tr_sessionGetDefaultSettings( const char * configDir, struct tr_benc * dictionary );

/**
 * Add the session's current configuration settings to the benc dictionary.
 *
 * FIXME: this probably belongs in libtransmissionapp
 *
 * @param session
 * @param dictionary
 * @see tr_sessionGetDefaultSettings()
 */
void tr_sessionGetSettings( tr_session *, struct tr_benc * dictionary );

/**
 * Load settings from the configuration directory's settings.json file,
 * using libtransmission's default settings as fallbacks for missing keys.
 *
 * FIXME: this belongs in libtransmissionapp
 *
 * @param dictionary pointer to an uninitialized tr_benc
 * @param configDir the configuration directory to find settings.json
 * @param appName if configDir is empty, appName is used to find the default dir.
 * @return success TRUE if the settings were loaded, FALSE otherwise
 * @see tr_sessionGetDefaultSettings()
 * @see tr_sessionInit()
 * @see tr_sessionSaveSettings()
 */
tr_bool tr_sessionLoadSettings( struct tr_benc  * dictionary,
                                const char      * configDir,
                                const char      * appName );

/**
 * Add the session's configuration settings to the benc dictionary
 * and save it to the configuration directory's settings.json file.
 *
 * FIXME: this belongs in libtransmissionapp
 *
 * @param session
 * @param dictionary
 * @see tr_sessionLoadSettings()
 */
void tr_sessionSaveSettings( tr_session           * session,
                             const char           * configDir,
                             const struct tr_benc * dictonary );

/**
 * @brief Initialize a libtransmission session.
 *
 * For example, this will instantiate a session with all the default values:
 * @code
 *     tr_benc settings;
 *     tr_session * session;
 *     const char * configDir;
 *
 *     tr_bencInitDict( &settings, 0 );
 *     tr_sessionGetDefaultSettings( &settings );
 *     configDir = tr_getDefaultConfigDir( "Transmission" );
 *     session = tr_sessionInit( "mac", configDir, true, &settings );
 *
 *     tr_bencFree( &settings );
 * @endcode
 *
 * @param tag "gtk", "macosx", "daemon", etc... this is only for pre-1.30 resume files
 * @param configDir where Transmission will look for resume files, blocklists, etc.
 * @param messageQueueingEnabled if false, messages will be dumped to stderr
 * @param settings libtransmission settings
 * @see tr_sessionGetDefaultSettings()
 * @see tr_sessionLoadSettings()
 * @see tr_getDefaultConfigDir()
 */
tr_session * tr_sessionInit( const char     * tag,
                             const char     * configDir,
                             tr_bool          messageQueueingEnabled,
                             struct tr_benc * settings );

/** @brief Update a session's settings from a benc dictionary like to the one used in tr_sessionInit() */
void tr_sessionSet( tr_session      * session,
                    struct tr_benc  * settings );

/** @brief Rescan the blocklists directory and reload whatever blocklist files are found there */
void tr_sessionReloadBlocklists( tr_session * session );


/** @brief End a libtransmission session
    @see tr_sessionInit() */
void tr_sessionClose( tr_session * );

/**
 * @brief Return the session's configuration directory.
 *
 * This is where transmission stores its .torrent files, .resume files,
 * blocklists, etc. It's set in tr_transmissionInit() and is immutable
 * during the session.
 */
const char * tr_sessionGetConfigDir( const tr_session * );

/**
 * @brief Set the per-session default download folder for new torrents.
 * @see tr_sessionInit()
 * @see tr_sessionGetDownloadDir()
 * @see tr_ctorSetDownloadDir()
 */
void tr_sessionSetDownloadDir( tr_session * session, const char * downloadDir );

/**
 * @brief Get the default download folder for new torrents.
 *
 * This is set by tr_sessionInit() or tr_sessionSetDownloadDir(),
 * and can be overridden on a per-torrent basis by tr_ctorSetDownloadDir().
 */
const char * tr_sessionGetDownloadDir( const tr_session * session );

/**
 * @brief Get available disk space (in bytes) for the default download folder.
 * @return zero or positive integer on success, -1 in case of error.
 */
int64_t tr_sessionGetDownloadDirFreeSpace( const tr_session * session );

/**
 * @brief Set the torrent's bandwidth priority.
 */
void tr_ctorSetBandwidthPriority( tr_ctor * ctor, tr_priority_t priority );

/**
 * @brief Get the torrent's bandwidth priority.
 */
tr_priority_t tr_ctorGetBandwidthPriority( const tr_ctor * ctor );


/**
 * @brief set the per-session incomplete download folder.
 *
 * When you add a new torrent and the session's incomplete directory is enabled,
 * the new torrent will start downloading into that directory, and then be moved
 * to tr_torrent.downloadDir when the torrent is finished downloading.
 *
 * Torrents are not moved as a result of changing the session's incomplete dir --
 * it's applied to new torrents, not existing ones.
 *
 * tr_torrentSetLocation() overrules the incomplete dir: when a user specifies
 * a new location, that becomes the torrent's new downloadDir and the torrent
 * is moved there immediately regardless of whether or not it's complete.
 *
 * @see tr_sessionInit()
 * @see tr_sessionGetIncompleteDir()
 * @see tr_sessionSetIncompleteDirEnabled()
 * @see tr_sessionGetIncompleteDirEnabled()
 */
void tr_sessionSetIncompleteDir( tr_session * session, const char * dir );

/** @brief get the per-session incomplete download folder */
const char* tr_sessionGetIncompleteDir( const tr_session * session );

/** @brief enable or disable use of the incomplete download folder */
void tr_sessionSetIncompleteDirEnabled( tr_session * session, tr_bool );

/** @brief get whether or not the incomplete download folder is enabled */
tr_bool tr_sessionIsIncompleteDirEnabled( const tr_session * session );


/**
 * @brief When enabled, newly-created files will have ".part" appended
 *        to their filename until the file is fully downloaded
 *
 * This is not retroactive -- toggling this will not rename existing files.
 * It only applies to new files created by Transmission after this API call.
 *
 * @see tr_sessionIsIncompleteFileNamingEnabled()
 */
void tr_sessionSetIncompleteFileNamingEnabled( tr_session * session, tr_bool );

/** @brief return whether or filenames will have ".part" at the end until they're complete */
tr_bool tr_sessionIsIncompleteFileNamingEnabled( const tr_session * session );

/**
 * @brief Set whether or not RPC calls are allowed in this session.
 *
 * @details If true, libtransmission will open a server socket to listen
 * for incoming http RPC requests as described in docs/rpc-spec.txt.
 *
 * This is intially set by tr_sessionInit() and can be
 * queried by tr_sessionIsRPCEnabled().
 */
void tr_sessionSetRPCEnabled( tr_session  * session,
                              tr_bool       isEnabled );

/** @brief Get whether or not RPC calls are allowed in this session.
    @see tr_sessionInit()
    @see tr_sessionSetRPCEnabled() */
tr_bool tr_sessionIsRPCEnabled( const tr_session * session );

/** @brief Specify which port to listen for RPC requests on.
    @see tr_sessionInit()
    @see tr_sessionGetRPCPort */
void tr_sessionSetRPCPort( tr_session  * session,
                           tr_port       port );

/** @brief Get which port to listen for RPC requests on.
    @see tr_sessionInit()
    @see tr_sessionSetRPCPort */
tr_port tr_sessionGetRPCPort( const tr_session * session );

/**
 * @brief Specify which base URL to use.
 *
 * @detail The RPC API is accessible under <url>/rpc, the web interface under
 * <url>/web.
 *
 *  @see tr_sessionGetRPCUrl
 */
void tr_sessionSetRPCUrl( tr_session  * session,
                          const char * url );

/**
 * @brief Get the base URL.
 * @see tr_sessionInit()
 * @see tr_sessionSetRPCUrl
 */
const char* tr_sessionGetRPCUrl( const tr_session * session );

/**
 * @brief Specify a whitelist for remote RPC access
 *
 * The whitelist is a comma-separated list of dotted-quad IP addresses
 * to be allowed. Wildmat notation is supported, meaning that
 * '?' is interpreted as a single-character wildcard and
 * '*' is interprted as a multi-character wildcard.
 */
void   tr_sessionSetRPCWhitelist( tr_session * session,
                                  const char * whitelist );

/** @brief get the Access Control List for allowing/denying RPC requests.
    @return a comma-separated string of whitelist domains.
    @see tr_sessionInit
    @see tr_sessionSetRPCWhitelist */
const char* tr_sessionGetRPCWhitelist( const tr_session * );

void  tr_sessionSetRPCWhitelistEnabled( tr_session * session,
                                        tr_bool      isEnabled );

tr_bool tr_sessionGetRPCWhitelistEnabled( const tr_session * session );

void  tr_sessionSetRPCPassword( tr_session * session,
                                const char * password );

void  tr_sessionSetRPCUsername( tr_session * session,
                                const char * username );

/** @brief get the password used to restrict RPC requests.
    @return the password string.
    @see tr_sessionInit()
    @see tr_sessionSetRPCPassword() */
const char* tr_sessionGetRPCPassword( const tr_session * session );

const char* tr_sessionGetRPCUsername( const tr_session * session  );

void  tr_sessionSetRPCPasswordEnabled( tr_session * session,
                                       tr_bool      isEnabled );

tr_bool tr_sessionIsRPCPasswordEnabled( const tr_session * session );

const char* tr_sessionGetRPCBindAddress( const tr_session * session );


typedef enum
{
    TR_RPC_TORRENT_ADDED,
    TR_RPC_TORRENT_STARTED,
    TR_RPC_TORRENT_STOPPED,
    TR_RPC_TORRENT_REMOVING,
    TR_RPC_TORRENT_CHANGED, /* catch-all for the "torrent-set" rpc method */
    TR_RPC_TORRENT_MOVED,
    TR_RPC_SESSION_CHANGED
}
tr_rpc_callback_type;

typedef enum
{
    /* no special handling is needed by the caller */
    TR_RPC_OK            = 0,

    /* indicates to the caller that the client will take care of
     * removing the torrent itself. For example the client may
     * need to keep the torrent alive long enough to cleanly close
     * some resources in another thread. */
    TR_RPC_NOREMOVE   = ( 1 << 1 )
}
tr_rpc_callback_status;

typedef tr_rpc_callback_status (*tr_rpc_func)(tr_session          * session,
                                              tr_rpc_callback_type  type,
                                              struct tr_torrent   * tor_or_null,
                                              void                * user_data );

/**
 * Register to be notified whenever something is changed via RPC,
 * such as a torrent being added, removed, started, stopped, etc.
 *
 * func is invoked FROM LIBTRANSMISSION'S THREAD!
 * This means func must be fast (to avoid blocking peers),
 * shouldn't call libtransmission functions (to avoid deadlock),
 * and shouldn't modify client-level memory without using a mutex!
 */
void tr_sessionSetRPCCallback( tr_session   * session,
                               tr_rpc_func    func,
                               void         * user_data );

/**
***
**/

tr_bool       tr_sessionIsProxyEnabled( const tr_session * );

tr_bool       tr_sessionIsProxyAuthEnabled( const tr_session * );

const char*   tr_sessionGetProxy( const tr_session * );

tr_port       tr_sessionGetProxyPort( const tr_session * );

tr_proxy_type tr_sessionGetProxyType( const tr_session * );

const char*   tr_sessionGetProxyUsername( const tr_session * );

const char*   tr_sessionGetProxyPassword( const tr_session * );

void          tr_sessionSetProxyEnabled( tr_session * session,
                                         tr_bool      isEnabled );

void          tr_sessionSetProxyAuthEnabled( tr_session * session,
                                             tr_bool      isEnabled );

void          tr_sessionSetProxy( tr_session * session,
                                  const char * proxy );

void          tr_sessionSetProxyPort( tr_session * session,
                                      tr_port      port );

void          tr_sessionSetProxyType( tr_session    * session,
                                      tr_proxy_type   proxy_type );

void          tr_sessionSetProxyUsername( tr_session * session,
                                          const char * username );

void          tr_sessionSetProxyPassword( tr_session * session,
                                          const char * password );

/**
***
**/

/** @brief Used by tr_sessionGetStats() and tr_sessionGetCumulativeStats() to give bandwidth statistics */
typedef struct tr_session_stats
{
    float       ratio;        /* TR_RATIO_INF, TR_RATIO_NA, or total up/down */
    uint64_t    uploadedBytes; /* total up */
    uint64_t    downloadedBytes; /* total down */
    uint64_t    filesAdded;   /* number of files added */
    uint64_t    sessionCount; /* program started N times */
    uint64_t    secondsActive; /* how long Transmisson's been running */
}
tr_session_stats;

/** @brief Get bandwidth use statistics about the current session */
void tr_sessionGetStats( const tr_session * session, tr_session_stats * setme );

/** @brief Get cumulative bandwidth use statistics for the current and past sessions */
void tr_sessionGetCumulativeStats( const tr_session * session, tr_session_stats * setme );

void tr_sessionClearStats( tr_session * session );

/**
 * @brief Set whether or not torrents are allowed to do peer exchanges.
 *
 * PEX is always disabled in private torrents regardless of this.
 * In public torrents, PEX is enabled by default.
 */
void     tr_sessionSetPexEnabled( tr_session  * session, tr_bool isEnabled );
tr_bool  tr_sessionIsPexEnabled( const tr_session * session );

tr_bool  tr_sessionIsDHTEnabled( const tr_session * session );
void     tr_sessionSetDHTEnabled( tr_session * session, tr_bool );

tr_bool  tr_sessionIsLPDEnabled( const tr_session * session );
void     tr_sessionSetLPDEnabled( tr_session * session, tr_bool enabled );

void     tr_sessionSetCacheLimit_MB( tr_session * session, int mb );
int      tr_sessionGetCacheLimit_MB( const tr_session * session );

void     tr_sessionSetLazyBitfieldEnabled( tr_session * session, tr_bool enabled );
tr_bool  tr_sessionIsLazyBitfieldEnabled( const tr_session * session );

tr_encryption_mode tr_sessionGetEncryption( tr_session * session );
void               tr_sessionSetEncryption( tr_session * session,
                                            tr_encryption_mode    mode );


/***********************************************************************
** Incoming Peer Connections Port
*/

void  tr_sessionSetPortForwardingEnabled( tr_session  * session,
                                          tr_bool       enabled );

tr_bool tr_sessionIsPortForwardingEnabled( const tr_session  * session );

void  tr_sessionSetPeerPort( tr_session  * session,
                             tr_port       port);

tr_port tr_sessionGetPeerPort( const tr_session * session );

tr_port tr_sessionSetPeerPortRandom( tr_session  * session );

void  tr_sessionSetPeerPortRandomOnStart( tr_session * session,
                                          tr_bool random );

tr_bool  tr_sessionGetPeerPortRandomOnStart( tr_session * session );

typedef enum
{
    TR_PORT_ERROR,
    TR_PORT_UNMAPPED,
    TR_PORT_UNMAPPING,
    TR_PORT_MAPPING,
    TR_PORT_MAPPED
}
tr_port_forwarding;

tr_port_forwarding tr_sessionGetPortForwarding( const tr_session * session );

typedef enum
{
    TR_CLIENT_TO_PEER = 0, TR_UP = 0,
    TR_PEER_TO_CLIENT = 1, TR_DOWN = 1
}
tr_direction;

/***
****
***/

/***
****  Primary session speed limits
***/

void tr_sessionSetSpeedLimit_KBps( tr_session *, tr_direction, int KBps );
int tr_sessionGetSpeedLimit_KBps( const tr_session *, tr_direction );

void     tr_sessionLimitSpeed         ( tr_session *, tr_direction, tr_bool );
tr_bool  tr_sessionIsSpeedLimited     ( const tr_session *, tr_direction );


/***
****  Alternative speed limits that are used during scheduled times
***/

void tr_sessionSetAltSpeed_KBps( tr_session *, tr_direction, int Bps );
int  tr_sessionGetAltSpeed_KBps( const tr_session *, tr_direction );

void     tr_sessionUseAltSpeed        ( tr_session *, tr_bool );
tr_bool  tr_sessionUsesAltSpeed       ( const tr_session * );

void     tr_sessionUseAltSpeedTime    ( tr_session *, tr_bool );
tr_bool  tr_sessionUsesAltSpeedTime   ( const tr_session * );

void     tr_sessionSetAltSpeedBegin   ( tr_session *, int minsSinceMidnight );
int      tr_sessionGetAltSpeedBegin   ( const tr_session * );

void     tr_sessionSetAltSpeedEnd     ( tr_session *, int minsSinceMidnight );
int      tr_sessionGetAltSpeedEnd     ( const tr_session * );

typedef enum
{
    TR_SCHED_SUN      = (1<<0),
    TR_SCHED_MON      = (1<<1),
    TR_SCHED_TUES     = (1<<2),
    TR_SCHED_WED      = (1<<3),
    TR_SCHED_THURS    = (1<<4),
    TR_SCHED_FRI      = (1<<5),
    TR_SCHED_SAT      = (1<<6),
    TR_SCHED_WEEKDAY  = (TR_SCHED_MON|TR_SCHED_TUES|TR_SCHED_WED|TR_SCHED_THURS|TR_SCHED_FRI),
    TR_SCHED_WEEKEND  = (TR_SCHED_SUN|TR_SCHED_SAT),
    TR_SCHED_ALL      = (TR_SCHED_WEEKDAY|TR_SCHED_WEEKEND)
}
tr_sched_day;

void     tr_sessionSetAltSpeedDay     ( tr_session *, tr_sched_day day );
tr_sched_day tr_sessionGetAltSpeedDay ( const tr_session * );

typedef void ( tr_altSpeedFunc )      ( tr_session *, tr_bool active, tr_bool userDriven, void * );
void     tr_sessionClearAltSpeedFunc  ( tr_session * );
void     tr_sessionSetAltSpeedFunc    ( tr_session *, tr_altSpeedFunc *, void * );


tr_bool  tr_sessionGetActiveSpeedLimit_KBps( const tr_session  * session,
                                             tr_direction        dir,
                                             double            * setme );

/***
****
***/

double     tr_sessionGetRawSpeed_KBps  ( const tr_session *, tr_direction );
double     tr_sessionGetPieceSpeed_KBps( const tr_session *, tr_direction );

void       tr_sessionSetRatioLimited  ( tr_session *, tr_bool isLimited );
tr_bool    tr_sessionIsRatioLimited   ( const tr_session * );

void       tr_sessionSetRatioLimit    ( tr_session *, double desiredRatio );
double     tr_sessionGetRatioLimit    ( const tr_session * );

void       tr_sessionSetIdleLimited  ( tr_session *, tr_bool isLimited );
tr_bool    tr_sessionIsIdleLimited   ( const tr_session * );

void       tr_sessionSetIdleLimit ( tr_session *, uint16_t idleMinutes );
uint16_t   tr_sessionGetIdleLimit ( const tr_session * );

void       tr_sessionSetPeerLimit( tr_session *, uint16_t maxGlobalPeers );
uint16_t   tr_sessionGetPeerLimit( const tr_session * );

void       tr_sessionSetPeerLimitPerTorrent( tr_session *, uint16_t maxGlobalPeers );
uint16_t   tr_sessionGetPeerLimitPerTorrent( const tr_session * );

tr_priority_t   tr_torrentGetPriority( const tr_torrent * );
void            tr_torrentSetPriority( tr_torrent *, tr_priority_t );

void       tr_sessionSetPaused        ( tr_session *, tr_bool isPaused );
tr_bool    tr_sessionGetPaused        ( const tr_session * );

void       tr_sessionSetDeleteSource  ( tr_session *, tr_bool deleteSource );
tr_bool    tr_sessionGetDeleteSource  ( const tr_session * );

/**
 *  Load all the torrents in tr_getTorrentDir().
 *  This can be used at startup to kickstart all the torrents
 *  from the previous session.
 */
tr_torrent ** tr_sessionLoadTorrents( tr_session  * session,
                                      tr_ctor     * ctor,
                                      int         * setmeCount );

/**
***
**/

tr_bool tr_sessionIsTorrentDoneScriptEnabled( const tr_session * );

void tr_sessionSetTorrentDoneScriptEnabled( tr_session *, tr_bool isEnabled );

const char * tr_sessionGetTorrentDoneScript( const tr_session * );

void tr_sessionSetTorrentDoneScript( tr_session *, const char * scriptFilename );


/** @} */

/**
***
**/


/***********************************************************************
** Message Logging
*/

typedef enum
{
    TR_MSG_ERR = 1,
    TR_MSG_INF = 2,
    TR_MSG_DBG = 3
}
tr_msg_level;

void tr_setMessageLevel( tr_msg_level );

typedef struct tr_msg_list
{
    /* TR_MSG_ERR, TR_MSG_INF, or TR_MSG_DBG */
    tr_msg_level level;

    /* The line number in the source file where this message originated */
    int line;

    /* Time the message was generated */
    time_t when;

    /* The torrent associated with this message,
     * or a module name such as "Port Forwarding" for non-torrent messages,
     * or NULL. */
    char *  name;

    /* The message */
    char *  message;

    /* The source file where this message originated */
    const char * file;

    /* linked list of messages */
    struct tr_msg_list * next;
}
tr_msg_list;

void          tr_setMessageQueuing( tr_bool isEnabled );

tr_bool       tr_getMessageQueuing( void );

tr_msg_list * tr_getQueuedMessages( void );

void          tr_freeMessageList( tr_msg_list * freeme );

/** @addtogroup Blocklists
    @{ */

/**
 * Specify a range of IPs for Transmission to block.
 *
 * Filename must be an uncompressed ascii file.
 *
 * libtransmission does not keep a handle to `filename'
 * after this call returns, so the caller is free to
 * keep or delete `filename' as it wishes.
 * libtransmission makes its own copy of the file
 * massaged into a binary format easier to search.
 *
 * The caller only needs to invoke this when the blocklist
 * has changed.
 *
 * Passing NULL for a filename will clear the blocklist.
 */
int     tr_blocklistSetContent   ( tr_session       * session,
                                   const char       * filename );

int     tr_blocklistGetRuleCount ( const tr_session * session );

tr_bool tr_blocklistExists       ( const tr_session * session );

tr_bool tr_blocklistIsEnabled    ( const tr_session * session );

void    tr_blocklistSetEnabled   ( tr_session       * session,
                                   tr_bool            isEnabled );

/** @brief The blocklist that ges updated when an RPC client
           invokes the "blocklist-update" method */
void tr_blocklistSetURL         ( tr_session *, const char * url );

const char * tr_blocklistGetURL ( const tr_session * );

/** @brief the file in the $config/blocklists/ directory that's
           used by tr_blocklistSetContent() and "blocklist-update" */
#define DEFAULT_BLOCKLIST_FILENAME "blocklist.bin"

/** @} */


/** @addtogroup tr_ctor Torrent Constructors
    @{

    Instantiating a tr_torrent had gotten more complicated as features were
    added. At one point there were four functions to check metainfo and five
    to create tr_torrent.

    To remedy this, a Torrent Constructor (struct tr_ctor) has been introduced:
    - Simplifies the API to two functions: tr_torrentParse() and tr_torrentNew()
    - You can set the fields you want; the system sets defaults for the rest.
    - You can specify whether or not your fields should supercede resume's.
    - We can add new features to tr_ctor without breaking tr_torrentNew()'s API.

    All the tr_ctor{Get,Set}*() functions with a return value return
    an error number, or zero if no error occurred.

    You must call one of the SetMetainfo() functions before creating
    a torrent with a tr_ctor. The other functions are optional.

    You can reuse a single tr_ctor to create a batch of torrents --
    just call one of the SetMetainfo() functions between each
    tr_torrentNew() call.

    Every call to tr_ctorSetMetainfo*() frees the previous metainfo.
 */

typedef enum
{
    TR_FALLBACK, /* indicates the ctor value should be used only
                    in case of missing resume settings */

    TR_FORCE, /* indicates the ctor value should be used
                 regardless of what's in the resume settings */
}
tr_ctorMode;

struct tr_benc;

/** @brief Create a new torrent constructor object used to instantiate a tr_torrent
    @param session the tr_session. NULL is allowed if you're only calling tr_torrentParse() rather than tr_torrentNew()
    @see tr_torrentNew(), tr_torrentParse() */
tr_ctor* tr_ctorNew( const tr_session * session_or_NULL );

/** @brief Free a torrent constructor object */
void  tr_ctorFree( tr_ctor * ctor );

/** @brief Set whether or not to delete the source .torrent file when a torrent is added. (Default: False) */
void  tr_ctorSetDeleteSource( tr_ctor * ctor, tr_bool doDelete );

/** @brief Set the constructor's metainfo from a magnet link */
int tr_ctorSetMetainfoFromMagnetLink( tr_ctor * ctor, const char * magnet_link );

/** @brief Set the constructor's metainfo from a raw benc already in memory */
int tr_ctorSetMetainfo( tr_ctor * ctor, const uint8_t * metainfo, size_t len );

/** @brief Set the constructor's metainfo from a local .torrent file */
int tr_ctorSetMetainfoFromFile( tr_ctor * ctor, const char * filename );

/**
 * @brief Set the constructor's metainfo from an already-existing file in tr_getTorrentDir().
 *
 * This is used by the mac client on startup to pick and choose which existing torrents to load
 */
int tr_ctorSetMetainfoFromHash( tr_ctor * ctor, const char * hashString );

/** @brief Set the maximum number of peers this torrent can connect to. (Default: 50) */
void tr_ctorSetPeerLimit( tr_ctor * ctor, tr_ctorMode mode, uint16_t peerLimit  );

/** @brief Set the download folder for the torrent being added with this ctor.
    @see tr_ctorSetDownloadDir()
    @see tr_sessionInit() */
void        tr_ctorSetDownloadDir( tr_ctor *    ctor,
                                   tr_ctorMode  mode,
                                   const char * directory );

/**
 * @brief Set the incompleteDir for this torrent.
 *
 * This is not a supported API call.
 * It only exists so the mac client can migrate
 * its older incompleteDir settings, and that's
 * the only place where it should be used.
 */
void tr_ctorSetIncompleteDir( tr_ctor * ctor, const char * directory );

/** Set whether or not the torrent begins downloading/seeding when created.
    (Default: not paused) */
void        tr_ctorSetPaused( tr_ctor      * ctor,
                              tr_ctorMode    mode,
                              tr_bool        isPaused );

/** @brief Set the priorities for files in a torrent */
void        tr_ctorSetFilePriorities( tr_ctor                * ctor,
                                      const tr_file_index_t  * files,
                                      tr_file_index_t          fileCount,
                                      tr_priority_t            priority );

/** @brief Set the download flag for files in a torrent */
void        tr_ctorSetFilesWanted( tr_ctor                * ctor,
                                   const tr_file_index_t  * fileIndices,
                                   tr_file_index_t          fileCount,
                                   tr_bool                  wanted );


/** @brief Get this peer constructor's peer limit */
int         tr_ctorGetPeerLimit( const tr_ctor * ctor,
                                 tr_ctorMode     mode,
                                 uint16_t *      setmeCount );

/** @brief Get the "isPaused" flag from this peer constructor */
int         tr_ctorGetPaused( const tr_ctor * ctor,
                              tr_ctorMode     mode,
                              tr_bool       * setmeIsPaused );

/** @brief Get the download path from this peer constructor */
int         tr_ctorGetDownloadDir( const tr_ctor  * ctor,
                                   tr_ctorMode      mode,
                                   const char    ** setmeDownloadDir );

/** @brief Get the incomplete directory from this peer constructor */
int         tr_ctorGetIncompleteDir( const tr_ctor  * ctor,
                                     const char    ** setmeIncompleteDir );

/** @brief Get the metainfo from this peer constructor */
int         tr_ctorGetMetainfo( const tr_ctor         * ctor,
                                const struct tr_benc ** setme );

/** @brief Get the "delete .torrent file" flag from this peer constructor */
int         tr_ctorGetDeleteSource( const tr_ctor  * ctor,
                                    tr_bool        * setmeDoDelete );

/** @brief Get the tr_session poiner from this peer constructor */
tr_session* tr_ctorGetSession( const tr_ctor * ctor );

/** @brief Get the .torrent file that this ctor's metainfo came from, or NULL if tr_ctorSetMetainfoFromFile() wasn't used */
const char* tr_ctorGetSourceFile( const tr_ctor * ctor );

typedef enum
{
    TR_PARSE_OK,
    TR_PARSE_ERR,
    TR_PARSE_DUPLICATE
}
tr_parse_result;

/**
 * @brief Parses the specified metainfo
 *
 * @return TR_PARSE_ERR if parsing failed;
 *         TR_PARSE_OK if parsing succeeded and it's not a duplicate;
 *         TR_PARSE_DUPLICATE if parsing succeeded but it's a duplicate.
 *
 * @param setme_info If parsing is successful and setme_info is non-NULL,
 *                   the parsed metainfo is stored there and sould be freed
 *                   by calling tr_metainfoFree() when no longer needed.
 *
 * Notes:
 *
 * 1. tr_torrentParse() won't be able to check for duplicates -- and therefore
 *    won't return TR_PARSE_DUPLICATE -- unless ctor's "download-dir" and
 *    session variable is set.
 *
 * 2. setme_info->torrent's value can't be set unless ctor's session variable
 *    is set.
 */
tr_parse_result  tr_torrentParse( const tr_ctor  * ctor,
                                  tr_info        * setme_info_or_NULL );

/** @brief free a metainfo
    @see tr_torrentParse */
void tr_metainfoFree( tr_info * inf );


/** Instantiate a single torrent.
    @return 0 on success,
            TR_EINVALID if the torrent couldn't be parsed, or
            TR_EDUPLICATE if there's already a matching torrent object. */
tr_torrent * tr_torrentNew( const tr_ctor   * ctor,
                            int             * setmeError );

/** @} */

/***********************************************************************
 ***
 ***  TORRENTS
 **/

/** @addtogroup tr_torrent Torrents
    @{ */

/** @brief Frees memory allocated by tr_torrentNew().
           Running torrents are stopped first. */
void tr_torrentFree( tr_torrent * torrent );

typedef int tr_fileFunc( const char * filename );

/** @brief Removes our .torrent and .resume files for
           this torrent, then calls tr_torrentFree(). */
void tr_torrentRemove( tr_torrent  * torrent,
                       tr_bool       removeLocalData,
                       tr_fileFunc   removeFunc );

/** @brief Start a torrent */
void tr_torrentStart( tr_torrent * torrent );

/** @brief Stop (pause) a torrent */
void tr_torrentStop( tr_torrent * torrent );

enum
{
    TR_LOC_MOVING,
    TR_LOC_DONE,
    TR_LOC_ERROR
};

/**
 * @brief Tell transmsision where to find this torrent's local data.
 *
 * if move_from_previous_location is `true', the torrent's incompleteDir
 * will be clobberred s.t. additional files being added will be saved
 * to the torrent's downloadDir.
 */
void tr_torrentSetLocation( tr_torrent  * torrent,
                            const char  * location,
                            tr_bool       move_from_previous_location,
                            double      * setme_progress,
                            int         * setme_state );

uint64_t tr_torrentGetBytesLeftToAllocate( const tr_torrent * torrent );

/**
 * @brief Returns this torrent's unique ID.
 *
 * IDs are good as simple lookup keys, but are not persistent
 * between sessions. If you need that, use tr_info.hash or
 * tr_info.hashString.
 */
int tr_torrentId( const tr_torrent * torrent );

tr_torrent* tr_torrentFindFromId( tr_session * session, int id );

tr_torrent* tr_torrentFindFromHash( tr_session * session, const uint8_t * hash );

/** @brief Convenience function similar to tr_torrentFindFromHash() */
tr_torrent* tr_torrentFindFromMagnetLink( tr_session * session, const char * link );


/**
 * @brief find the location of a torrent's file by looking with and without
 *        the ".part" suffix, looking in downloadDir and incompleteDir, etc.
 * @return a newly-allocated string (that must be tr_freed() by the caller when done)
 *         that gives the location of this file on disk, or NULL if no file exists yet.
 * @param tor the torrent whose file we're looking for
 * @param fileNum the fileIndex, in [0...tr_info.fileCount)
 */
char* tr_torrentFindFile( const tr_torrent * tor, tr_file_index_t fileNo );


/***
****  Torrent speed limits
****
***/

void     tr_torrentSetSpeedLimit_KBps  ( tr_torrent *, tr_direction, int KBps );
int      tr_torrentGetSpeedLimit_KBps  ( const tr_torrent *, tr_direction );

void     tr_torrentUseSpeedLimit      ( tr_torrent *, tr_direction, tr_bool );
tr_bool  tr_torrentUsesSpeedLimit     ( const tr_torrent *, tr_direction );

void     tr_torrentUseSessionLimits   ( tr_torrent *, tr_bool );
tr_bool  tr_torrentUsesSessionLimits  ( const tr_torrent * );


/****
*****  Ratio Limits
****/

typedef enum
{
    TR_RATIOLIMIT_GLOBAL    = 0, /* follow the global settings */
    TR_RATIOLIMIT_SINGLE    = 1, /* override the global settings, seeding until a certain ratio */
    TR_RATIOLIMIT_UNLIMITED = 2  /* override the global settings, seeding regardless of ratio */
}
tr_ratiolimit;

void          tr_torrentSetRatioMode( tr_torrent         * tor,
                                      tr_ratiolimit        mode );

tr_ratiolimit tr_torrentGetRatioMode( const tr_torrent   * tor );

void          tr_torrentSetRatioLimit( tr_torrent        * tor,
                                       double              ratio );

double        tr_torrentGetRatioLimit( const tr_torrent  * tor );


tr_bool       tr_torrentGetSeedRatio( const tr_torrent *, double * ratio );


/****
*****  Idle Time Limits
****/

typedef enum
{
    TR_IDLELIMIT_GLOBAL    = 0, /* follow the global settings */
    TR_IDLELIMIT_SINGLE    = 1, /* override the global settings, seeding until a certain idle time */
    TR_IDLELIMIT_UNLIMITED = 2  /* override the global settings, seeding regardless of activity */
}
tr_idlelimit;

void          tr_torrentSetIdleMode ( tr_torrent         * tor,
                                      tr_idlelimit         mode );

tr_idlelimit  tr_torrentGetIdleMode ( const tr_torrent   * tor );

void          tr_torrentSetIdleLimit( tr_torrent         * tor,
                                      uint16_t             idleMinutes );

uint16_t      tr_torrentGetIdleLimit( const tr_torrent   * tor );


tr_bool       tr_torrentGetSeedIdle( const tr_torrent *, uint16_t * idleMinutes );

/****
*****  Peer Limits
****/

void          tr_torrentSetPeerLimit( tr_torrent * tor,
                                      uint16_t     peerLimit );

uint16_t      tr_torrentGetPeerLimit( const tr_torrent * tor );

/****
*****  File Priorities
****/

enum
{
    TR_PRI_LOW    = -1,
    TR_PRI_NORMAL =  0, /* since NORMAL is 0, memset initializes nicely */
    TR_PRI_HIGH   =  1
};

/**
 * @brief Set a batch of files to a particular priority.
 *
 * @param priority must be one of TR_PRI_NORMAL, _HIGH, or _LOW
 */
void tr_torrentSetFilePriorities( tr_torrent             * torrent,
                                  const tr_file_index_t  * files,
                                  tr_file_index_t          fileCount,
                                  tr_priority_t            priority );

/**
 * @brief Get this torrent's file priorities.
 *
 * @return A malloc()ed array of tor->info.fileCount items,
 *         each holding a TR_PRI_NORMAL, TR_PRI_HIGH, or TR_PRI_LOW.
 *         It's the caller's responsibility to free() this.
 */
tr_priority_t*  tr_torrentGetFilePriorities( const tr_torrent * torrent );

/** @brief Set a batch of files to be downloaded or not. */
void tr_torrentSetFileDLs( tr_torrent             * torrent,
                           const tr_file_index_t  * files,
                           tr_file_index_t          fileCount,
                           tr_bool                  do_download );


const tr_info * tr_torrentInfo( const tr_torrent * torrent );

/* Raw function to change the torrent's downloadDir field.
   This should only be used by libtransmission or to bootstrap
   a newly-instantiated tr_torrent object. */
void tr_torrentSetDownloadDir( tr_torrent  * torrent, const char * path );

const char * tr_torrentGetDownloadDir( const tr_torrent * torrent );

/**
 * Returns the root directory of where the torrent is.
 *
 * This will usually be the downloadDir. However if the torrent
 * has an incompleteDir enabled and hasn't finished downloading
 * yet, that will be returned instead.
 */
const char * tr_torrentGetCurrentDir( const tr_torrent * tor );


/**
 * Returns a newly-allocated string with a magnet link of the torrent.
 * Use tr_free() to free the string when done.
 */
char* tr_torrentGetMagnetLink( const tr_torrent * tor );

/**
***
**/


/** @brief a part of tr_info that represents a single tracker */
typedef struct tr_tracker_info
{
    int      tier;
    char *   announce;
    char *   scrape;
    uint32_t id; /* unique identifier used to match to a tr_tracker_stat */
}
tr_tracker_info;

/**
 * @brief Modify a torrent's tracker list.
 *
 * This updates both the `torrent' object's tracker list
 * and the metainfo file in tr_sessionGetConfigDir()'s torrent subdirectory.
 *
 * @param torrent The torrent whose tracker list is to be modified
 * @param trackers An array of trackers, sorted by tier from first to last.
 *                 NOTE: only the `tier' and `announce' fields are used.
 *                 libtransmission derives `scrape' from `announce'
 *                 and reassigns 'id'.
 * @param trackerCount size of the `trackers' array
 */
tr_bool
tr_torrentSetAnnounceList( tr_torrent             * torrent,
                           const tr_tracker_info  * trackers,
                           int                      trackerCount );


/**
***
**/

typedef enum
{
    TR_LEECH,           /* doesn't have all the desired pieces */
    TR_SEED,            /* has the entire torrent */
    TR_PARTIAL_SEED     /* has the desired pieces, but not the entire torrent */
}
tr_completeness;

/**
 * @param wasRunning whether or not the torrent was running when
 *                   it changed its completeness state
 */
typedef void ( tr_torrent_completeness_func )( tr_torrent       * torrent,
                                               tr_completeness    completeness,
                                               tr_bool            wasRunning,
                                               void             * user_data );

typedef void ( tr_torrent_ratio_limit_hit_func )( tr_torrent   * torrent,
                                                  void         * user_data );

typedef void ( tr_torrent_idle_limit_hit_func )( tr_torrent   * torrent,
                                                 void         * user_data );


/**
 * Register to be notified whenever a torrent's "completeness"
 * changes. This will be called, for example, when a torrent
 * finishes downloading and changes from TR_LEECH to
 * either TR_SEED or TR_PARTIAL_SEED.
 *
 * func is invoked FROM LIBTRANSMISSION'S THREAD!
 * This means func must be fast (to avoid blocking peers),
 * shouldn't call libtransmission functions (to avoid deadlock),
 * and shouldn't modify client-level memory without using a mutex!
 *
 * @see tr_completeness
 */
void tr_torrentSetCompletenessCallback(
         tr_torrent                    * torrent,
         tr_torrent_completeness_func    func,
         void                          * user_data );

void tr_torrentClearCompletenessCallback( tr_torrent * torrent );



typedef void ( tr_torrent_metadata_func )( tr_torrent  * torrent,
                                           void        * user_data );
/**
 * Register to be notified whenever a torrent changes from
 * having incomplete metadata to having complete metadata.
 * This happens when a magnet link finishes downloading
 * metadata from its peers.
 */
void tr_torrentSetMetadataCallback (
         tr_torrent                * tor,
         tr_torrent_metadata_func     func,
         void                      * user_data );

/**
 * Register to be notified whenever a torrent's ratio limit
 * has been hit. This will be called when the torrent's
 * ul/dl ratio has met or exceeded the designated ratio limit.
 *
 * Has the same restrictions as tr_torrentSetCompletenessCallback
 */
void tr_torrentSetRatioLimitHitCallback(
     tr_torrent                     * torrent,
     tr_torrent_ratio_limit_hit_func  func,
     void                           * user_data );

void tr_torrentClearRatioLimitHitCallback( tr_torrent * torrent );

/**
 * Register to be notified whenever a torrent's idle limit
 * has been hit. This will be called when the seeding torrent's
 * idle time has met or exceeded the designated idle limit.
 *
 * Has the same restrictions as tr_torrentSetCompletenessCallback
 */
void tr_torrentSetIdleLimitHitCallback(
     tr_torrent                          * torrent,
     tr_torrent_idle_limit_hit_func        func,
     void                                * user_data );

void tr_torrentClearIdleLimitHitCallback( tr_torrent * torrent );


/**
 * MANUAL ANNOUNCE
 *
 * Trackers usually set an announce interval of 15 or 30 minutes.
 * Users can send one-time announce requests that override this
 * interval by calling tr_torrentManualUpdate().
 *
 * The wait interval for tr_torrentManualUpdate() is much smaller.
 * You can test whether or not a manual update is possible
 * (for example, to desensitize the button) by calling
 * tr_torrentCanManualUpdate().
 */

void tr_torrentManualUpdate( tr_torrent * torrent );

tr_bool tr_torrentCanManualUpdate( const tr_torrent * torrent );

/***
****  tr_peer_stat
***/

typedef struct tr_peer_stat
{
    tr_bool  isEncrypted;
    tr_bool  isDownloadingFrom;
    tr_bool  isUploadingTo;
    tr_bool  isSeed;

    tr_bool  peerIsChoked;
    tr_bool  peerIsInterested;
    tr_bool  clientIsChoked;
    tr_bool  clientIsInterested;
    tr_bool  isIncoming;

    uint8_t  from;
    tr_port  port;

    char     addr[TR_INET6_ADDRSTRLEN];
    char     client[80];
    char     flagStr[32];

    float    progress;
    double   rateToPeer_KBps;
    double   rateToClient_KBps;


/***
****  THESE NEXT FOUR FIELDS ARE EXPERIMENTAL.
****  They're currently being used in the GTK+ client to help tune the new download congestion code
****  and probably won't make the cut for 2.0.
***/
    /* how many blocks we've sent to this peer in the last 120 seconds */
    uint32_t  blocksToPeer;
    /* how many blocks this client's sent to us in the last 120 seconds */
    uint32_t  blocksToClient;
    /* how many requests to this peer that we've cancelled in the last 120 seconds */
    uint32_t  cancelsToPeer;
    /* how many requests this peer made of us, then cancelled, in the last 120 seconds */
    uint32_t  cancelsToClient;

    /* how many requests the peer has made that we haven't responded to yet */
    int      pendingReqsToClient;

    /* how many requests we've made and are currently awaiting a response for */
    int      pendingReqsToPeer;
}
tr_peer_stat;

tr_peer_stat * tr_torrentPeers( const tr_torrent * torrent,
                                int *              peerCount );

void           tr_torrentPeersFree( tr_peer_stat * peerStats,
                                    int            peerCount );

/***
****  tr_tracker_stat
***/

typedef enum
{
    /* we won't (announce,scrape) this torrent to this tracker because
     * the torrent is stopped, or because of an error, or whatever */
    TR_TRACKER_INACTIVE = 0,

    /* we will (announce,scrape) this torrent to this tracker, and are
     * waiting for enough time to pass to satisfy the tracker's interval */
    TR_TRACKER_WAITING = 1,

    /* it's time to (announce,scrape) this torrent, and we're waiting on a
     * a free slot to open up in the announce manager */
    TR_TRACKER_QUEUED = 2,

    /* we're (announcing,scraping) this torrent right now */
    TR_TRACKER_ACTIVE = 3
}
tr_tracker_state;

typedef struct
{
    /* how many downloads this tracker knows of (-1 means it does not know) */
    int downloadCount;

    /* whether or not we've ever sent this tracker an announcement */
    tr_bool hasAnnounced;

    /* whether or not we've ever scraped to this tracker */
    tr_bool hasScraped;

    /* ex: http://www.legaltorrents.com:7070 */
    char host[1024];

    /* the full announce URL */
    char announce[1024];

    /* the full scrape URL */
    char scrape[1024];

    /* Transmission uses one tracker per tier,
     * and the others are kept as backups */
    tr_bool isBackup;

    /* is the tracker announcing, waiting, queued, etc */
    tr_tracker_state announceState;

    /* is the tracker scraping, waiting, queued, etc */
    tr_tracker_state scrapeState;

    /* number of peers the tracker told us about last time.
     * if "lastAnnounceSucceeded" is false, this field is undefined */
    int lastAnnouncePeerCount;

    /* human-readable string with the result of the last announce.
       if "hasAnnounced" is false, this field is undefined */
    char lastAnnounceResult[128];

    /* when the last announce was sent to the tracker.
     * if "hasAnnounced" is false, this field is undefined */
    time_t lastAnnounceStartTime;

    /* whether or not the last announce was a success.
       if "hasAnnounced" is false, this field is undefined */
    tr_bool lastAnnounceSucceeded;

    /* whether or not the last announce timed out. */
    tr_bool lastAnnounceTimedOut;

    /* when the last announce was completed.
       if "hasAnnounced" is false, this field is undefined */
    time_t lastAnnounceTime;

    /* human-readable string with the result of the last scrape.
     * if "hasScraped" is false, this field is undefined */
    char lastScrapeResult[128];

    /* when the last scrape was sent to the tracker.
     * if "hasScraped" is false, this field is undefined */
    time_t lastScrapeStartTime;

    /* whether or not the last scrape was a success.
       if "hasAnnounced" is false, this field is undefined */
    tr_bool lastScrapeSucceeded;

    /* whether or not the last scrape timed out. */
    tr_bool lastScrapeTimedOut;

    /* when the last scrape was completed.
       if "hasScraped" is false, this field is undefined */
    time_t lastScrapeTime;

    /* number of leechers this tracker knows of (-1 means it does not know) */
    int leecherCount;

    /* when the next periodic announce message will be sent out.
       if announceState isn't TR_TRACKER_WAITING, this field is undefined */
    time_t nextAnnounceTime;

    /* when the next periodic scrape message will be sent out.
       if scrapeState isn't TR_TRACKER_WAITING, this field is undefined */
    time_t nextScrapeTime;

    /* number of seeders this tracker knows of (-1 means it does not know) */
    int seederCount;

    /* which tier this tracker is in */
    int tier;

    /* used to match to a tr_tracker_info */
    uint32_t id;
}
tr_tracker_stat;

tr_tracker_stat * tr_torrentTrackers( const tr_torrent * torrent,
                                      int              * setmeTrackerCount );

void tr_torrentTrackersFree( tr_tracker_stat * trackerStats,
                             int               trackerCount );



/**
 * @brief get the download speeds for each of this torrent's webseed sources.
 *
 * @return an array of tor->info.webseedCount floats giving download speeds.
 *         Each speed in the array corresponds to the webseed at the same
 *         array index in tor->info.webseeds.
 *         To differentiate "idle" and "stalled" status, idle webseeds will
 *         return -1 instead of 0 KiB/s.
 *         NOTE: always free this array with tr_free() when you're done with it.
 */
double*  tr_torrentWebSpeeds_KBps( const tr_torrent * torrent );

typedef struct tr_file_stat
{
    uint64_t    bytesCompleted;
    float       progress;
}
tr_file_stat;

tr_file_stat * tr_torrentFiles( const tr_torrent  * torrent,
                                tr_file_index_t   * fileCount );

void tr_torrentFilesFree( tr_file_stat     * files,
                          tr_file_index_t    fileCount );


/***********************************************************************
 * tr_torrentAvailability
 ***********************************************************************
 * Use this to draw an advanced progress bar which is 'size' pixels
 * wide. Fills 'tab' which you must have allocated: each byte is set
 * to either -1 if we have the piece, otherwise it is set to the number
 * of connected peers who have the piece.
 **********************************************************************/
void tr_torrentAvailability( const tr_torrent  * torrent,
                             int8_t            * tab,
                             int                  size );

void tr_torrentAmountFinished( const tr_torrent  * torrent,
                               float *             tab,
                               int                 size );

void tr_torrentVerify( tr_torrent * torrent );

/**
 * This function will mark all data in files in the torrent as "verified",
 * if the files themselves already exist and are wanted (i.e. file->dnd
 * is FALSE).
 *
 * If the torrent is currently being verified, the verification is
 * cancelled. If the torrent is running, it is stopped and restarted
 * afterwards as part of this function.
 *
 * @note Since the piece data itself is not checked against the checksums
 *       in the torrent, using this function with partially complete files
 *       will result in bad data being sent to peers. This is not what
 *       you want. Take extra care when using this function, or just use
 *       tr_torrentVerify().
 *
 * @see tr_torrentVerify()
 * @see tr_torrentSetFileDLs()
 */
void tr_torrentSetFilesVerified( tr_torrent * tor );

/***********************************************************************
 * tr_info
 **********************************************************************/

/** @brief a part of tr_info that represents a single file of the torrent's content */
typedef struct tr_file
{
    uint64_t            length;    /* Length of the file, in bytes */
    char *              name;      /* Path to the file */
    int8_t              priority;  /* TR_PRI_HIGH, _NORMAL, or _LOW */
    int8_t              dnd;       /* nonzero if the file shouldn't be
                                     downloaded */
    tr_piece_index_t    firstPiece; /* We need pieces [firstPiece... */
    tr_piece_index_t    lastPiece; /* ...lastPiece] to dl this file */
    uint64_t            offset;    /* file begins at the torrent's nth byte */
}
tr_file;

/** @brief a part of tr_info that represents a single piece of the torrent's content */
typedef struct tr_piece
{
    time_t     timeChecked;             /* the last time we tested this piece */
    uint8_t    hash[SHA_DIGEST_LENGTH]; /* pieces hash */
    int8_t     priority;               /* TR_PRI_HIGH, _NORMAL, or _LOW */
    int8_t     dnd;                    /* nonzero if the piece shouldn't be
                                         downloaded */
}
tr_piece;

/** @brief information about a torrent that comes from its metainfo file */
struct tr_info
{
    /* total size of the torrent, in bytes */
    uint64_t           totalSize;

    /* the torrent's name */
    char             * name;

    /* Path to torrent Transmission's internal copy of the .torrent file. */
    char             * torrent;

    char            ** webseeds;

    char             * comment;
    char             * creator;
    tr_file          * files;
    tr_piece         * pieces;

    /* these trackers are sorted by tier */
    tr_tracker_info  * trackers;

    /* Torrent info */
    time_t             dateCreated;

    int                trackerCount;
    int                webseedCount;
    tr_file_index_t    fileCount;
    uint32_t           pieceSize;
    tr_piece_index_t   pieceCount;

    /* General info */
    uint8_t            hash[SHA_DIGEST_LENGTH];
    char               hashString[2 * SHA_DIGEST_LENGTH + 1];

    /* hash, escaped as per rfc2396 for tracker announces */
    char               hashEscaped[3 * SHA_DIGEST_LENGTH + 1];

    /* Flags */
    tr_bool            isPrivate;
    tr_bool            isMultifile;
};

static inline tr_bool tr_torrentHasMetadata( const tr_torrent * tor )
{
    return tr_torrentInfo( tor )->fileCount > 0;
}

/**
 * What the torrent is doing right now.
 *
 * Note: these values will become a straight enum at some point in the future.
 * Do not rely on their current `bitfield' implementation
 */
typedef enum
{
    TR_STATUS_CHECK_WAIT   = ( 1 << 0 ), /* Waiting in queue to check files */
    TR_STATUS_CHECK        = ( 1 << 1 ), /* Checking files */
    TR_STATUS_DOWNLOAD     = ( 1 << 2 ), /* Downloading */
    TR_STATUS_SEED         = ( 1 << 3 ), /* Seeding */
    TR_STATUS_STOPPED      = ( 1 << 4 )  /* Torrent is stopped */
}
tr_torrent_activity;

enum
{
    TR_PEER_FROM_INCOMING  = 0, /* connections made to the listening port */
    TR_PEER_FROM_LPD,           /* peers found by local announcements */
    TR_PEER_FROM_TRACKER,       /* peers found from a tracker */
    TR_PEER_FROM_DHT,           /* peers found from the DHT */
    TR_PEER_FROM_PEX,           /* peers found from PEX */
    TR_PEER_FROM_RESUME,        /* peers found in the .resume file */
    TR_PEER_FROM_LTEP,          /* peer address provided in an LTEP handshake */
    TR_PEER_FROM__MAX
};

typedef enum
{
    /* everything's fine */
    TR_STAT_OK               = 0,

    /* when we anounced to the tracker, we got a warning in the response */
    TR_STAT_TRACKER_WARNING  = 1,

    /* when we anounced to the tracker, we got an error in the response */
    TR_STAT_TRACKER_ERROR    = 2,

    /* local trouble, such as disk full or permissions error */
    TR_STAT_LOCAL_ERROR      = 3
}
tr_stat_errtype;

/** @brief Used by tr_torrentStat() to tell clients about a torrent's state and statistics */
typedef struct tr_stat
{
    /** The torrent's unique Id.
        @see tr_torrentId() */
    int    id;

    /** What is this torrent doing right now? */
    tr_torrent_activity activity;

    /** Defines what kind of text is in errorString.
        @see errorString */
    tr_stat_errtype error;

    /** A warning or error message regarding the torrent.
        @see error */
    char errorString[512];

    /** When tr_stat.activity is TR_STATUS_CHECK or TR_STATUS_CHECK_WAIT,
        this is the percentage of how much of the files has been
        verified. When it gets to 1, the verify process is done.
        Range is [0..1]
        @see tr_stat.activity */
    double recheckProgress;

    /** How much has been downloaded of the entire torrent.
        Range is [0..1] */
    double percentComplete;

    /** How much of the metadata the torrent has.
        For torrents added from a .torrent this will always be 1.
        For magnet links, this number will from from 0 to 1 as the metadata is downloaded.
        Range is [0..1] */
    double metadataPercentComplete;

    /** How much has been downloaded of the files the user wants. This differs
        from percentComplete if the user wants only some of the torrent's files.
        Range is [0..1]
        @see tr_stat.leftUntilDone */
    double percentDone;

    /** How much has been uploaded to satisfy the seed ratio.
        This is 1 if the ratio is reached or the torrent is set to seed forever.
        Range is [0..1] */
    double seedRatioPercentDone;

    /** Speed all data being sent for this torrent.
        This includes piece data, protocol messages, and TCP overhead */
    double rawUploadSpeed_KBps;

    /** Speed all data being received for this torrent.
        This includes piece data, protocol messages, and TCP overhead */
    double rawDownloadSpeed_KBps;

    /** Speed all piece being sent for this torrent.
        This ONLY counts piece data. */
    double pieceUploadSpeed_KBps;

    /** Speed all piece being received for this torrent.
        This ONLY counts piece data. */
    double pieceDownloadSpeed_KBps;

#define TR_ETA_NOT_AVAIL -1
#define TR_ETA_UNKNOWN -2
    /** If downloading, estimated number of seconds left until the torrent is done.
        If seeding, estimated number of seconds left until seed ratio is reached. */
    int    eta;
    /** If seeding, number of seconds left until the idle time limit is reached. */
    int    etaIdle;

    /** Number of peers that the tracker says this torrent has */
    int    peersKnown;

    /** Number of peers that we're connected to */
    int    peersConnected;

    /** How many peers we found out about from the tracker, or from pex,
        or from incoming connections, or from our resume file. */
    int    peersFrom[TR_PEER_FROM__MAX];

    /** Number of peers that are sending data to us. */
    int    peersSendingToUs;

    /** Number of peers that we're sending data to */
    int    peersGettingFromUs;

    /** Number of webseeds that are sending data to us. */
    int    webseedsSendingToUs;

    /** Byte count of all the piece data we'll have downloaded when we're done,
        whether or not we have it yet. This may be less than tr_info.totalSize
        if only some of the torrent's files are wanted.
        [0...tr_info.totalSize] */
    uint64_t    sizeWhenDone;

    /** Byte count of how much data is left to be downloaded until we've got
        all the pieces that we want. [0...tr_info.sizeWhenDone] */
    uint64_t    leftUntilDone;

    /** Byte count of all the piece data we want and don't have yet,
        but that a connected peer does have. [0...leftUntilDone] */
    uint64_t    desiredAvailable;

    /** Byte count of all the corrupt data you've ever downloaded for
        this torrent. If you're on a poisoned torrent, this number can
        grow very large. */
    uint64_t    corruptEver;

    /** Byte count of all data you've ever uploaded for this torrent. */
    uint64_t    uploadedEver;

    /** Byte count of all the non-corrupt data you've ever downloaded
        for this torrent. If you deleted the files and downloaded a second
        time, this will be 2*totalSize.. */
    uint64_t    downloadedEver;

    /** Byte count of all the checksum-verified data we have for this torrent.
      */
    uint64_t    haveValid;

    /** Byte count of all the partial piece data we have for this torrent.
        As pieces become complete, this value may decrease as portions of it
        are moved to `corrupt' or `haveValid'. */
    uint64_t    haveUnchecked;

    /** time when one or more of the torrent's trackers will
        allow you to manually ask for more peers,
        or 0 if you can't */
    time_t manualAnnounceTime;

#define TR_RATIO_NA  -1
#define TR_RATIO_INF -2
    /** TR_RATIO_INF, TR_RATIO_NA, or a regular ratio */
    float    ratio;

    /** When the torrent was first added. */
    time_t    addedDate;

    /** When the torrent finished downloading. */
    time_t    doneDate;

    /** When the torrent was last started. */
    time_t    startDate;

    /** The last time we uploaded or downloaded piece data on this torrent. */
    time_t    activityDate;

    /** Number of seconds since the last activity (or since started).
        -1 if activity is not seeding or downloading. */
    int    idleSecs;

    /** Cumulative seconds the torrent's ever spent downloading */
    int    secondsDownloading;

    /** Cumulative seconds the torrent's ever spent seeding */
    int    secondsSeeding;

    /** A torrent is considered finished if it has met its seed ratio.
        As a result, only paused torrents can be finished. */
    tr_bool   finished;
}
tr_stat;

/** Return a pointer to an tr_stat structure with updated information
    on the torrent. This is typically called by the GUI clients every
    second or so to get a new snapshot of the torrent's status. */
const tr_stat * tr_torrentStat( tr_torrent * torrent );

/** Like tr_torrentStat(), but only recalculates the statistics if it's
    been longer than a second since they were last calculated. This can
    reduce the CPU load if you're calling tr_torrentStat() frequently. */
const tr_stat * tr_torrentStatCached( tr_torrent * torrent );

/** @deprecated */
void tr_torrentSetAddedDate( tr_torrent * torrent,
                             time_t       addedDate );

/** @deprecated */
void tr_torrentSetActivityDate( tr_torrent * torrent,
                                time_t       activityDate );

/** @deprecated */
void tr_torrentSetDoneDate( tr_torrent * torrent, time_t doneDate );

/** @} */

/** @brief Sanity checker to test that the direction is TR_UP or TR_DOWN */
static inline tr_bool tr_isDirection( tr_direction d ) { return d==TR_UP || d==TR_DOWN; }

/** @brief Sanity checker to test that a bool is TRUE or FALSE */
static inline tr_bool tr_isBool( tr_bool b ) { return b==1 || b==0; }

#ifdef __cplusplus
}
#endif

#endif
