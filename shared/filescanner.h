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
public:
  FileScanner(const QString& rootPathDirRules, QSharedPointer<SyncRules> rules);

  virtual bool scanStep();
private:
  void scanDir(const QString& path, QSharedPointer<SyncRules> rules);
#if !defined( USE_QDIR)
  QString fRootDir;
  HANDLE fFile;
#else
  QDir fScanner;
#endif
  QList<DirRules> fUnscannedDirs;
};

#endif //QUICKSYNC_FILESCANNER_H
