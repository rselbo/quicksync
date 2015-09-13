#include "PreCompile.h"
#include "syncsystem.h"
#include "clientsettings.h"
#include "utils.h"
#include "filesystemwatcher.h"

//Wait for 5 seconds of disk inactivity before restarting the full sync
static quint32 s_ResyncTimeout = 5000;

SyncSystem::SyncSystem(QSharedPointer<SyncRules> syncRules) :
  m_Connection(NULL),
  m_SyncState(e_Unconnected),
  m_SyncRules(syncRules),
  m_ReconnectTimer(NULL),
  m_Scanner(NULL),
  m_RestartSyncOnReconnect(false),
  m_BytesInTransit(0)
{
  resetStats();
}

SyncSystem::~SyncSystem()
{
  Q_ASSERT(m_FileSystemWatchers.empty());
  Q_ASSERT(m_UnresolvedFiles.empty());
  Q_ASSERT(m_Scanner == NULL);
  Q_ASSERT(m_NameToInfo.empty());
  Q_ASSERT(m_Files.empty());
}

//////////////////////////////////////////////////////////////////////////
/// Quicksync settings have changed, reconnect to the server
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotSettingsChanged()
{
  reconnect();
}

//////////////////////////////////////////////////////////////////////////
/// Starts a full sync
/// 
/// It will pull the current settings into local variables and do some 
/// basic sanity checks on them before selecting the scanner and starting
/// the scan dir timer.
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotStartSync()
{
  if(!m_Connection || m_SyncState == e_Unconnected)
    return;
  resetStats();
  setSyncState(e_Syncing);

  //Get the name of the current sync target
  QString currentBranch = m_Settings.value("CurrentlySelectedBranch").toString();

  if(currentBranch.isEmpty())
  {
    emit signalError("Information", "You must configure and select a branch before attempting to sync");
    setSyncState(e_Idle);
    return;
  }

  //Read out source and destination paths for the sync target
  m_Settings.beginGroup(ClientSettings::branchesStr);
  m_Settings.beginGroup(currentBranch);
  m_CurrentSourcePath = m_Settings.value(ClientSettings::srcPathStr).toString();
  m_CurrentDestinationPath = m_Settings.value(ClientSettings::dstPathStr).toString();
  m_Settings.endGroup();
  m_Settings.endGroup();

  //Sanity check source and destination
  if(m_CurrentSourcePath.isEmpty())
  {
    emit signalError("Information", QString("Missing source path for branch ").arg(currentBranch));
    setSyncState(e_Idle);
    return;
  }
  if(m_CurrentDestinationPath.isEmpty())
  {
    emit signalError("Information", QString("Missing destination path for branch ").arg(currentBranch));
    setSyncState(e_Idle);
    return;
  }

  //Get the scanner and check that it is valid
  m_Scanner = ScannerBase::selectScannerForFolder( m_CurrentSourcePath, m_SyncRules );
  if(m_Scanner == NULL)
  {
    emit signalError("Information", QString("Could not find scanner for source directory: ").arg(currentBranch));
    setSyncState(e_Idle);
    return;
  }

  //Connect to the reparsepoint signal so we can nodewatch in other disks and add current path to nodewatching
  connect(m_Scanner, SIGNAL(signalReparsePoint(const QString&)), SLOT(slotStartNodeWatching(const QString&)));
  connect(m_Scanner, SIGNAL(signalSyncRuleFile(const QString&, QSharedPointer<SyncRules>)), SLOT(slotSyncRuleFile(const QString&, QSharedPointer<SyncRules>)));
  slotStartNodeWatching(".");

  m_Connection->sendTargetDirectory(m_CurrentDestinationPath);

  m_ScanDirTimer->start();
}

//////////////////////////////////////////////////////////////////////////
/// This function is called when a full scan is completed.
/// 
/// A full scan is completed if the user stopped the scan, if we scanned 
/// through the directory structure or we lost connection to the server.
/// It will delete and reset the scanner, stop the scan dir timer and 
/// update the sync system state.
//////////////////////////////////////////////////////////////////////////
void SyncSystem::scanComplete()
{
  delete m_Scanner;
  m_Scanner = NULL;
  updateSyncState();
  m_ScanDirTimer->stop();
}

