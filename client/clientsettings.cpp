// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include "PreCompile.h"
#include "clientsettings.h"

#include "syncrules.h"
#include "rulevisualizerwidget.h"
#include "utils.h"
//-----------------------------------------------------------------------------
const QString ClientSettings::branchesStr = "branches";
const QString ClientSettings::srcPathStr = "SourcePath";
const QString ClientSettings::dstPathStr = "DestinationPath";
const QString ClientSettings::ignoreExtStr = "IgnoredExtensions";

const int EXCLUDE = 0; //Exclude/include table col
const int FLAGS = 1; //flags table col
const int PATTERN = 2; //pattern table col

ClientSettings::ClientSettings(QSharedPointer<SyncRules> syncRules) : 
 QDialog()
{
  setupUi( this );
  readWindowsSettings();

#ifdef _MSC_VER
  QString cBuild = QString("Build: %1 %2").arg(__DATE__).arg(__TIME__);
  labelBuild->setText(cBuild);
#endif

  QSettings settings;
  hostnameEdit->setText( settings.value("server/hostname").toString() );
  portEdit->setText( settings.value("server/port").toString() );

  patchServer->setText( settings.value("patcher/hostname").toString() );
  patchPort->setText( settings.value("patcher/port", 80).toString() );
  bool usepatcher = settings.value("patcher/usepatcher", false).toBool();
  bool autoclose = settings.value("patcher/autoclose", true).toBool();
  if(usepatcher)
    usePatcher->setCheckState(Qt::Checked);
  else
    usePatcher->setCheckState(Qt::Unchecked);
  if(autoclose)
    autoClose->setCheckState(Qt::Checked);
  else
    autoClose->setCheckState(Qt::Unchecked);

  LoadBranchSpecs();
  for(auto i = m_Branches.begin(); i != m_Branches.end(); ++i)
  {
    branchList->addItem(i->m_Name);
  }

  branchNameEdit->setDisabled(true);
  sourcePathEdit->setDisabled(true);
  destPathEdit->setDisabled(true);
}

ClientSettings::~ClientSettings()
{
}

bool ClientSettings::LoadBranchSpecs()
{
  QSettings settings;
  settings.beginGroup(branchesStr);
  QStringList branches = settings.childGroups();
  for(int i=0; i<branches.size(); ++i)
  {
    BranchSpec spec;
    spec.m_Name = branches.at(i);
    settings.beginGroup(spec.m_Name);
    spec.m_SourcePath = settings.value(srcPathStr).toString();
    spec.m_DestinationPath = settings.value(dstPathStr).toString();
    settings.endGroup();
    m_Branches[spec.m_Name] = spec;
  }
  settings.endGroup();
  return settings.status() == QSettings::NoError;
}

bool ClientSettings::SaveBranchSpecs()
{
  QSettings settings;
  //clear all the old entries
  settings.remove(branchesStr);
  //create the group again
  settings.beginGroup(branchesStr);

  //add each entry
  for(auto i = m_Branches.begin(); i != m_Branches.end(); ++i)
  {
    settings.beginGroup(i->m_Name);
    settings.setValue(srcPathStr, i->m_SourcePath);
    settings.setValue(dstPathStr, i->m_DestinationPath);
    settings.endGroup();
  }
  settings.endGroup();

  return settings.status() == QSettings::NoError;
}

void ClientSettings::Save()
{
  SaveBranchSpecs();
}

void ClientSettings::on_sourcePathButton_clicked()
{
  QString dir = QFileDialog::getExistingDirectory( this, "Select source directory",  sourcePathEdit->text());
  if( !dir.isEmpty() )
  {
    sourcePathEdit->setText( dir );
    on_sourcePathEdit_returnPressed();
  }
}

