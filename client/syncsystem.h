#ifndef SYNCSYSTEM_H
#define SYNCSYSTEM_H

#include "scannerbase.h"
#include "remoteobjectconnection.h"
#include "syncrules.h"

class FileSystemWatcher;

struct FileTodo
{
  FileTodo() : m_Binary(false), m_Executable(false), m_Delete(false), m_Retries(0), m_Started(false), m_Size(0) {}
  FileTodo(const QString file, bool binary, bool executable, QTime time, bool deletefile, QDateTime mtime, qint64 size) :
  m_Filename(file), m_Binary(binary), m_Executable(executable), m_Time(time), m_Delete(deletefile), m_Mtime(mtime), m_Retries(0), m_Started(false), m_Size(size) {}
  QString m_Filename;
  bool m_Binary;
  bool m_Executable;
  QTime m_Time;
  QDateTime m_Mtime;
  // m_Delete true means we delete the file, false means normal update
  bool m_Delete;
  int m_Retries;
  bool m_Started;
  qint64 m_Size;
};

class SyncSystem : public QObject
{
  Q_OBJECT
public:
  enum SyncSystemState { e_Unconnected, e_Idle, e_NodeWatching, e_Syncing};

  SyncSystem(QSharedPointer<SyncRules> rules);
  virtual ~SyncSystem();

protected:

  void reconnect(int delay=1000);
  void sendFile(const QString &filename, bool binary, bool executable);
  void scanComplete();

signals:
  void signalDirsScanned(int dirsFinished, int dirsKnown, int dirsIgnored); 
  void signalFilesScanned(int filesKnown, int filesIgnored);
  void signalFileStats(int filesResolved, int filesPendingStat);
  void signalFilesCopied(int filesCopied, int filesPendingCopy, int fileErrors);
  void signalFilesDeleted(int filesDeleted);
  void signalFileAction(QString fileName, QDateTime mTime, bool deleteFile);
  void signalFileStatus(QString fileName, QDateTime mTime, bool success);

  void signalStateChanged(int state);
  void signalError(const QString& level, const QString& errorMessage);
  void signalVersionMismatch();
  void signalUnknownPacket();

private slots:
  void slotSettingsChanged();
  void slotStartSync();
  void slotReSync();
  void slotStopSync();
  void slotLostSync(QString error);

  void slotStartNodeWatching(const QString& dir);
  void slotSyncRuleFile(const QString&, QSharedPointer<SyncRules>);

  void slotFileAdded(QString file);
  void slotFileDeleted(QString file);
  void slotFileChanged(QString file);
  void slotFileRenamed(QString oldName, QString newName);
  void slotFilewatchError(QString file);

  void reconnectTimer();
  void connected();
  void disconnected();
  void error(QAbstractSocket::SocketError);

  void recvStatFileReply(const QString &, const QDateTime &);
  void recvSendFileResult(const QString &, const QDateTime &, int);

  void slotScanDir();
  void slotSyncUpdate();

public slots:
  void started();
  void finished();

private:
  void stopNodeWatching();
  void stopFullSync();
  void setSyncState(SyncSystemState state);
  void updateSyncState();
  void resetStats();
  void addTodo(const QString& fileName, bool binary, bool executable, bool deletefile, bool retry=false);
  void writeFileList();
  bool checkForRescan(const QString& name);
  QSharedPointer<SyncRules> GetSyncRulesForPath(const QString& path);
  RemoteObjectConnection *m_Connection;

  SyncSystemState m_SyncState;

  QList<FileSystemWatcher*> m_FileSystemWatchers;
  QMap<QString, ScannerBase::FileInfo> m_UnresolvedFiles;

  ScannerBase* m_Scanner;

  QMap<QString, FileTodo> m_NameToInfo;
  QSet<QString> m_Files;
  QSharedPointer<SyncRules> m_SyncRules;
  QMap<QString, QSharedPointer<SyncRules> > m_PathRules;
  QSettings m_Settings;

  QTimer* m_ReconnectTimer;
  QTimer* m_ScanDirTimer;
  QTimer* m_SyncUpdateTimer;
  //This runs when a nodewatcher fails. It will issue a resync when the timeout elapses without any changes to the filesystem
  QTimer* m_LostSyncTimer;

  QString m_CurrentSourcePath;
  QString m_CurrentDestinationPath;

  bool m_RestartSyncOnReconnect;

  //Stats
  int m_DirsFinished;
  int m_DirsKnown;
  int m_DirsIgnored;
  int m_FilesKnown;
  int m_FilesIgnored;
  int m_FilesResolved;
  int m_FilesPendingStat;
  int m_FilesCopied;
  int m_FilesPendingCopy;
  int m_FileErrors;
  int m_FilesDeleted;

  quint64 m_BytesInTransit;

};

#endif //SYNCSYSTEM_H