//////////////////////////////////////////////////////////////////////////
/// Scans all the files in one dir
/// 
/// If the SyncSystem is in the correct state this function will scan all 
/// files in the current directory. It will emit the signalDirsScanned, 
/// signalFilesScanned and signalFileStats for each directory scanned, 
/// or emit signalError if anything went wrong.
/// It is called by the m_ScanDirTimer with 0 ms delay to allow the event
/// loop to process network traffic as well as scan.
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotScanDir()
{
  if(m_Scanner && m_Connection)
  {
    if(m_Scanner->scanStep())
    {
      //Loop over each file in this directory
      for( ScannerBase::const_iterator fileIterator = m_Scanner->allFiles().begin(); fileIterator != m_Scanner->allFiles().end(); fileIterator++ )
      {
        QString fileName(fileIterator.key());
        QFileInfo fileInfo(joinPath(m_CurrentSourcePath,fileName));
        if(!fileInfo.exists())
          continue; //File has been locally deleted since the sync was pressed

        m_UnresolvedFiles[fileName] = fileIterator.value();
        m_Connection->sendStatFileReq(fileName);
        m_FilesPendingStat++;
        //Add to the known files list, this is used to find files to delete
        m_Files.insert(fileName);
      }
      m_DirsFinished++;
      m_DirsKnown = m_Scanner->dirCount();
      m_DirsIgnored = m_Scanner->dirIgnored();
      emit signalDirsScanned(m_DirsFinished, m_DirsKnown, m_DirsIgnored);

      m_FilesKnown = m_Scanner->fileNumber();
      m_FilesIgnored = m_Scanner->fileIgnored();
      emit signalFilesScanned(m_FilesKnown, m_FilesIgnored);
      emit signalFileStats(m_FilesResolved, m_FilesPendingStat);
      m_Scanner->clearAllFiles();
    }
    else
    {
      scanComplete();
    }
  }
}

//////////////////////////////////////////////////////////////////////////
/// Do a full nodewatching stop and full scan and nodewatching start
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotReSync()
{
  slotStopSync();
  slotStartSync();
}

//////////////////////////////////////////////////////////////////////////
/// Do a full nodewatching and scan stop.
/// 
/// Returns the sync system state to idle.
/// Loops over all FileSystemWatchers, stops, disconnects all signals and
/// waits for the thread to stop.
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotStopSync()
{
  setSyncState(e_Idle);
  stopFullSync();
  stopNodeWatching();
}

void SyncSystem::slotLostSync(QString error)
{
  qInformation() << "FilesystemWatcher lost sync " << error;
  m_LostSyncTimer->start(s_ResyncTimeout);
  updateSyncState();
}

//////////////////////////////////////////////////////////////////////////
/// Add a FileSystemWatcher to the directory
/// 
/// Creates a FileSystemWatcher, connects all the signals, adds it to the
/// list of watchers and starts it.
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotStartNodeWatching(const QString& dir)
{
  QString watchDir = joinPath(m_CurrentSourcePath, dir);
  qDebug() << "[SyncSystem.Debug] slotStartNodeWatching in dir " << watchDir;

  FileSystemWatcher* watcher = new FileSystemWatcher();
  connect(watcher, SIGNAL(fileAdded(QString)), SLOT(slotFileAdded(QString)));
  connect(watcher, SIGNAL(fileDeleted(QString)), SLOT(slotFileDeleted(QString)));
  connect(watcher, SIGNAL(fileChanged(QString)), SLOT(slotFileChanged(QString)));
  connect(watcher, SIGNAL(fileRenamed(QString,QString)), SLOT(slotFileRenamed(QString,QString)));
  connect(watcher, SIGNAL(filewatchError(QString)), SLOT(slotFilewatchError(QString)));
  connect(watcher, SIGNAL(filewatchLostSync(QString)), SLOT(slotLostSync()));
  m_FileSystemWatchers.append(watcher);
  watcher->setWatchDir(watchDir);
  watcher->setRelativeDir(dir);
  watcher->start();
}