void ClientSettings::on_addBranchButton_clicked()
{
  //create the new spec and give it a unique name
  BranchSpec spec;
  spec.m_Name = "NewBranch";
  while(!VerifyBranchName(spec.m_Name))
  {
    spec.m_Name = MakeNewBranchName(spec.m_Name);
  }

  //reset the editing gui
  branchNameEdit->setText(spec.m_Name);
  sourcePathEdit->setText("");
  destPathEdit->setText("");

  //add the item to the branch gui list amd select it 
  branchList->addItem(spec.m_Name);
  branchList->setCurrentRow(branchList->count()-1);
  branchNameEdit->setEnabled(true);
  sourcePathEdit->setEnabled(true);
  destPathEdit->setEnabled(true);


  //add it to the memory list
  m_Branches[spec.m_Name] = spec;
}

void ClientSettings::on_removeBranchButton_clicked()
{
  QListWidgetItem* pcListItem = branchList->currentItem();
  if(pcListItem != NULL)
  {
    QString cBranchName = pcListItem->text();
    branchList->takeItem(branchList->currentRow());
    m_Branches.remove(cBranchName);
  }
}

void ClientSettings::on_cloneBranchButton_clicked()
{
  QListWidgetItem* pcListItem = branchList->currentItem();
  if(pcListItem != NULL)
  {
    Branches::ConstIterator i = m_Branches.find(pcListItem->text());
    if(i != m_Branches.end())
    {
      const BranchSpec& originalSpec = i.value();

      //create the new spec and give it a unique name
      BranchSpec spec = originalSpec;
      while(!VerifyBranchName(spec.m_Name))
      {
        spec.m_Name = MakeNewBranchName(spec.m_Name);
      }


      //reset the editing gui
      branchNameEdit->setText(spec.m_Name);
      sourcePathEdit->setText(spec.m_SourcePath);
      destPathEdit->setText(spec.m_DestinationPath);

      //add the item to the branch gui list amd select it 
      branchList->addItem(spec.m_Name);
      branchList->setCurrentRow(branchList->count()-1);
      branchNameEdit->setEnabled(true);
      sourcePathEdit->setEnabled(true);
      destPathEdit->setEnabled(true);


      //add it to the memory list
      m_Branches[spec.m_Name] = spec;
    }
  }
}

void ClientSettings::on_branchList_itemSelectionChanged()
{
  QListWidgetItem* pcListItem = branchList->currentItem();
  if(pcListItem != NULL)
  {
    QString cBranchName = pcListItem->text();
    auto spec = m_Branches.find(cBranchName);
    if(spec == m_Branches.end())
    {
      return;
    }
    branchNameEdit->setText(cBranchName);
    sourcePathEdit->setText(spec->m_SourcePath);
    destPathEdit->setText(spec->m_DestinationPath);

    branchNameEdit->setEnabled(true);
    sourcePathEdit->setEnabled(true);
    destPathEdit->setEnabled(true);
  }
  else
  {
    branchNameEdit->setDisabled(true);
    sourcePathEdit->setDisabled(true);
    destPathEdit->setDisabled(true);
  }
}

void ClientSettings::on_branchNameEdit_returnPressed()
{
  QString newBranchName = branchNameEdit->text();
  QListWidgetItem* listItem = branchList->currentItem();


  //verify that it is a namechange and that we actually have a list item selected
  if(listItem != NULL && newBranchName != listItem->text())
  {
    //make sure we get a unique and valid name
    while(!VerifyBranchName(newBranchName))
    {
      newBranchName = MakeNewBranchName(newBranchName);
    }

    //get the old name and the related branch spec
    QString oldName = listItem->text();
    auto spec = m_Branches.find(oldName);
    if(spec == m_Branches.end())
    {
      return;
    }

    //rename the branch spec, copy it into the new name in the m_Branches map and erase the old value
    spec->m_Name = newBranchName;
    m_Branches[newBranchName] = *spec;
    m_Branches.erase(spec);

    //update the guiname
    listItem->setText(newBranchName);
    branchNameEdit->setText(newBranchName);
  }
}

void ClientSettings::on_branchNameEdit_editingFinished()
{
  on_branchNameEdit_returnPressed();
}

