// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include "serverconnection.h"
#include "shared/utils.h"
#include <sys/time.h>

//-----------------------------------------------------------------------------

ServerConnection::ServerConnection( const QString &sourcedir, QTcpSocket *socket ) : 
  RemoteObjectConnection( socket )
{
  fDefaultSourceDir = sourcedir;
  fSourceDir = sourcedir;
  
  connect( this, SIGNAL(recvTargetDirectory(const QString &)), SLOT(recvTargetDirectory(const QString &)) );
  connect( this, SIGNAL(recvStatFileReq(const QString &)), SLOT(recvStatFileReq(const QString &)) );
  connect( this, SIGNAL(recvSendFile(const QString &, const QDateTime &, const QByteArray &, bool)), SLOT(recvSendFile(const QString &, const QDateTime &, const QByteArray &, bool)) );
  connect( this, SIGNAL(recvDeleteFile(const QString &)), SLOT(recvDeleteFile(const QString &)) );

  sendVersion();
}

void ServerConnection::recvTargetDirectory(const QString &path)
{
	if(path.isEmpty())
	{
		fSourceDir = fDefaultSourceDir;
	}
	else
	{
		fSourceDir = path;
	}
}

void ServerConnection::recvStatFileReq( const QString &filename )
{
  QString absfilepath = fSourceDir+"/"+filename;
  QFileInfo fileInfo( absfilepath );
  if( fileInfo.isFile() )
  {
    QDateTime time = fileInfo.lastModified();
    sendStatFileReply( filename, time );
  }
  else
  {
    qWarning() << "Could not get mtime for \"" << filename << "\"";
    sendStatFileReply( filename, QDateTime() );
  }
  
}

void ServerConnection::recvSendFile( const QString &filename, const QDateTime &mtime, const QByteArray &data, bool executable )
{
  qDebug() << "File " << fSourceDir << filename << " datasize " << data.size() << " date: " << mtime;

  QFile file( joinPath(fSourceDir,filename) );
  if( !file.open(QIODevice::WriteOnly) )
  {
    // maybe we are missing some directories
    QString filedir = joinPath( fSourceDir, filename ).section( '/', 0, -2 );
    QDir dir(filedir);
    bool result = true;
    if(!dir.exists())
    {
      result = dir.mkpath(filedir);
    }

    if( !result )
    {
      qWarning() << "Could not create path \"" << filedir << "\"";
    }
    else
    {
      file.open( QIODevice::WriteOnly );
    }
  }
  
  if( !file.isOpen() )
  {
    sendSendFileResult( filename, mtime, false );
    qWarning() << "Could not create file \"" << filename << "\"";
  }
  else
  {
    file.write( data );
    if(executable)
    {
      file.setPermissions(file.permissions()|QFile::ExeOwner|QFile::ExeGroup|QFile::ExeOther);
    }
    file.flush();
    
    // For some reason QFileInfo is lacking setLastModified()
    struct timeval times[2];
    times[0].tv_sec = times[1].tv_sec = mtime.toTime_t();
    times[0].tv_usec = times[1].tv_usec = 0;
    if( futimes(file.handle(), times) != 0 )
    {
      sendSendFileResult( filename, mtime, false );
      qWarning() << "Could not set times on file \"" << filename << "\"";
    }
    else
    {
      sendSendFileResult( filename, mtime, true );
    }
  }
}

void ServerConnection::recvDeleteFile( const QString &filename )
{
  qDebug() << "DeleteFile request for " << fSourceDir << filename;

  QFile file( joinPath(fSourceDir,filename) );
  if(!file.remove())
  {
    qDebug() << "Could not remove " << fSourceDir << filename;
  }
}


//-----------------------------------------------------------------------------