void SyncSystem::slotSyncRuleFile(const QString& path, QSharedPointer<SyncRules> rules)
{
  m_PathRules[path] = rules;
}
//////////////////////////////////////////////////////////////////////////
/// A FileSystemWatcher notification when a file is added
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotFileAdded(QString file)
{
  if(m_LostSyncTimer->isActive())
  {
    //Force the timer to run for 5 more seconds
    m_LostSyncTimer->setInterval(s_ResyncTimeout);
  }

  QSharedPointer<SyncRules> rules = GetSyncRulesForPath(file);

  SyncRuleFlags_e eFlags;
  if(rules->CheckFileAndPath(file.toLower(), eFlags))
  {
    if(checkForRescan(file))
    {
      return;
    }
    bool binary = ((eFlags & e_Binary) == e_Binary);
    bool executable = ((eFlags & e_Executable) == e_Executable);
    addTodo(file, binary, executable, false);
  }
}

//////////////////////////////////////////////////////////////////////////
/// A FileSystemWatcher notification when a file is deleted
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotFileDeleted(QString file)
{
  if(m_LostSyncTimer->isActive())
  {
    //Force the timer to run for 5 more seconds
    m_LostSyncTimer->setInterval(s_ResyncTimeout);
  }

  QSharedPointer<SyncRules> rules = GetSyncRulesForPath(file);

  SyncRuleFlags_e eFlags;
  if(rules->CheckFileAndPath(file.toLower(), eFlags))
  {
    bool binary = ((eFlags & e_Binary) == e_Binary);
    bool executable = ((eFlags & e_Executable) == e_Executable);
    addTodo(file, binary, executable, true);
  }
}

//////////////////////////////////////////////////////////////////////////
/// A FileSystemWatcher notification when a file is changed
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotFileChanged(QString file)
{
  if(m_LostSyncTimer->isActive())
  {
    //Force the timer to run for 5 more seconds
    m_LostSyncTimer->setInterval(s_ResyncTimeout);
  }

  QSharedPointer<SyncRules> rules = GetSyncRulesForPath(file);

  SyncRuleFlags_e eFlags;
  if(rules->CheckFileAndPath(file.toLower(), eFlags))
  {
    bool binary = ((eFlags & e_Binary) == e_Binary);
    bool executable = ((eFlags & e_Executable) == e_Executable);
    addTodo(file, binary, executable, false);
  }
}

//////////////////////////////////////////////////////////////////////////
/// A FileSystemWatcher notification when a file is renamed
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotFileRenamed(QString oldName, QString newName)
{
  if(m_LostSyncTimer->isActive())
  {
    //Force the timer to run for 5 more seconds
    m_LostSyncTimer->setInterval(s_ResyncTimeout);
  }

  QSharedPointer<SyncRules> oldPathRules = GetSyncRulesForPath(oldName);
  QSharedPointer<SyncRules> newPathRules = GetSyncRulesForPath(newName);

  SyncRuleFlags_e eOldFlags;
  SyncRuleFlags_e eFlags;
  if(oldPathRules->CheckFileAndPath(oldName.toLower(), eOldFlags) )
  {
    if(checkForRescan(newName))
    {
      return;
    }

    bool oldBinary = ((eOldFlags & e_Binary) == e_Binary);
    bool oldExecutable = ((eOldFlags & e_Executable) == e_Executable);

    //old name should be synced
    if(newPathRules->CheckFileAndPath(newName.toLower(), eFlags))
    {
      bool binary = ((eFlags & e_Binary) == e_Binary);
      bool executable = ((eFlags & e_Executable) == e_Executable);
      //new file should also be synced, delete the old sync the new (to avoid the need for new server protocol)
      addTodo(oldName, oldBinary, oldExecutable, true);
      addTodo(newName, binary, executable, false);
    }
    else
    {
      //old name was in sync, new is not. delete 
      addTodo(oldName, oldBinary, oldExecutable, true);
    }
  }
  else if(newPathRules->CheckFileAndPath(newName.toLower(), eFlags))
  {
    if(checkForRescan(newName))
    {
      return;
    }

    bool binary = ((eFlags & e_Binary) == e_Binary);
    bool executable = ((eFlags & e_Executable) == e_Executable);

    //Old file was not in sync, but the new is
    addTodo(newName, binary, executable, false);
  }
}

