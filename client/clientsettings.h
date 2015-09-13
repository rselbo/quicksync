// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#ifndef QUICKSYNC_CLIENTSETTINGS_H
#define QUICKSYNC_CLIENTSETTINGS_H

#pragma warning(push, 1)
#include "ui_settings.h"
#pragma warning(pop)
#include "syncrules.h"

class SyncRules;
//-----------------------------------------------------------------------------

struct BranchSpec
{
  QString m_Name;
  QString m_SourcePath;
  QString m_DestinationPath;
};
typedef QMap<QString, BranchSpec> Branches;

class ClientSettings : public QDialog, private Ui::Settings
{
  Q_OBJECT
public:
  ClientSettings(QSharedPointer<SyncRules> syncRules);
  virtual ~ClientSettings();
  static const QString branchesStr;
  static const QString srcPathStr;
  static const QString dstPathStr;
  static const QString ignoreExtStr;

  void Save();
private slots:
  void on_sourcePathButton_clicked();
  void on_addBranchButton_clicked();
  void on_removeBranchButton_clicked();
  void on_cloneBranchButton_clicked();
  void on_branchList_itemSelectionChanged();
  void on_branchNameEdit_returnPressed();
  void on_branchNameEdit_editingFinished();
  void on_sourcePathEdit_returnPressed();
  void on_sourcePathEdit_editingFinished();
  void on_destPathEdit_returnPressed();
  void on_destPathEdit_editingFinished();
  void accept();
  void reject();
  void writeWindowsSettings();
  void readWindowsSettings();
signals:
  void signalClearStats();

private:
  bool LoadBranchSpecs();
  bool SaveBranchSpecs();

  bool VerifyBranchName(const QString& name);
  QString MakeNewBranchName(const QString& name);
  QComboBox* CreateExclude();
  QComboBox* CreateFlags();

  Branches m_Branches;
};

//-----------------------------------------------------------------------------

#endif
