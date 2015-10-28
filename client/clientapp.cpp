// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include "PreCompile.h"
#include "clientapp.h"
#include "clientwindow.h"
//#include "patcher.h"
#include "exceptionhandler.h"
#include "utils.h"
//-----------------------------------------------------------------------------

ClientApp::ClientApp( int argc, char **argv ) : QApplication( argc, argv ), m_Window(NULL)
{
  ::QString cBranch;
  bool bShutdownAfterUpdate = false;
  bool bInitialUpdate = false;

  for(int i=1;i<argc;++i)
  {
    QString arg(argv[i]);
    if(arg.startsWith("-branch=", Qt::CaseInsensitive))
    {
      cBranch = arg.right(arg.size()-8);
      qDebug() << "[ClientApp.Init] Found branch argument, will try to set" << cBranch << "as default branch";
    }
    else if(arg == "-s")
    {
      bShutdownAfterUpdate = true;
      bInitialUpdate = true;
      qDebug() << "[ClientApp.Init] Found argument telling us to shut down after update (also implies the -u option)";
    }
    else if(arg == "-u")
    {
      bInitialUpdate = true;
      qDebug() << "[ClientApp.Init] Found argument telling us to do a update on the selected branch at startup";
    }
  }

  m_Window = new ClientWindow(cBranch, bInitialUpdate, bShutdownAfterUpdate);
  m_Window->show();
}

ClientApp::~ClientApp()
{
  delete m_Window;
}

//-----------------------------------------------------------------------------

int main( int argc, char **argv )
{
  QCoreApplication::setApplicationName( "syncclient" );

  CreateLogfile();
  ExceptionHandler::registerExceptionHandler();

  ClientApp app( argc, argv );
  int nRet = app.exec(); 

  ExceptionHandler::unRegisterExceptionHandler();

  CloseLogfile();
  return nRet;
}

int __stdcall WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, char* lpCmdLine, int /*nCmdShow*/)
{
  QCoreApplication::setApplicationName( "syncclient" );

  CreateLogfile();
  ExceptionHandler::registerExceptionHandler();

  ClientApp app( 1, &lpCmdLine );
  int nRet = 0; app.exec();

  ExceptionHandler::unRegisterExceptionHandler();
  CloseLogfile();
  return nRet;
}
//-----------------------------------------------------------------------------