//////////////////////////////////////////////////////////////////////////
/// A FileSystemWatcher notification about some error
//////////////////////////////////////////////////////////////////////////
void SyncSystem::slotFilewatchError(QString file)
{
  qDebug() << "slotFilewatchError " << file;
}

//////////////////////////////////////////////////////////////////////////
/// Threads run function
/// 
/// Creates the timers and connects the signals.
/// Does the initial connection to the server before entering the event 
/// loop.
//////////////////////////////////////////////////////////////////////////
void SyncSystem::started()
{
  m_ReconnectTimer = new QTimer;
  m_ReconnectTimer->setSingleShot(true);
  connect(m_ReconnectTimer, SIGNAL(timeout()), this, SLOT(reconnectTimer()));
  m_ScanDirTimer = new QTimer;
  connect(m_ScanDirTimer, SIGNAL(timeout()), this, SLOT(slotScanDir()));
  m_SyncUpdateTimer = new QTimer;
  connect(m_SyncUpdateTimer, SIGNAL(timeout()), this, SLOT(slotSyncUpdate()));
  m_LostSyncTimer = new QTimer;
  m_LostSyncTimer->setSingleShot(true);
  connect(m_LostSyncTimer, SIGNAL(timeout()), this, SLOT(slotReSync()));
  //Initial connect
  reconnect(0);
}

void SyncSystem::finished()
{
  //cleanup after the event loop is finished
  stopFullSync();
  stopNodeWatching();
  if(m_Connection != NULL)
  {
    m_Connection->fSocket->disconnect( this );
    m_Connection->disconnect( this );
    m_Connection->fSocket->close();
    m_Connection->deleteLater();
    m_Connection = NULL;
  }
  delete m_ReconnectTimer;
  m_ReconnectTimer = NULL;
  delete m_ScanDirTimer;
  m_ScanDirTimer = NULL;
  delete m_SyncUpdateTimer;
  m_SyncUpdateTimer = NULL;
  delete m_LostSyncTimer;
  m_LostSyncTimer = NULL;
}

//////////////////////////////////////////////////////////////////////////
/// Send a file to the server, replacing \r\n with \n for text files.
/// 
/// Creates the timers and connects the signals.
/// Does the initial connection to the server before entering the event 
/// loop.
//////////////////////////////////////////////////////////////////////////
void SyncSystem::sendFile( const QString &filename, bool binary, bool executable )
{
  //Get the source and destination directory from the branch information
  if(m_CurrentSourcePath.isEmpty())
  {
//     QString infoString("Missing source path for branch");
//     QMessageBox::information(this, "Information", infoString, "Ok", "", "");
    return;
  }

  QString path = joinPath(m_CurrentSourcePath,filename);
  QFile file( path );

  if( file.open(QIODevice::ReadOnly) )
  {
    QFileInfo fileinfo( file );

    QByteArray data = file.readAll();
    if( !binary )
    {
      // 0d 0a -> 0a
      char *src = data.data();
      char *dst = data.data();
      char *srcend = src + data.size();
      //int state = 0;
      while( src != srcend )
      {
        if( *src != 0x0d )
        {
          *dst++ = *src;
        }
        src++;
      }
      data.resize( static_cast<int>(dst-data.data()) );
    }
    m_Connection->sendSendFile( filename, fileinfo.lastModified(), data, executable );
  }
  else
  {
    qWarning() << "[SyncSystem.sendFile] Could not open file " << filename;
    addTodo(filename, binary, executable, false, true);
  }
}

