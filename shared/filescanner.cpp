#include "PreCompile.h"
#include "filescanner.h"
#include "utils.h"
#include "syncrules.h"

FileScanner::FileScanner(const QString& rootPath, QSharedPointer<SyncRules> rules)
  : ScannerBase(rootPath, rules)
  , m_RootDir(rootPath)
  , m_WriteLog(false)
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

  m_UnscannedDirs.push_back(DirRules(".", rules));
  fDirCount++;
}

bool FileScanner::scanStep()
{
  if (!m_UnscannedDirs.isEmpty())
  {
    DirRules dir = m_UnscannedDirs.front();
    m_UnscannedDirs.pop_front();
    scanDir(dir.m_Path, dir.m_Rules);
    return true;
  }
  return false;
}

void FileScanner::scanDir(const QString& path, QSharedPointer<SyncRules> rules)
{
  fDirNumber++;
  QString search = joinPath(m_RootDir, path);
  QDir searchDir(search);
  QFileInfoList dirList = searchDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

  QFile tmp(joinPath(GetDataDir(), "filescanner.log"));
  QTextStream stream(&tmp);
  if (m_WriteLog)
  {
    tmp.open(QIODevice::ReadWrite | QIODevice::Append);
  }
  for (auto info : dirList)
  {
    QString dir = joinPath(path, info.fileName());
    if (info.isDir())
    {
      //if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
      //{
      //  emit signalReparsePoint(dir);
      //}

      QString rulePath = joinPath(m_RootDir, dir, "syncrules.xml");
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
        if(m_WriteLog)
        {
          stream << dir << endl;
        }
        m_UnscannedDirs << DirRules(dir, rules);
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
        if(m_WriteLog)
        {
          stream << fileName << " " << fileinfo.size() / 1024 << " Kb" << endl;
        }

      }
      else
      {
        fFileIgnored++;
      }
    }
  }
}

NewFileScanner::NewFileScanner(const QString& rootPathDirRules, ProcessFile fileProcesser)
  : m_FileProcessor(fileProcesser)
  , m_WriteLog(true)
{

}

void NewFileScanner::ScanDir(const QString& path)
{
  QDir searchDir(path);

  QFileInfoList fileList = searchDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
  for (auto info : fileList)
  {
    QString entryName = info.absolutePath();
    if (info.isDir())
    {
      if (m_FileProcessor(entryName))
      {
        ScanDir(entryName);
      }
    }
    else
    {
      if (m_FileProcessor(entryName))
      {
        SignalFile(entryName);
      }
    }
  }

}