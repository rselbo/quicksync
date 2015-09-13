// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include "PreCompile.h"
#include "scannerbase.h"
#include "utils.h"
#include "filescanner.h"

//-----------------------------------------------------------------------------

ScannerBase::ScannerBase( const QString &rootPath, QSharedPointer<SyncRules> rules )
{
  fRootPath = rootPath;

  fFileCount = 0;
  fFileNumber = 0;
  fFileIgnored = 0;
  fDirCount = 0;
  fDirNumber = 0;
  fDirIgnored = 0;
}

ScannerBase::~ScannerBase()
{
}

//-----------------------------------------------------------------------------

ScannerBase *ScannerBase::selectScannerForFolder( const QString &rootPath, QSharedPointer<SyncRules> rules )
{
  return new FileScanner(rootPath, rules);
}

//-----------------------------------------------------------------------------
