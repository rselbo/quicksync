#ifndef QUICKSYNC_FILESCANNER_H
#define QUICKSYNC_FILESCANNER_H

//#define USE_QDIR
#include "scannerbase.h"
#include "windows.h"

class SyncRules;
class DirRules
{
public:
  DirRules(const QString& path, QSharedPointer<SyncRules> rules) : m_Path(path), m_Rules(rules) {}
  QString m_Path;
  QSharedPointer<SyncRules> m_Rules;
};

class FileScanner : public ScannerBase
{
  typedef bool(*ProcessFile)(const QString& fileName);
public:
  FileScanner(const QString& rootPathDirRules, QSharedPointer<SyncRules> rules);

  virtual bool scanStep();
private:
  void scanDir(const QString& path, QSharedPointer<SyncRules> rules);
  QString m_RootDir;
  QList<DirRules> m_UnscannedDirs;
  ProcessFile m_FileProcessor;
  bool m_WriteLog;
};

class NewFileScanner : public QObject
{
  Q_OBJECT
  typedef bool(*ProcessFile)(const QString& fileName);
public:
  NewFileScanner(const QString& rootPathDirRules, ProcessFile fileProcesser);

  void ScanDir(const QString& path);
signals:
  // not threadsafe, do not queue to another thread
  void SignalFile(const QString& path);
private:
  ProcessFile m_FileProcessor;
  bool m_WriteLog;
};
#endif //QUICKSYNC_FILESCANNER_H
