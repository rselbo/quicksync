// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include "PreCompile.h"
#include "utils.h"

#ifdef WINDOWS
#include <shlobj.h>
#endif
//-----------------------------------------------------------------------------

QString joinPath( const QString &path1, const QString &path2 )
{
  if( path1.isEmpty() || (!path2.isEmpty() && path2[0]=='/') ) return path2;
  if( path2.isEmpty() ) return path1;
  bool endsWithSlash = path1[int(path1.length()-1)] == '/';
  bool startsWithSlash = path2[0] == '/';
  if( endsWithSlash && startsWithSlash ) return path1.right(path1.length()-1)+path2;
  if( !endsWithSlash && !startsWithSlash ) return path1+"/"+path2;
  return path1+path2;
}

QString joinPath( const QString &path1, const QString &path2, const QString &path3 )
{
  return joinPath( joinPath(path1, path2), path3 );
}


//custom message level
const QtMsgType QSInformationMsg = QtMsgType(QtFatalMsg+1);
QDebug qInformation() { return QDebug(QSInformationMsg); }
namespace Log
{
  enum LogLevel_e { e_Debug, e_Information, e_Warning, e_Critical, e_Fatal};
}

QFile* fLogFile = NULL;
QMutex fLogMutex;
int fLogLevel = 0;
void LogfileMessageHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString& msg)
{
#undef _DEBUG
#ifdef _DEBUG
  QTextStream streamStdout(stdout);
#endif
  QTextStream stream(fLogFile);
  QDateTime now = QDateTime::currentDateTime();

  QString message(msg);
  QString emitter("QT");

  if(message.startsWith('[') && message.indexOf(']') != -1)
  {
    message.remove(0, 1);
    int nIndex = message.indexOf(']');
    emitter = message.left(nIndex);
    message.remove(0,nIndex+1);
    message = message.trimmed();
  }

  fLogMutex.lock();
  QString timeNow = now.toString("[MM.dd.yyyy hh:mm:ss] ");

#ifdef WINDOWS
#pragma warning(disable: 4063) //warning C4063: case '4' is not a valid value for switch of enum 'QtMsgType'. Caused by the custom QtMsgType, QSInformationMsg
#else
#pragma GCC diagnostic ignored "-Wswitch"
#endif
  switch (type) {
     case QtDebugMsg:
       if(fLogLevel <= Log::e_Debug)
       {
#ifdef _DEBUG
         streamStdout << timeNow << "Debug : " << emitter << " : " << message << "\n";
#endif
         stream << timeNow << "Debug : " << emitter << " : " << message << "\n";
       }
       break;
     case QSInformationMsg:
       if(fLogLevel <= Log::e_Information)
       {
#ifdef _DEBUG
         streamStdout << timeNow << "Information : " << emitter << " : " << message << "\n";
#endif
         stream << timeNow << "Information : " << emitter << " : " << message << "\n";
       }
       break;
     case QtWarningMsg:
       if(fLogLevel <= Log::e_Warning)
       {
#ifdef _DEBUG
         streamStdout << timeNow << "Warning : " << emitter << " : " << message << "\n";
#endif
         stream << timeNow << "Warning : " << emitter << " : " << message << "\n";
       }
       break;
     case QtCriticalMsg:
       if(fLogLevel <= Log::e_Critical)
       {
#ifdef _DEBUG
         streamStdout << timeNow << "Critical : " << emitter << " : " << message << "\n";
#endif
         stream << timeNow << "Critical : " << emitter << " : " << message << "\n";
       }
       break;
     case QtFatalMsg:
       if(fLogLevel <= Log::e_Fatal)
       {
#ifdef _DEBUG
         streamStdout << timeNow << "Fatal : " << emitter << " : " << message << "\n";
#endif
         stream << timeNow << "Fatal : " << emitter << " : " << message << "\n";
       }
       abort();
  }
  fLogMutex.unlock();
}
void CreateLogfile()
{
  QSettings cLogLevel;
  fLogLevel = cLogLevel.value("LogLevel", 1).toInt();
  QString logFilePath = joinPath(GetDataDir(), "syncclient.log");
  fLogFile = new QFile(logFilePath);
  if(fLogFile->open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
  {
    qInstallMessageHandler(LogfileMessageHandler);
  }
}
void CloseLogfile()
{
  if(fLogFile != NULL)
  {
    qInstallMessageHandler(0);
    delete fLogFile;
  }
}

QString GetDataDir()
{
  static QString dirString;
  if (dirString.isEmpty())
  {
#ifdef WINDOWS
    PWSTR localAppDataFolder;

    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppDataFolder) == S_OK)
    {
      dirString = QString::fromWCharArray(localAppDataFolder);
      CoTaskMemFree(localAppDataFolder);

      dirString = dirString.replace('\\', '/') + "/quicksync/";
      QDir dir(dirString);
      if (!dir.exists())
      {
        dir.mkpath(dirString);
      }
    }
#else
    dirString = QDir::homePath() + "/.quicksync";
#endif
  }
  return dirString;
}

QString GetHumanReadableSize(quint64 value)
{
  if(value == 0)
    return "0";
  //Fill the human readable list
  QStringList units;
  units.append("");
  units.append("Kb");
  units.append("Mb");
  units.append("Gb");
  units.append("Tb");
  units.append("Pb");
  units.append("Eb");
  units.append("Yb");

  double logVal = log10(static_cast<double>(value));
  quint64 unitIndex = static_cast<quint64>(logVal) / 3;
  unitIndex = std::min<quint64>(unitIndex, static_cast<quint64>(units.size()-1));
  QString sizeSting;
  if(unitIndex == 0)
    sizeSting = QString("%1b").arg(value);
  else
    sizeSting = QString("%1%2").arg(static_cast<double>(value)/(static_cast<quint64>(1)<<(10*unitIndex)), 0, 'f', 2).arg(units.at(static_cast<int>(unitIndex)));
  return sizeSting;
}
//-----------------------------------------------------------------------------

