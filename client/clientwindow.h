// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#ifndef QUICKSYNC_CLIENTWIN_H
#define QUICKSYNC_CLIENTWIN_H

#include "filestabledialog.h"
#include "filesystemwatcher.h"
#include "syncrules.h"
#include "scannerbase.h"

#pragma warning(disable:4003)
#include "ui_sync.h"
#include "syncsystem.h"

//-----------------------------------------------------------------------------

class RemoteObjectConnection;
class ScannerBase;


class ClientWindow : public QWidget, private Ui::MainWindow
{
  enum IconState { e_Main, e_Syncing, e_Disconnected, e_NodeWatching };
  Q_OBJECT
public:
  ClientWindow(const QString& branch="", bool initialupdate=false, bool shutdown=false);
  virtual ~ClientWindow();
  
signals:
  void closing();
  void signalSettingsChanged();
  void signalStartSync();
  void signalStopSync();

private slots:
  void slotDirsScanned(int dirsScanned, int dirsKnown, int dirsIgnored);
  void slotFilesScanned(int filesKnown, int filesIgnored);
  void slotFileStats(int filesResolved, int filesPendingStat);
  void slotFilesCopied(int filesCopied, int filesPendingCopy, int fileErrors);
  void slotFilesDeleted(int filesDeleted);
  void slotVersionMismatch();
  void slotUnknownPacket();
  void slotSyncStateChanged(int);
  void slotFileAction(QString fileName, QDateTime mTime, bool deleteFile);
  void slotFileStatus(QString fileName, QDateTime mTime, bool success);

  void on_settingsButton_clicked();
  void on_syncButton_clicked();

  void on_toolCopy_clicked();

  void on_branchSelector_currentIndexChanged();

  void closeEvent ( QCloseEvent *e );
  void changeEvent( QEvent* e );

  void slotSystemTrayIconActivated();

  void myMinimize();

private:
  void setIcon(IconState state);

  QSettings fSettings;


  //Flag to avoid saving the branch settings 
  bool fSaveBranch;
  //Commandline flag used to terminate the application after a sync
  bool fShutdown;
  //Commandline flag used to autosync when the connection is established
  bool fInitialUpdate;
  //Internal flag indicating if we are currently syncing files

  QIcon fMainIcon;
  QIcon fSyncIcon;
  QIcon fDisconnectIcon; 
  QIcon fNodeWatchingIcon;
  QSystemTrayIcon fSystemTrayIcon;

  QSharedPointer<SyncRules> m_SyncRules;  
  CopiedFilesDialog_c* fFilesTableDialog;
  SyncSystem *m_SyncSystem;
  QThread* m_SyncThread;
};

//-----------------------------------------------------------------------------

#endif
