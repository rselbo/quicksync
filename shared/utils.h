// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#ifndef QUICKSYNC_UTILS_H
#define QUICKSYNC_UTILS_H

//-----------------------------------------------------------------------------

extern QString joinPath( const QString &path1, const QString &path2 );
extern QString joinPath( const QString &path1, const QString &path2, const QString &path3 );

//custom message level
QDebug qInformation();

void LogfileMessageHandler(QtMsgType type, const char *msg);
void CreateLogfile();
void CloseLogfile();

QString GetDataDir();
QString GetHumanReadableSize(quint64);
//-----------------------------------------------------------------------------

#endif

