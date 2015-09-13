# Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
# Content of this file is subject to the GPL v2
TEMPLATE	= app
TARGET		= syncclient

CONFIG      += qt warn_on precompile_header # thread
#CONFIG      += release # debug/release
#CONFIG      += console
QT          += network xml
INCLUDEPATH += ..

FORMS			= ../resources/sync.ui ../resources/settings.ui

win32 {
  RC_FILE = ../resources/client.rc
}
RESOURCES	= ../resources/syncclient.qrc 
 
PRECOMPILED_HEADER = ../prefix.h

HEADERS		= clientapp.h clientwindow.h clientsettings.h ../shared/remoteobjectconnection.h
SOURCES		= clientapp.cpp clientwindow.cpp clientsettings.cpp \
	../shared/scannerbase.cpp ../shared/utils.cpp ../shared/remoteobjectconnection.cpp
