#include "PreCompile.h"
#include "filesystemwatcher.h"
#include "utils.h"
#include "windows.h"
FileSystemWatcher::FileSystemWatcher() :
  fThreadRunning(false)
{
}

FileSystemWatcher::~FileSystemWatcher()
{

}

void FileSystemWatcher::run()
{
  fThreadRunning = true;
  //Open a handle to the directory
  wchar_t* dirToWatchW = static_cast<wchar_t*>(alloca((fDirToWatch.length() + 1) * sizeof(wchar_t)));
  HANDLE hDir = CreateFile(dirToWatchW,
    FILE_LIST_DIRECTORY, 
    FILE_SHARE_READ | FILE_SHARE_WRITE ,//| FILE_SHARE_DELETE, <-- removing FILE_SHARE_DELETE prevents the user or someone else from renaming or deleting the watched directory. This is a good thing to prevent.
    NULL, //security attributes
    OPEN_EXISTING,
    FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, //<- the required priviliges for this flag are: SE_BACKUP_NAME and SE_RESTORE_NAME.  CPrivilegeEnabler takes care of that.
    NULL);

  if(hDir == INVALID_HANDLE_VALUE)
  {
    QString error = GetLastErrorStr();
    emit filewatchError(error);
    return;
  }
  //Create the changes buffer and the overlapped (windows async) buffer
  FILE_NOTIFY_INFORMATION* buffer = new FILE_NOTIFY_INFORMATION[1<<11];
  int buffersize = sizeof(FILE_NOTIFY_INFORMATION) * (1<<11);
  OVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(overlapped));

  //Create a IO port handle so we can wait for the async data from ReadDirectoryChangesW
  HANDLE hPort = CreateIoCompletionPort(hDir, NULL, (ULONG_PTR)&buffer, 0);
  if(hPort == INVALID_HANDLE_VALUE)
  {
    QString error = GetLastErrorStr();
    emit filewatchError(error);
    delete[] buffer;
    return;
  }

  DWORD read = 0;

  //Start the async watch for changes
  ReadDirectoryChangesW(hDir, buffer, buffersize, true,
    FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE,
    &read, &overlapped, NULL);


  while(fThreadRunning)
  {
    PFILE_NOTIFY_INFORMATION data;
    LPOVERLAPPED async;
    //Wait for data from 
    if(GetQueuedCompletionStatus(hPort,&read, (PULONG_PTR)&data, &async, 100))
    {
      PFILE_NOTIFY_INFORMATION info = buffer;
      bool loop = true;
      while(loop)
      {
        loop = info->NextEntryOffset != 0;
        QString filename(QString::fromUtf16((const ushort*)info->FileName, info->FileNameLength/2));
        //To make this similar to what the filescanner produces, i prepend ./ and replace all '\' with '/'
        filename = joinPath(fRelativeDir, filename);
        filename = filename.replace('\\', '/');
        switch(info->Action)
        {
        case FILE_ACTION_ADDED:
          {
            emit fileAdded(filename);
            //qDebug() << "[FileSystemWatcher.Action] FILE_ACTION_ADDED " << filename;
            break;
          }
        case FILE_ACTION_REMOVED:
          {
            emit fileDeleted(filename);
            //qDebug() << "[FileSystemWatcher.Action] FILE_ACTION_REMOVED " << filename;
            break;
          }
        case FILE_ACTION_MODIFIED:
          {
            emit fileChanged(filename);
            //qDebug() << "[FileSystemWatcher.Action] FILE_ACTION_MODIFIED " << filename;
            break;
          }
        case FILE_ACTION_RENAMED_OLD_NAME:
          {
            //qDebug() << "[FileSystemWatcher.Action] FILE_ACTION_RENAMED_OLD_NAME " << filename;
            fOldFilename = filename;
            break;
          }
        case FILE_ACTION_RENAMED_NEW_NAME:
          {
            //qDebug() << "[FileSystemWatcher.Action] FILE_ACTION_RENAMED_NEW_NAME " << filename;
            emit fileRenamed(fOldFilename, filename);
            break;
          }
        default:
          qWarning() << "[FileSystemWatcher.Action] Unknown action " << info->Action;
          break;
        }
        info = (PFILE_NOTIFY_INFORMATION)((LPBYTE)info + info->NextEntryOffset);
      }
      //rewatch the directory
      memset(&overlapped, 0, sizeof(OVERLAPPED));
      ReadDirectoryChangesW(hDir, buffer, buffersize, true,
        FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE,
        &read, &overlapped, NULL);

    }
    else
    {
      if(GetLastError() != WAIT_TIMEOUT)
      {
        //rewatch directory so the other side can determine when things calm down
        ReadDirectoryChangesW(hDir, buffer, buffersize, true,
          FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE,
          &read, &overlapped, NULL);

        QString error = GetLastErrorStr();
        emit filewatchLostSync(error);
      }
    }
  }
  delete[] buffer;
  CloseHandle(hPort);
  CloseHandle(hDir);
}

QString FileSystemWatcher::GetLastErrorStr()
{
  LPVOID errstr; 
  DWORD err = GetLastError(); 
  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
    NULL, 
    err, 
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
    (LPTSTR)&errstr, 
    0, 
    NULL );
  QString ret = QString::fromLocal8Bit((const char*)errstr);
  LocalFree(errstr); 
  return ret;
}