void SyncSystem::reconnect( int delay )
{
  qDebug() << "[SyncSystem.Debug] SyncSystem::reconnect" << delay;

  if( m_ReconnectTimer->isActive() )
  {
    qDebug() << "[SyncSystem.Info] connect timer is already active";
    return;
  }

  if( m_Connection != NULL )
  {
    stopFullSync();
    stopNodeWatching();
    m_BytesInTransit = 0;

    m_Connection->fSocket->disconnect( this );
    m_Connection->disconnect( this );
    m_Connection->fSocket->close();
    m_Connection->deleteLater();
    m_Connection = NULL;
    setSyncState(e_Unconnected);
  }

  m_ReconnectTimer->start( delay );
}

void SyncSystem::reconnectTimer()
{
  qDebug() << "[SyncSystem.Debug] SyncSystem::reconnectTimer";

  Q_ASSERT( m_Connection == NULL );

  //fWasConnected = false;
  m_Connection = new RemoteObjectConnection();
  connect( m_Connection->fSocket, SIGNAL(connected()), SLOT(connected()) );
  connect( m_Connection->fSocket, SIGNAL(disconnected()), SLOT(disconnected()) );
  connect( m_Connection->fSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(error(QAbstractSocket::SocketError)) );
  connect( m_Connection, SIGNAL(recvStatFileReply(const QString &, const QDateTime &)), SLOT(recvStatFileReply(const QString &, const QDateTime &)) );
  connect( m_Connection, SIGNAL(recvSendFileResult(const QString &, const QDateTime &, int)), SLOT(recvSendFileResult(const QString &, const QDateTime &, int)) );
  connect( m_Connection, SIGNAL(recvVersionMismatch()), SIGNAL(signalVersionMismatch()) );
  connect( m_Connection, SIGNAL(recvUnknownPacket()), SIGNAL(signalUnknownPacket()) );

  QString host = m_Settings.value("server/hostname").toString();
  qint16 port = static_cast<quint16>(m_Settings.value("server/port").toInt());

  qDebug() << "[SyncSystem.Debug] connecting to" << host << port;
  m_Connection->fSocket->connectToHost( host, port );
}

void SyncSystem::connected()
{
  setSyncState(e_Idle);
  m_BytesInTransit = 0;

  if(m_RestartSyncOnReconnect)
  {
    m_RestartSyncOnReconnect = false;
    QTimer::singleShot(1000, this, SLOT(slotStartSync()));
  }
}

void SyncSystem::disconnected()
{
  m_RestartSyncOnReconnect = m_SyncState == e_NodeWatching || m_SyncState == e_Syncing;
  reconnect();
}

void SyncSystem::error(QAbstractSocket::SocketError)
{
  m_RestartSyncOnReconnect = m_SyncState == e_NodeWatching || m_SyncState == e_Syncing;
  reconnect();
}

void SyncSystem::recvStatFileReply(const QString &filename, const QDateTime &mtime)
{
  //sync has been stopped, ignore what the server is sending
  if(m_SyncState == e_Idle)
    return;
  //find the unresolved file in the map
  QMap<QString,ScannerBase::FileInfo>::iterator iUnresolved = m_UnresolvedFiles.find( filename );
  if( iUnresolved == m_UnresolvedFiles.end() )
  {
    qCritical() << "[SyncSystem.Error]  *** Internal error: " << filename << " missing from unresolved list";
    return;
  }

  //check if the mtimes are similar (cant do exact comparison due to different OSes having somewhat different resolutions)
  if(!mtime.isValid() ||  abs(iUnresolved.value().mtime.secsTo(mtime)) > 1 ) 
  {
    addTodo(filename, iUnresolved.value().binary, iUnresolved.value().executable, false);
  }

  m_UnresolvedFiles.erase( iUnresolved );
  m_FilesResolved++;
  emit signalFileStats(m_FilesResolved, m_FilesPendingStat);
  updateSyncState();
}

