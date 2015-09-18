#include "PreCompile.h"
#include "filescanner.h"
#include "utils.h"
#include "syncrules.h"

FileScanner::FileScanner(const QString& rootPath, QSharedPointer<SyncRules> rules) :
ScannerBase(rootPath, rules)
, fRootDir(rootPath)
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
  QDir searchDir(search);
  QFileInfoList dirList = searchDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

  for (auto info : dirList)
  {
#define WRITE_DEBUG_LOG 0
#if WRITE_DEBUG_LOG == 1
    QFile tmp("filelist");
    tmp.open(QIODevice::ReadWrite | QIODevice::Append);
    QTextStream stream(&tmp);
#endif
    QString dir = joinPath(path, info.fileName());
    if (info.isDir())
    {
      //if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
      //{
      //  emit signalReparsePoint(dir);
      //}

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
    else
    {
      QString fileName = info.fileName();
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
  }
}
