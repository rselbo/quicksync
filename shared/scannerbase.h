// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#ifndef QUICKSYNC_SCANNERBASE_H
#define QUICKSYNC_SCANNERBASE_H

//-----------------------------------------------------------------------------
//-------------------------------------
class SyncRules;

class ScannerBase : public QObject
{
  Q_OBJECT
public:
  class Error
  {
  public:
    Error( const QString &error ) { fError = error; }
    QString fError;
  };

  struct FileInfo { QDateTime mtime; bool binary; bool executable;};

  //-------------------------------------

  ScannerBase( const QString &rootPath, QSharedPointer<SyncRules> rules );
  virtual ~ScannerBase();
  
  static ScannerBase *selectScannerForFolder( const QString &rootPart, QSharedPointer<SyncRules> rules );
  
  virtual bool scanStep() =0;

    
  typedef QMap<QString,FileInfo>::const_iterator const_iterator;
  typedef QMap<QString,FileInfo>::iterator iterator;
  const QMap<QString,FileInfo> &allFiles() const { return fAllFiles; }
  void clearAllFiles() { fAllFiles.clear(); }
  
  int fileCount() const { return fFileCount; }
  int fileNumber() const { return fFileNumber; }
  int fileIgnored() const { return fFileIgnored; }
  int dirCount() const { return fDirCount; }
  int dirNumber() const { return fDirNumber; }
  int dirIgnored() const { return fDirIgnored; }

signals:
  void signalReparsePoint(const QString& dir);
  void signalSyncRuleFile(const QString& dir, QSharedPointer<SyncRules> rules);

protected:
  QString fRootPath;

  int fFileCount;
  int fFileNumber;
  int fFileIgnored;
  int fDirCount;
  int fDirNumber;
  int fDirIgnored;
  
  QMap<QString,FileInfo> fAllFiles;

};

//-----------------------------------------------------------------------------

#endif
