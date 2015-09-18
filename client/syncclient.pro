# Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
# Content of this file is subject to the GPL v2
TEMPLATE	= app
TARGET		= syncclient

CONFIG      += qt warn_on precompile_header # thread
#CONFIG      += release # debug/release
#CONFIG      += console
QT          += core gui widgets network xml
INCLUDEPATH += ..

win32 {
  RC_FILE = resources/client.rc
}

FORMS = resources/copiedfilesdialog.ui resources/rulevisualizer.ui resources/rulevisualizer.ui resources/rulewidget.ui resources/settings.ui resources/sync.ui
INCLUDEPATH = ../pcre/include ../shared
HEADERS	= clientapp.h clientsettings.h clientwindow.h exceptionhandler.h filestabledialog.h filesystemwatcher.h \
          ruletreewidget.h rulevisualizerwidget.h rulevisualizerworker.h rulewidget.h syncrules.h syncruleviewmodel.h \
          syncsystem.h ../shared/filescanner.h ../shared/remoteobjectconnection.h ../shared/scannerbase.h ../shared/utils.h
SOURCES	= clientapp.cpp clientsettings.cpp clientwindow.cpp exceptionhandler.cpp filestabledialog.cpp filesystemwatcher.cpp \
          ruletreewidget.cpp rulevisualizerwidget.cpp rulevisualizerworker.cpp rulewidget.cpp syncrules.cpp syncruleviewmodel.cpp \
          syncsystem.cpp ../shared/filescanner.cpp ../shared/remoteobjectconnection.cpp ../shared/scannerbase.cpp ../shared/utils.cpp
