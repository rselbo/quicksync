// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#ifndef QUICKSYNC_SERVERCONNECTION_H
#define QUICKSYNC_SERVERCONNECTION_H

#include "shared/remoteobjectconnection.h"

//-----------------------------------------------------------------------------

class ServerConnection : public RemoteObjectConnection
{
  Q_OBJECT
public:
  ServerConnection( const QString &sourcedir, QTcpSocket *socket );

private slots:
  void recvTargetDirectory(const QString &path);
  void recvStatFileReq( const QString &filename );
  void recvSendFile( const QString &filename, const QDateTime &mtime, const QByteArray &data, bool executable );
  void recvDeleteFile( const QString &filename );
private:
  QString fDefaultSourceDir;
  QString fSourceDir;

  QSet<QString> fFiles;
};

//-----------------------------------------------------------------------------

#endif
