#ifndef FILESYSTEMWATCHER_H
#define FILESYSTEMWATCHER_H

class FileSystemWatcher : public QThread
{
  Q_OBJECT
public:
  FileSystemWatcher();
  virtual ~FileSystemWatcher();

  void setWatchDir(const QString& dir) { fDirToWatch = dir; }
  void setRelativeDir(const QString& dir) { fRelativeDir = dir; }
  void run();
  void stop() { fThreadRunning = false; }

signals:
  void fileAdded(QString filename);
  void fileDeleted(QString filename);
  void fileChanged(QString filename);
  void fileRenamed(QString oldName, QString newName);
  void filewatchError(QString error);
  void filewatchLostSync(QString lastError);
private:
  QString GetLastErrorStr();
  QString fDirToWatch;
  QString fRelativeDir;
  QString fOldFilename;
  bool fThreadRunning;
};

#endif //FILESYSTEMWATCHER_H
