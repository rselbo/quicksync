// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#ifndef QUICKSYNC_ROCONNECTION_H
#define QUICKSYNC_ROCONNECTION_H


//-----------------------------------------------------------------------------

class RemoteObject;

//-----------------------------------------------------------------------------

class RemoteObjectConnection : public QObject
{
  Q_OBJECT

public:
  RemoteObjectConnection( QTcpSocket *socket=NULL );
  virtual ~RemoteObjectConnection();
  
  void sendTargetDirectory(const QString &targetdirectory);
  void sendStatFileReq( const QString &filename );
  void sendStatFileReply( const QString &filename, const QDateTime &mtime );
  void sendSendFile( const QString &filename, const QDateTime &mtime, const QByteArray &data, bool executable );
  void sendSendFileResult( const QString &filename, const QDateTime &mtime, int result );
  void sendVersion();
  void sendDeleteFile( const QString &filename );
  QTcpSocket *fSocket;

signals:
  void recvTargetDirectory(const QString &filename);
  void recvStatFileReq( const QString &filename );
  void recvStatFileReply( const QString &filename, const QDateTime &mtime );
  void recvSendFile( const QString &filename, const QDateTime &mtime, const QByteArray &data, bool executable );
  void recvSendFileResult( const QString &filename, const QDateTime &mtime, int result );
  void recvVersionMismatch();
  void recvDeleteFile( const QString &filename );
  void recvUnknownPacket();
private:
  void decodeTargetDirectory( QDataStream &stream );
  void decodeStatFileReq( QDataStream &stream );
  void decodeStatFileReply( QDataStream &stream );
  void decodeSendFile( QDataStream &stream );
  void decodeSendFileResult( QDataStream &stream );
  void decodeVersion( QDataStream &stream );
  void decodeDeleteFile( QDataStream &stream );

private slots:
  void readyRead();
  void disconnected();
  void connected();
  
private:
  void sendRemoteObject( int commandid, const QByteArray &data );
  
  QDataStream fStream;
  qint32 fHash;
  qint32 fSize;
  bool isVersionKnown;

  static const int version;
};

//-----------------------------------------------------------------------------

#endif

