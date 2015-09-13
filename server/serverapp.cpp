// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include "serverapp.h"
#include "serverconnection.h"

//-----------------------------------------------------------------------------

SyncSocketServer::SyncSocketServer( int port, const QString &sourcedir ) : QTcpServer()
{
  fSourceDir = sourcedir;

  connect( this, SIGNAL(newConnection()), SLOT(newConnection()) );
  
  if( !listen(QHostAddress::Any, port) )
  {
    qFatal( "SyncSocketServer: failed to bind to port" );
  }
}

void SyncSocketServer::newConnection()
{
  QTcpSocket *socket = nextPendingConnection();
  qDebug() << "new connection: " << socket;

  new ServerConnection( fSourceDir, socket );
}

//-----------------------------------------------------------------------------

ServerApp::ServerApp( int argc, char **argv ) : QCoreApplication( argc, argv )
{
  if( argc!=2 || atoi(argv[1])==0 )
  {
    qFatal( "Usage: syncserver <port>" );
  }
  
  fServer = new SyncSocketServer( atoi(argv[1]), "." );
}

ServerApp::~ServerApp()
{
  delete fServer;
}

//-----------------------------------------------------------------------------

int main( int argc, char **argv ) 
{
  ServerApp app( argc, argv );
  return app.exec();
}


