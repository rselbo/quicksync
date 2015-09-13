# Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
# Content of this file is subject to the GPL v2
TEMPLATE	= app
TARGET		= syncserver

CONFIG      += qt warn_on precompile_header # thread
CONFIG      += debug # release
QT          = core network
INCLUDEPATH += ..

PRECOMPILED_HEADER = ../prefix.h

HEADERS		= serverapp.h serverconnection.h ../shared/remoteobjectconnection.h
SOURCES		= serverapp.cpp serverconnection.cpp \
	../shared/utils.cpp ../shared/remoteobjectconnection.cpp