void ClientSettings::on_sourcePathEdit_returnPressed()
{
  //qDebug() << "[ClientSettings.Debug]on_sourcePathEdit_returnPressed";
  QString branchName = branchNameEdit->text();
  auto spec = m_Branches.find(branchName);
  if(spec == m_Branches.end())
  {
    return;
  }
  spec->m_SourcePath = sourcePathEdit->text();
}

void ClientSettings::on_sourcePathEdit_editingFinished()
{
  on_sourcePathEdit_returnPressed();
}

void ClientSettings::on_destPathEdit_returnPressed()
{
  QString branchName = branchNameEdit->text();
  auto spec = m_Branches.find(branchName);
  if(spec == m_Branches.end())
  {
    return;
  }
  spec->m_DestinationPath = destPathEdit->text();
}

void ClientSettings::on_destPathEdit_editingFinished()
{
  on_destPathEdit_returnPressed();
}

void ClientSettings::accept()
{
  //qDebug() << "[ClientSettings.Debug] accept";
  writeWindowsSettings();
  SaveBranchSpecs();

  QSettings settings;
  settings.setValue( "server/hostname", hostnameEdit->text() );
  settings.setValue( "server/port", portEdit->text().toInt() );

  bool usepatcher = false;
  if(usePatcher->checkState() == Qt::Checked)
  {
    usepatcher = true;
  }
  bool autoclose = true;
  if(autoClose->checkState() == Qt::Unchecked)
  {
    autoclose = false;
  }
  settings.setValue( "patcher/hostname", patchServer->text() );
  settings.setValue( "patcher/port", patchPort->text().toInt() );
  settings.setValue( "patcher/usepatcher", usepatcher );
  settings.setValue( "patcher/autoclose", autoclose );
 
  QDialog::accept();
}

void ClientSettings::reject()
{
  qDebug() << "[ClientSettings.Debug] reject";
  writeWindowsSettings();
  QDialog::reject();
}

void ClientSettings::readWindowsSettings()
{
  QSettings settings;

  settings.beginGroup("SettingsWindow");
  resize(settings.value("size", QSize(370, 320)).toSize());
  move(settings.value("pos", QPoint(200, 200)).toPoint());
  settings.endGroup();
}

void ClientSettings::writeWindowsSettings()
{
  QSettings settings;
  settings.beginGroup("SettingsWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.endGroup();
}

//QComboBox* ClientSettings::CreateExclude()
//{
//  QComboBox* pcRules = new QComboBox(ignoreTable);
//  pcRules->addItem("exclude", true);
//  pcRules->addItem("include", false);
//
//  return pcRules;
//}
//
//QComboBox* ClientSettings::CreateFlags()
//{
//  QComboBox* pcFlags = new QComboBox(ignoreTable);
//  pcFlags->addItem("None", 0);
//  pcFlags->addItem("Binary", (int)e_Binary);
//  pcFlags->addItem("Executable", (int)e_Executable);
//  pcFlags->addItem("Binary Executable", (int)e_Binary|e_Executable);
//
//  return pcFlags;
//}
//-----------------------------------------------------------------------------

bool ClientSettings::VerifyBranchName(const QString& name)
{
  if(name.isEmpty())
    return false;

  for(auto i = m_Branches.begin(); i != m_Branches.end(); ++i)
  {
    if(name == i->m_Name)
      return false;
  }

  return true;
}
QString ClientSettings::MakeNewBranchName(const QString& name)
{
  if(name.isEmpty())
    return "NoName";

  QRegExp regExp("(.*)([0-9]+)");
  if(regExp.exactMatch(name) == true)
  {
    //increase the trailing number
    QString baseName = regExp.cap(1);
    QString digits = regExp.cap(2);
    int nr = digits.toInt();
    
    return QString("%1%2").arg(baseName).arg(++nr);
  }
  else
  {
    //just add a "1" at the end
    //this is the only way to make qts silly string formatter make "name1", %11 does not work and %s1 is not recognised
    return QString("%1%2").arg(name).arg(1);
  }
}