#include "PreCompile.h"
#include "filescanner.h"
#include "utils.h"
#include "syncrules.h"

FileScanner::FileScanner(const QString& rootPath, QSharedPointer<SyncRules> rules) :
ScannerBase(rootPath, rules)
#if !defined( USE_QDIR)
, fRootDir(rootPath)
#else
,fScanner(rootPath)
#endif
{
  QString branchRule(joinPath(rootPath, "syncrules.xml"));
  if (QFile::exists(branchRule))
  {
    QSharedPointer<SyncRules> loadRules(new SyncRules);
    if (loadRules->loadXmlRules(branchRule))
    {
      //there is a valid syncrules file in the root of the branch, we will never hit the default rules so just replace them with the rules we just loaded
      rules = loadRules;
    }
  }

  fUnscannedDirs.push_back(DirRules(".", rules));
  fDirCount++;
}

#if !defined( USE_QDIR )
bool FileScanner::scanStep()
{
  if (!fUnscannedDirs.isEmpty())
  {
    DirRules dir = fUnscannedDirs.front();
    fUnscannedDirs.pop_front();
    scanDir(dir.m_Path, dir.m_Rules);
    return true;
  }
  return false;
}

void FileScanner::scanDir(const QString& path, QSharedPointer<SyncRules> rules)
{
  fDirNumber++;
  QString search = joinPath(fRootDir, path);
  search = joinPath(search, "*");
  WIN32_FIND_DATA fileData;
  wchar_t* searchW = static_cast<wchar_t*>(alloca((search.length() + 1) * sizeof(wchar_t)));
  search.toWCharArray(searchW);
  HANDLE dirList = FindFirstFile(searchW, &fileData);

#define WRITE_DEBUG_LOG 0
#if WRITE_DEBUG_LOG == 1
  QFile tmp("filelist");
  tmp.open(QIODevice::ReadWrite | QIODevice::Append);
  QTextStream stream(&tmp);
#endif
  if (dirList != INVALID_HANDLE_VALUE)
  {
    do
    {
      QString fileName = QString::fromWCharArray(fileData.cFileName);
      QString dir = joinPath(path, fileName);
      if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
      {
        if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
        {
          emit signalReparsePoint(dir);
        }


        if (fileName.compare(".") != 0 && fileName.compare("..") != 0)
        {
          QString rulePath = joinPath(fRootDir, dir, "syncrules.xml");
          if (QFile::exists(rulePath))
          {
            QSharedPointer<SyncRules> loadRules(new SyncRules);
            if (loadRules->loadXmlRules(rulePath))
            {
              rules = loadRules;
              emit signalSyncRuleFile(dir, loadRules);
            }
          }
          SyncRuleFlags_e flags;
          if (rules->CheckFile(dir.toLower(), flags))
          {
#if WRITE_DEBUG_LOG == 1
            stream << dirStr << endl;
#endif
            fUnscannedDirs << DirRules(dir, rules);
            fDirCount++;
          }
          else
          {
            fDirIgnored++;
          }
        }
      }
      else
      {
        if (fileName.endsWith("syncrules.xml"))
          continue;

        SyncRuleFlags_e flags;
        if (rules->CheckFile(fileName.toLower(), flags))
        {
          QFileInfo fileinfo(joinPath(fRootPath, fileName));
          FileInfo info;
          info.mtime = fileinfo.lastModified();
          info.binary = false;
          info.executable = false;
          if ((flags&e_Binary) == e_Binary)
            info.binary = true;
          if ((flags&e_Executable) == e_Executable)
            info.executable = true;

          fAllFiles[joinPath(path, fileName)] = info;
          fFileNumber++;
          fFileCount++;
#if WRITE_DEBUG_LOG == 1
          stream << fileStr << " " << fileinfo.size() / 1024 << " Kb" << endl;
#endif
        }
        else
        {
          fFileIgnored++;
        }
      }
    } while (FindNextFile(dirList, &fileData));
    FindClose(dirList);
  }
  else
  {
    qCritical() << "[FileScanner] ERROR cant scan dir: " << search;
  }
}
#else
#error "Not 100% supported"
bool FileScanner::scanStep()
{
  if(!fUnscannedDirs.isEmpty())
  {
    QString dir = fUnscannedDirs[0];
    fUnscannedDirs.pop_front();
    scanDir( dir );
    return true;
  }
  return false;
}

void FileScanner::scanDir(const QString &path)
{
  fDirNumber++;
  QDir dir(path);

  QStringList dirList = dir.entryList(QDir::AllDirs|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System);
  if(!dirList.isEmpty())
  {
    fDirCount += dirList.size();
    QString dirStr;
    foreach(dirStr, dirList)
    {
      dirStr = joinPath(path, dirStr);
      fUnscannedDirs << dirStr;
    }
  }

  QFileInfoList fileList = dir.entryInfoList(QDir::Files|QDir::Hidden|QDir::System);
  QFileInfo fileInfo;
  foreach(fileInfo, fileList)
  {
    FileInfo info;
    info.mtime = fileInfo.lastModified();
    info.binary = false;
    info.executable = fileInfo.isExecutable();
    fAllFiles[fileInfo.absoluteFilePath()] = info;
    fFileNumber++;
    fFileCount++;
  }
}

#endif