void SyncSystem::recvSendFileResult(const QString &filename, const QDateTime &mtime, int result)
{
  //sync has been stopped, ignore what the server is sending
  if(m_SyncState == e_Idle)
    return;

  if( !result )
  {
    m_FileErrors++;
    qWarning() << "[SyncSystem.sendFile] copy of file failed: " << filename;
    SyncRuleFlags_e flags = e_NoFlags;
    if(GetSyncRulesForPath(filename)->CheckFile(filename.toLower(), flags))
    {
      bool binary = flags == e_Binary || flags == e_BinaryExecutable;
      addTodo(filename, binary, false, false, true);
      emit signalFileStatus(filename, mtime, false);
    }
  }
  else
  {
    m_FilesCopied++;
    QMap<QString, FileTodo>::iterator erase = m_NameToInfo.find(filename);
    Q_ASSERT(erase != m_NameToInfo.end());
    if(erase != m_NameToInfo.end())
    {
      m_BytesInTransit -= erase.value().m_Size;
      m_NameToInfo.erase(erase);
      emit signalFileStatus(filename, mtime, true);
    }
  }
  emit signalFilesCopied(m_FilesCopied, m_FilesPendingCopy, m_FileErrors);
  updateSyncState();
}


void SyncSystem::stopNodeWatching()
{
  //for each watcher, disconnect signals wait for the thread to exit then delete the watcher
  FileSystemWatcher* watcher = NULL;
  foreach(watcher, m_FileSystemWatchers)
  {
    watcher->stop();
    watcher->disconnect(this);

    while(!watcher->isFinished())
      watcher->wait(100);
    delete watcher;
  }

  //Empty the list of deleted pointers
  m_FileSystemWatchers.clear();

  m_NameToInfo.clear();
  m_Files.clear();
}

void SyncSystem::stopFullSync()
{
  m_ScanDirTimer->stop();
  m_SyncUpdateTimer->stop();
  delete m_Scanner;
  m_Scanner = NULL;
}
//////////////////////////////////////////////////////////////////////////
/// Reset all stats and emit the signals
//////////////////////////////////////////////////////////////////////////
void SyncSystem::setSyncState(SyncSystemState state)
{
  if(m_SyncState == state)
    return;

  switch(state)
  {
  case e_Idle:
  case e_Unconnected:
    resetStats();
    break;
  default:
    //do nothing
    break;
  }
  m_SyncState = state;
  emit signalStateChanged(m_SyncState);
}

void SyncSystem::updateSyncState()
{
  if(m_Connection)
  {
    if(m_Scanner || m_FilesPendingCopy > m_FilesCopied || m_FilesPendingStat > m_FilesResolved || m_DirsFinished > m_DirsKnown || !m_NameToInfo.empty() || m_LostSyncTimer->isActive())
      setSyncState(e_Syncing);
    else
      setSyncState(e_NodeWatching);
  }
}
//////////////////////////////////////////////////////////////////////////
/// Reset all stats and emit the signals
//////////////////////////////////////////////////////////////////////////
void SyncSystem::resetStats()
{
  m_DirsFinished = 0;
  m_DirsKnown = 0;
  m_DirsIgnored = 0;
  m_FilesKnown = 0;
  m_FilesIgnored = 0;
  m_FilesResolved = 0;
  m_FilesPendingStat = 0;
  m_FilesCopied = 0;
  m_FilesPendingCopy = 0;
  m_FileErrors = 0;
  m_FilesDeleted = 0;

  emit signalDirsScanned(m_DirsFinished, m_DirsKnown, m_DirsIgnored);
  emit signalFilesScanned(m_FilesKnown, m_FilesIgnored);
  emit signalFileStats(m_FilesResolved, m_FilesPendingStat);
  emit signalFilesCopied(m_FilesCopied, m_FilesPendingCopy, m_FileErrors);
  emit signalFilesDeleted(m_FilesDeleted);
}

