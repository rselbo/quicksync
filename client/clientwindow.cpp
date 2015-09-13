// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include "PreCompile.h"
#include "clientapp.h"
#include "clientwindow.h"
#include "clientsettings.h"
#include "utils.h"
#include "rulevisualizerwidget.h"
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

ClientWindow::ClientWindow(const QString& branch, bool initialupdate, bool shutdown) : 
	QWidget( NULL ),
  fSaveBranch(false),
  fShutdown(shutdown),
  fInitialUpdate(initialupdate),
  fMainIcon(":/qs.png"),
  fSyncIcon(":/qssync.png"),
  fDisconnectIcon(":/qsdis.png"),
  fNodeWatchingIcon(":/qsnw.png"),
  m_SyncRules(new SyncRules),
  fFilesTableDialog(NULL)
{
  setupUi( this );


  QSettings settings;

  //Load window size and position
  settings.beginGroup("MainWindow");
  QSize minAppSize(210, 190);
  resize(settings.value("size", minAppSize).toSize());

  //get the stored position, and sanity check it a bit
  QPoint cPoint = settings.value("pos", QPoint(200, 200)).toPoint();
  QPoint sanePosition(std::min(std::max(cPoint.x(), 0), std::max(0, QApplication::desktop()->width() - minAppSize.width())),
                      std::min(std::max(cPoint.y(), 0), std::max(0, QApplication::desktop()->height() - minAppSize.height())));
  move(sanePosition);

  bool bCopiedOpen = settings.value("copiedopen", false).toBool();
  settings.endGroup();

  //Load currently configured branches
  settings.beginGroup(ClientSettings::branchesStr);
  QStringList branches = settings.childGroups();
  branchSelector->addItems(branches);
  settings.endGroup();

  //Load and set currently selected branch
  QString currentSelectedBranch;
  if(branch.isEmpty())
    currentSelectedBranch = settings.value("CurrentlySelectedBranch").toString();
  else
    currentSelectedBranch = branch;

  int index = branchSelector->findText(currentSelectedBranch);
  if(index >= 0)
  {
    qDebug() << "[ClientWindow.Init] setting index to " << index << " for " << currentSelectedBranch;
    branchSelector->setCurrentIndex(index);
  }
  else
  {
    QApplication::exit(1);
  }
  fSaveBranch = true;

  fFilesTableDialog = new CopiedFilesDialog_c;
  connect(this, SIGNAL(closing()), fFilesTableDialog, SLOT(saveWindowPos()));
  if(bCopiedOpen)
    on_toolCopy_clicked();

  slotSyncStateChanged(SyncSystem::e_Unconnected);

  connect(&fSystemTrayIcon, SIGNAL(activated( QSystemTrayIcon::ActivationReason )), SLOT(slotSystemTrayIconActivated()));
  fSystemTrayIcon.show();

  m_SyncRules->loadRules();

  m_SyncSystem = new SyncSystem(m_SyncRules);
  m_SyncThread = new QThread(this);

  connect(m_SyncThread, &QThread::started, m_SyncSystem, &SyncSystem::started);
  connect(m_SyncThread, &QThread::finished, m_SyncSystem, &SyncSystem::finished);
  connect(m_SyncSystem, SIGNAL(signalDirsScanned(int,int,int)), this, SLOT(slotDirsScanned(int,int,int)));
  connect(m_SyncSystem, SIGNAL(signalFilesScanned(int,int)), this, SLOT(slotFilesScanned(int,int)));
  connect(m_SyncSystem, SIGNAL(signalFileStats(int,int)), this, SLOT(slotFileStats(int,int)));
  connect(m_SyncSystem, SIGNAL(signalFilesCopied(int,int,int)), this, SLOT(slotFilesCopied(int,int,int)));
  connect(m_SyncSystem, SIGNAL(signalFilesDeleted(int)), this, SLOT(slotFilesDeleted(int)));
  connect(m_SyncSystem, SIGNAL(signalStateChanged(int)), this, SLOT(slotSyncStateChanged(int)));
  connect(m_SyncSystem, SIGNAL(signalVersionMismatch()), this, SLOT(slotVersionMismatch()));
  connect(m_SyncSystem, SIGNAL(signalUnknownPacket()), this, SLOT(slotUnknownPacket()));
  connect(m_SyncSystem, SIGNAL(signalFileAction(QString, QDateTime, bool)), this, SLOT(slotFileAction(QString, QDateTime, bool)));
  connect(m_SyncSystem, SIGNAL(signalFileStatus(QString, QDateTime, bool)), this, SLOT(slotFileStatus(QString, QDateTime, bool)));
  connect(this, SIGNAL(signalStartSync()), m_SyncSystem, SLOT(slotStartSync()));
  connect(this, SIGNAL(signalStopSync()), m_SyncSystem, SLOT(slotStopSync()));
  connect(this, SIGNAL(signalSettingsChanged()), m_SyncSystem, SLOT(slotSettingsChanged()));
  //Move the syncsystem into its own thread so the signals we send to it is executed in the threads own context
  m_SyncSystem->moveToThread(m_SyncThread);

  m_SyncThread->start();

}

