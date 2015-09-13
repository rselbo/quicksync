// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include "PreCompile.h"
#include "remoteobjectconnection.h"
#include <assert.h>

//-----------------------------------------------------------------------------

enum { RO_STATFILE, RO_STATFILEREPLY, RO_SENDFILE, RO_SENDFILERESULT, RO_TARGETDIRECTORY, RO_VERSION, RO_DELETEFILE };
const int RemoteObjectConnection::version = 2;

//-----------------------------------------------------------------------------

RemoteObjectConnection::RemoteObjectConnection( QTcpSocket *socket )
{
  fHash = -1;
  fSize = -1;
  isVersionKnown = false;
  
  if( socket )
  {
    fSocket = socket;
  }
  else
  {
    fSocket = new QTcpSocket();
  }
  
  connect( fSocket, SIGNAL(readyRead()), SLOT(readyRead()) );
//  connect( fSocket, SIGNAL(disconnected()), SLOT(disconnected()) );
//  connect( fSocket, SIGNAL(connected()), SLOT(connected()) );
  
  fStream.setDevice( fSocket );
}

RemoteObjectConnection::~RemoteObjectConnection()
{
  delete fSocket;
  fSocket = NULL;
}

//-----------------------------------------------------------------------------

void RemoteObjectConnection::sendTargetDirectory(const QString &filename)
{
  QByteArray data;
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << filename;
  sendRemoteObject(RO_TARGETDIRECTORY, data);
}

void RemoteObjectConnection::decodeTargetDirectory( QDataStream &stream )
{
  QString filename;
  stream >> filename;
  emit recvTargetDirectory( filename );
}

//-----------------------------------------------------------------------------

void RemoteObjectConnection::sendStatFileReq( const QString &filename )
{
  QByteArray data;
  QDataStream stream( &data, QIODevice::WriteOnly );
  stream << filename;
  sendRemoteObject( RO_STATFILE, data );
}

void RemoteObjectConnection::decodeStatFileReq( QDataStream &stream )
{
  QString filename;
  stream >> filename;
  emit recvStatFileReq( filename );
}

//-----------------------------------------------------------------------------

void RemoteObjectConnection::sendStatFileReply( const QString &filename, const QDateTime &mtime )
{
  QByteArray data;
  QDataStream stream( &data, QIODevice::WriteOnly );
  stream << filename << mtime.toUTC();
  sendRemoteObject( RO_STATFILEREPLY, data );
}

void RemoteObjectConnection::decodeStatFileReply( QDataStream &stream )
{
  //qDebug() << "[RemoteObjectConnection.Debug] decodeStatFileReply ";
  QString filename;
  QDateTime mtime;
  stream >> filename >> mtime;
  mtime = mtime.toLocalTime();
  //qDebug() << "[RemoteObjectConnection.Debug] decodeStatFileReply " << filename << " " << mtime;
  emit recvStatFileReply( filename, mtime );
}

//-----------------------------------------------------------------------------

void RemoteObjectConnection::sendSendFile( const QString &filename, const QDateTime &mtime, const QByteArray &data, bool executable )
{
  QByteArray sdata;
  QDataStream stream( &sdata, QIODevice::WriteOnly );
  stream << filename << mtime.toUTC() << data << executable;
  sendRemoteObject( RO_SENDFILE, sdata );
}

void RemoteObjectConnection::decodeSendFile( QDataStream &stream )
{
  QString filename;
  QDateTime mtime;
  QByteArray data;
  bool executable;
  stream >> filename >> mtime >> data >> executable;
  mtime = mtime.toLocalTime();
  emit recvSendFile( filename, mtime, data, executable );
}

//-----------------------------------------------------------------------------

void RemoteObjectConnection::sendSendFileResult( const QString &filename, const QDateTime &mtime, int result )
{
  QByteArray sdata;
  QDataStream stream( &sdata, QIODevice::WriteOnly );
  stream << filename << mtime.toUTC() << result;
  sendRemoteObject( RO_SENDFILERESULT, sdata );
}

void RemoteObjectConnection::decodeSendFileResult( QDataStream &stream )
{
  QString filename;
  QDateTime mtime;
  int result;
  stream >> filename >> mtime >> result;
  mtime = mtime.toLocalTime();
  emit recvSendFileResult( filename, mtime, result );
}

//-----------------------------------------------------------------------------

void RemoteObjectConnection::sendVersion()
{
  QByteArray sdata;
  QDataStream stream( &sdata, QIODevice::WriteOnly );
  stream << RemoteObjectConnection::version;
  isVersionKnown = true;
  sendRemoteObject( RO_VERSION, sdata );
}

void RemoteObjectConnection::decodeVersion( QDataStream &stream )
{
  int version;
  stream >> version;
  if(version != RemoteObjectConnection::version)
  {
    emit recvVersionMismatch();
  }
  else
  {
    isVersionKnown = true;
  }
}

//-----------------------------------------------------------------------------

void RemoteObjectConnection::sendDeleteFile( const QString &filename )
{
  QByteArray sdata;
  QDataStream stream( &sdata, QIODevice::WriteOnly );
  stream << filename;
  sendRemoteObject( RO_DELETEFILE, sdata );
}

void RemoteObjectConnection::decodeDeleteFile( QDataStream &stream )
{
  QString filename;
  stream >> filename;
  emit recvDeleteFile( filename );
}

//-----------------------------------------------------------------------------

void RemoteObjectConnection::sendRemoteObject( int commandid, const QByteArray &data )
{
  if(!isVersionKnown)
  {
    emit recvVersionMismatch();
    return;
  }

  fStream << commandid;
  fStream << data.size();
  //fStream << data;
  fSocket->write( data );
}

void RemoteObjectConnection::readyRead()
{
  //qDebug() << "[RemoteObjectConnection.Debug] ServerSocket::readyRead: " << fSocket->bytesAvailable();
  
  for(;;)
  {
    if( fSize==-1 && fSocket->bytesAvailable()>=8 )
    {
      fStream >> fHash >> fSize;
      //qDebug() << "[RemoteObjectConnection.Debug] ServerSocket::readyRead: " << fHash << " " << fSize;
    }
    if( fSize==-1 || (int)fSocket->bytesAvailable()<fSize )
    {
      break;
    }

    //qDebug() << "[RemoteObjectConnection.Debug] ServerSocket::readyRead: read command";
    
    QByteArray data = fSocket->read( fSize );
    assert( data.size() ==fSize );
    QDataStream stream( &data, QIODevice::ReadOnly );

    fSize = -1;
    
    switch( fHash )
    {
      case RO_TARGETDIRECTORY: decodeTargetDirectory(stream); break;
      case RO_STATFILE: decodeStatFileReq( stream ); break;
      case RO_STATFILEREPLY: decodeStatFileReply( stream ); break;
      case RO_SENDFILE: decodeSendFile( stream ); break;
      case RO_SENDFILERESULT: decodeSendFileResult( stream ); break;
      case RO_VERSION: decodeVersion( stream ); break;
      case RO_DELETEFILE: decodeDeleteFile( stream ); break;
      default:
        emit recvUnknownPacket();
    }
  }
}

void RemoteObjectConnection::disconnected()
{
  qDebug() << "[RemoteObjectConnection.Debug] RemoteObjectConnection::disconnected";
  //delete this;
}

void RemoteObjectConnection::connected()
{
  qDebug() << "[RemoteObjectConnection.Debug] RemoteObjectConnection::connected";
}

//-----------------------------------------------------------------------------