//! Time in ms to wait after getting a file notification
const int SYNCDELAY = 500; 
void SyncSystem::addTodo(const QString& fileName, bool binary, bool executable, bool deletefile, bool retry)
{
  QFileInfo fileinfo(joinPath(m_CurrentSourcePath, fileName));
  if(fileinfo.isDir()) //we dont care about dir stuff yet
    return;

  qInformation() << "[SyncSystem.addTodo] addTodo " << fileName << " " << binary << " " << executable << " " << deletefile;
  QTime syncTime = QTime::currentTime().addMSecs(SYNCDELAY);
  QMap<QString, FileTodo>::iterator i = m_NameToInfo.find(fileName);
  QDateTime lastModified;
  if(fileinfo.exists())
    lastModified = fileinfo.lastModified();
  else
    lastModified = QDateTime::currentDateTime();
  if(i == m_NameToInfo.end())
  {
    //no info about this file yet
    m_NameToInfo[fileName] = FileTodo(fileName, binary, executable, syncTime, deletefile, lastModified, fileinfo.size());
    if(!deletefile)
    {
      emit signalFilesCopied(m_FilesCopied, ++m_FilesPendingCopy, m_FileErrors);
      m_Files.insert(fileName); //add new file to the name list
    }
    else
    {
      m_Files.erase(m_Files.find(fileName));
    }
  }
  else
  {
    if(!i.value().m_Started && !i.value().m_Delete && deletefile)
    {
      //This was added, then deleted (and it has not been sent to the server). No need to do anything
      m_NameToInfo.erase(i);
      emit signalFilesCopied(m_FilesCopied, --m_FilesPendingCopy, m_FileErrors);
    }
    else
    {
      if(i.value().m_Delete && !deletefile && !retry)
      {
        emit signalFilesCopied(m_FilesCopied, ++m_FilesPendingCopy, m_FileErrors);
      }
      //We already have info about this file
      i.value().m_Binary = binary;
      i.value().m_Executable = executable;
      i.value().m_Delete = deletefile;
      i.value().m_Time = syncTime;
      i.value().m_Mtime = lastModified;
      if(retry)
      {
        i.value().m_Retries++;
        i.value().m_Started = false;
      }
    }
  }

  updateSyncState();
  m_SyncUpdateTimer->start(100);
}

void SyncSystem::slotSyncUpdate()
{
  if(m_Scanner)
    return; //Dont start copying files until the stats are done

  static int maxSize = 1<<25; //32MB
  if(m_BytesInTransit >= maxSize)
    return; //Too much data in memory already, lets not add more

  QTime currentTime = QTime::currentTime();
  QMap<QString, FileTodo>::iterator todo = m_NameToInfo.begin();
  for(; todo != m_NameToInfo.end();)
  {
    if(currentTime >= todo.value().m_Time)
    {
      if(todo.value().m_Delete)
      {
        todo.value().m_Started = true;
        m_Connection->sendDeleteFile(todo.key());
        emit signalFileAction(todo.key(), todo.value().m_Mtime, true);
        todo = m_NameToInfo.erase(todo);

        m_FilesDeleted++;
        emit signalFilesDeleted(m_FilesDeleted);
      }
      else if(!todo.value().m_Started)
      {
        todo.value().m_Started = true;
        //sendFile can modify m_NameToInfo and cause a crash here, but im to lazy to handle it right now
        sendFile(todo.key(), todo.value().m_Binary, todo.value().m_Executable);
        emit signalFileAction(todo.key(), todo.value().m_Mtime, false);
        if(todo.value().m_Retries == 0)
        {
          m_BytesInTransit += todo.value().m_Size;
          if(m_BytesInTransit >= maxSize)
            break; //With this file we went above max in memory size, so break out of the loop 
        }
        ++todo;
      }
      else
      {
        ++todo;
      }
    }
    else
    {
      ++todo;
    }
  }
  if(m_NameToInfo.empty())
    m_SyncUpdateTimer->stop();

  updateSyncState();
}

bool SyncSystem::checkForRescan(const QString& name)
{
  QString path = joinPath(m_CurrentSourcePath, name);
  QFileInfo fileinfo(path);
  if(fileinfo.isDir())
  {
    //A directory was changed in a way we care about, restart with a full sync
    slotReSync();
    return true;
  }
  return false;
}

QSharedPointer<SyncRules> SyncSystem::GetSyncRulesForPath(const QString& path)
{
  QString searchPath(path);
  int pos = 0;
  while((pos = searchPath.lastIndexOf('/')) != -1)
  {
    searchPath = searchPath.left(pos);
    auto i = m_PathRules.find(searchPath);
    if(i != m_PathRules.end())
    {
      return i.value();
    }
  }
  return m_SyncRules;
}