//-----------------------------------------------------------------------------

ClientWindow::~ClientWindow()
{
  qDebug() << "ClientWindow::~ClientWindow";
  //shut down the sync system
  if(m_SyncThread)
  {
    m_SyncThread->quit();
    while(!m_SyncThread->isFinished())
      m_SyncThread->wait(1);
    m_SyncThread->disconnect(this);
    m_SyncSystem->disconnect(this);
    delete m_SyncThread;
    m_SyncThread = NULL;
    delete m_SyncSystem;
    m_SyncSystem = NULL;
  }
}

void ClientWindow::closeEvent(QCloseEvent*)
{
  fSystemTrayIcon.hide();

  QSettings settings;
  settings.beginGroup("MainWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  if(fFilesTableDialog)
  {
    settings.setValue("copiedopen", fFilesTableDialog->isVisible());
  }
  settings.endGroup();
}

//Handle showing and hiding the windows when losing and gaining focus
void ClientWindow::changeEvent( QEvent* e )
{
  //qDebug() << "[ClientWindow.Debug] ClientWindow::changeEvent " << e->type();
  if(e->type() == QEvent::ActivationChange && isActiveWindow())
  {
    if(fFilesTableDialog && fFilesTableDialog->isVisible())
      fFilesTableDialog->raise();

    activateWindow();
  }
  if(e->type() == QEvent::WindowStateChange)
  {
    if(isMinimized())
    {
      QTimer::singleShot(10, this, SLOT(myMinimize()));
    }
  }
  QWidget::changeEvent(e);
}

void ClientWindow::myMinimize()
{
  hide();
}

void ClientWindow::slotSystemTrayIconActivated()
{
  if(isMinimized())
  {
    show();
    showNormal();
  }
  activateWindow();
}
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

void ClientWindow::on_settingsButton_clicked()
{
  ClientSettings clientSettings(m_SyncRules);
  connect(&clientSettings, SIGNAL(signalClearStats()), m_SyncSystem, SLOT(slotClearStatData()));
  if(clientSettings.exec() == QDialog::Accepted)
  {
    //Update the branches list
    fSettings.beginGroup(ClientSettings::branchesStr);
    QStringList branches = fSettings.childGroups();
    branchSelector->clear();
    branchSelector->addItems(branches);
    fSettings.endGroup();

    //if(m_SyncRules != clientSettings.m_cIgnoreRules)
    //{
    //  //copy the new rule set and save
    //  m_SyncRules = clientSettings.m_cIgnoreRules;
    //  m_SyncRules->saveXmlRules(defaultFile);
    //}
  }
  else
  {
    //reload the syncrules on cancel
    m_SyncRules->loadRules();
  }

  //Load and set currently selected branch
  QString currentSelectedBranch = fSettings.value("CurrentlySelectedBranch").toString();
  branchSelector->setCurrentIndex(branchSelector->findText(currentSelectedBranch));
  fSaveBranch = true;

  emit signalSettingsChanged();
}


///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
void ClientWindow::slotDirsScanned(int dirsScanned, int dirsKnown, int dirsIgnored)
{
  if(dirsScanned == 0 && dirsKnown == 0 && dirsIgnored == 0)
    cvsDirProgressLabel->setText("");
  else
    cvsDirProgressLabel->setText( QString("%1/%2 (ignored: %3)").arg(dirsScanned).arg(dirsKnown).arg(dirsIgnored) );
}
void ClientWindow::slotFilesScanned(int filesKnown, int filesIgnored)
{
  if(filesKnown == 0 && filesIgnored == 0)
    cvsFileProgressLabel->setText("");
  else
    cvsFileProgressLabel->setText( QString("%1 (ignored: %2)").arg(filesKnown).arg(filesIgnored) );
}
void ClientWindow::slotFileStats(int filesResolved, int filesPendingStat)
{
  if(filesResolved == 0 && filesPendingStat == 0)
    statProgressLabel->setText("");
  else
    statProgressLabel->setText( QString("%1/%2").arg(filesResolved).arg(filesPendingStat) );
}
void ClientWindow::slotFilesCopied(int filesCopied, int filesPendingCopy, int fileErrors)
{
  if(filesCopied == 0 && filesPendingCopy == 0 && fileErrors == 0)
    copyProgressLabel->setText("");
  else if(fileErrors == 0)
    copyProgressLabel->setText( QString("%1/%2").arg(filesCopied).arg(filesPendingCopy) );
  else
    copyProgressLabel->setText( QString("%1/%2 (errors: %3)").arg(filesCopied).arg(filesPendingCopy).arg(fileErrors) );
}

void ClientWindow::slotFilesDeleted(int filesDeleted)
{
  if(filesDeleted == 0)
    deletedLabel->setText("");
  else
    deletedLabel->setText(QString("%1").arg(filesDeleted));
}

void ClientWindow::slotSyncStateChanged(int state)
{
  IconState iconState = e_Disconnected;
  bool syncButtonChecked = true;
  bool syncButtonEnabled = true;
  bool settingsButtonEnabled = true;
  bool branchSelectionEnabled = true;

  switch(state)
  {
  case SyncSystem::e_Unconnected:
    iconState = e_Disconnected;
    syncButtonChecked = false ;
    syncButtonEnabled = false;
    settingsButtonEnabled = true;
    branchSelectionEnabled = true;
    break;
  case SyncSystem::e_Idle:
    iconState = e_Main;
    syncButtonChecked = false;
    syncButtonEnabled = true;
    settingsButtonEnabled = true;
    branchSelectionEnabled = true;
    break;
  case SyncSystem::e_Syncing:
    iconState = e_Syncing;
    syncButtonChecked = true;
    syncButtonEnabled = true;
    settingsButtonEnabled = false;
    branchSelectionEnabled = false;
    break;
  case SyncSystem::e_NodeWatching:
    iconState = e_NodeWatching;
    syncButtonChecked = true;
    syncButtonEnabled = true;
    settingsButtonEnabled = false;
    branchSelectionEnabled = false;
    break;
  }
  setIcon(iconState); 
  syncButton->setChecked(syncButtonChecked);
  syncButton->setEnabled(syncButtonEnabled);
  settingsButton->setEnabled(settingsButtonEnabled);
  branchSelector->setEnabled(branchSelectionEnabled);
}
void ClientWindow::slotFileAction(QString fileName, QDateTime mTime, bool deleteFile)
{
  if(deleteFile)
  {
    fFilesTableDialog->AddFile(fileName, mTime, deleteFile, true);
    qInformation() << "[DeleteFile] " << fileName << " " << mTime;
  }
  else
    qInformation() << "[SyncFile] " << fileName << " " << mTime;
}

void ClientWindow::slotFileStatus(QString fileName, QDateTime mTime, bool success)
{
  qDebug() << "[SyncFile] " << fileName << " success: " << success;
  fFilesTableDialog->AddFile(fileName, mTime, false, success);
}

void ClientWindow::on_syncButton_clicked()
{
  if(syncButton->isChecked())
  {
      emit signalStartSync();
  }
  else
  {
      emit signalStopSync();
  }

  qDebug() << "[ClientWindow.Debug] ClientWindow::on_syncButton_clicked... done";
}

void ClientWindow::slotVersionMismatch()
{
  QMessageBox::critical( this, "QuickSync client", "Version mismatch. The server has another version than the client. Exiting client" );
  exit( 1 );
}

void ClientWindow::slotUnknownPacket()
{
  QMessageBox::critical( this, "QuickSync client", "Received an unknown packet, you might need to update your server and client. Exiting client" );
  exit( 1 );
}
void ClientWindow::on_branchSelector_currentIndexChanged()
{
  if(fSaveBranch)
  {
    QSettings settings;
    settings.setValue("CurrentlySelectedBranch", branchSelector->currentText());
  }
}

void ClientWindow::on_toolCopy_clicked()
{
  if(fFilesTableDialog == NULL)
  {
    fFilesTableDialog = new CopiedFilesDialog_c;
    connect(this, SIGNAL(closing()), fFilesTableDialog, SLOT(saveWindowPos()));
  }

  if(fFilesTableDialog->isVisible())
  {
    fFilesTableDialog->hide();
  }
  else
  {
    fFilesTableDialog->show();
  }
  activateWindow();
}

void ClientWindow::setIcon(IconState state)
{
  QIcon* icon = NULL; 
  switch(state)
  {
  case e_Main:
    icon = &fMainIcon;
    break;
  case e_Syncing:
    icon = &fSyncIcon;
    break;
  case e_Disconnected:
    icon = &fDisconnectIcon;
    break;
  case e_NodeWatching:
    icon = &fNodeWatchingIcon;
    break;
  }
  if(icon == NULL)
    return;

  //if(fSystemTrayIcon.isVisible())
  {
    fSystemTrayIcon.setIcon(*icon);
  }
  setWindowIcon(*icon);
}

