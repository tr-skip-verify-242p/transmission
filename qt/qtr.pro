TARGET = transmission-qt
NAME = "Transmission"
DESCRIPTION = "Transmission: a fast, easy, and free BitTorrent client"
VERSION = 2.00
LICENSE = "GPL"

target.path = /bin
INSTALLS += target

unix: INSTALLS += man
man.path = /share/man/man1/
man.files = transmission-qt.1

CONFIG += qt qdbus thread debug link_pkgconfig
QT += network
PKGCONFIG = fontconfig libcurl openssl dbus-1

# if you have libevent2 installed in a custom path, specify it here
EVENT_TOP = /usr
INCLUDEPATH = $${EVENT_TOP}/include $${INCLUDEPATH}

TRANSMISSION_TOP = ..
INCLUDEPATH += $${TRANSMISSION_TOP}
LIBS += $${TRANSMISSION_TOP}/libtransmission/libtransmission.a
LIBS += $${TRANSMISSION_TOP}/third-party/dht/libdht.a
LIBS += $${TRANSMISSION_TOP}/third-party/miniupnp/libminiupnp.a
LIBS += $${TRANSMISSION_TOP}/third-party/libnatpmp/libnatpmp.a
unix: LIBS += -L$${EVENT_TOP}/lib -levent -lz
win32:DEFINES += QT_DBUS
win32:LIBS += -levent -lws2_32 -lintl
win32:LIBS += -lidn -liconv -lwldap32 -liphlpapi

TRANSLATIONS += translations/transmission_en.ts \
                translations/transmission_es.ts \
                translations/transmission_pt_BR.ts \
                translations/transmission_ru.ts

FORMS += mainwin.ui
RESOURCES += application.qrc
SOURCES += about.cc \
           add-data.cc \
           app.cc \
           dbus-adaptor.cc \
           details.cc \
           favicon.cc \
           file-tree.cc \
           filterbar.cc \
           filters.cc \
           formatter.cc \
           hig.cc \
           license.cc \
           mainwin.cc \
           make-dialog.cc \
           options.cc \
           prefs.cc \
           prefs-dialog.cc \
           qticonloader.cc \
           relocate.cc \
           session.cc \
           session-dialog.cc \
           squeezelabel.cc \
           stats-dialog.cc \
           torrent.cc torrent-delegate.cc \
           torrent-delegate-min.cc \
           torrent-filter.cc \
           torrent-model.cc \
           tracker-delegate.cc \
           tracker-model.cc \
           tracker-model-filter.cc \
           triconpushbutton.cc \
           utils.cc \
           watchdir.cc
HEADERS += $$replace(SOURCES, .cc, .h)
HEADERS += speed.h types.h

win32:RC_FILE = qtr.rc
