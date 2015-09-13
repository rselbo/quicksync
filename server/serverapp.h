// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#ifndef QUICKSYNC_SERVERAPP_H
#define QUICKSYNC_SERVERAPP_H

//-----------------------------------------------------------------------------

class SyncSocketServer : public QTcpServer
{
  Q_OBJECT
public:
  SyncSocketServer( int port, const QString &sourcedir );
  
private slots:
  void newConnection(); 

private:
  QString fSourceDir;
};

//-----------------------------------------------------------------------------

class ServerApp : public QCoreApplication
{
public:
  ServerApp( int argc, char **argv );
  virtual ~ServerApp();
  
private:
  SyncSocketServer *fServer;
};

//-----------------------------------------------------------------------------

#endif
