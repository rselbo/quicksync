#include "PreCompile.h"
#include "exceptionhandler.h"
#if defined( WINDOWS )
#include "windows.h"
#include <ErrorRep.h>
#include <DbgHelp.h>
#include <shlobj.h>
#endif
#include "utils.h"

namespace ExceptionHandler
{
  LPTOP_LEVEL_EXCEPTION_FILTER  gPreviousFilter = NULL;

  LONG WINAPI                   process( PEXCEPTION_POINTERS pExceptionInfo );
}


bool ExceptionHandler::registerExceptionHandler()
{
  gPreviousFilter = SetUnhandledExceptionFilter( ExceptionHandler::process );
  return true;
}

void ExceptionHandler::unRegisterExceptionHandler()
{
  SetUnhandledExceptionFilter( gPreviousFilter );
}

LONG WINAPI ExceptionHandler::process( PEXCEPTION_POINTERS pInfo )
{
  MINIDUMP_EXCEPTION_INFORMATION  cMDInfo;

  HANDLE hFile;
  QString cDumpName("syncclient.dmp");
  QString cDumpPath(joinPath(GetDataDir(), QDateTime::currentDateTime().toString("yyyyMMddhhmmss")));
//  QDir dir(cDumpPath); dir.mkpath(cDumpPath); //make sure the dir is created
  cDumpName = joinPath(cDumpPath, cDumpName);
  cDumpName.replace('/', '\\');
  wchar_t* dumpNameW = static_cast<wchar_t*>(alloca((cDumpName.length() + 1) * sizeof(wchar_t)));
  //Try to create the the minidump in the appdata folder
  hFile = CreateFile(dumpNameW, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  if(hFile == INVALID_HANDLE_VALUE)
  {
    //if it failed try to create it in the current folder
    cDumpName = "syncclient.dmp";
    hFile = CreateFile(dumpNameW, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
  }

  if( hFile != INVALID_HANDLE_VALUE )
  {
    cMDInfo.ThreadId = GetCurrentThreadId();
    cMDInfo.ClientPointers = true;
    cMDInfo.ExceptionPointers = pInfo;

    if( !MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &cMDInfo, NULL, NULL ))
    {
      LPWSTR pBuffer = NULL;

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, 
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        pBuffer,
        0, NULL);

      QMessageBox::warning(NULL, "Crash 1", QString("Ooops, it looks like quicksync crashed, and we did not even manage to write a minidump file, sucks to be you. GetLastError returned %1").arg(QString::fromWCharArray(pBuffer)));
      LocalFree(pBuffer);
      return EXCEPTION_EXECUTE_HANDLER;
    }
    CloseHandle(hFile);
  }
  else
  {
    LPWSTR pBuffer = NULL;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, 
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      pBuffer, 
      0, NULL);

    QMessageBox::warning(NULL, "Crash 2", QString("Could not open a file handle to write the minidump. GetLastError returned %1").arg(QString::fromWCharArray(pBuffer)));
    LocalFree(pBuffer);
    return EXCEPTION_EXECUTE_HANDLER;
  }

  QMessageBox::warning(NULL, "Crash 3", QString("Ooops, it looks like quicksync crashed, but we managed to write a minidump file at \"%1\". Please tell the coder in charge about this").arg(cDumpName));
  exit( EXIT_SUCCESS );
}