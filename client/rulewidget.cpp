#include "PreCompile.h"
#include "rulewidget.h"
#include "utils.h"

RuleWidget::RuleWidget(QWidget* parent) : QWidget(parent)
{
  setupUi(this);
  m_LoadingRules = false;
  ignoreTable->horizontalHeader()->setStretchLastSection(true);
  slotNewRuleLocations(QStringList());
}

/// Slot to receive the file locations where the rule files exist
void RuleWidget::slotNewRuleLocations(const QStringList& locations)
{
  m_Rules.clear();
  m_ModifiedRules.clear();
  
  qDebug() << "slotNewRuleLocations " << locations.size() << " Locations";
  m_RuleLocations = locations;
  ruleLocationDropdown->clear();
  ruleLocationDropdown->addItem(defaultFile);
  ruleLocationDropdown->addItems(m_RuleLocations);

  if(!ruleLocationDropdown->currentText().isEmpty())
  {
    on_ruleLocationDropdown_activated(ruleLocationDropdown->currentText());
  }

  QIcon default(":qs.png");
  for(int i=0;i<ruleLocationDropdown->count();i++)
  {
    ruleLocationDropdown->setItemIcon(i, default);
  }
}

/// Called when the user presses the delete rule location button
void RuleWidget::on_deleteSyncrules_clicked()
{
  if(ruleLocationDropdown->currentIndex() != -1)
  {
    if(QMessageBox::warning(this, "Delete file", QString("Do you want to delete the rule file %1?").arg(ruleLocationDropdown->currentText()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
      m_Rules.remove(ruleLocationDropdown->currentText());
      m_ModifiedRules.remove(ruleLocationDropdown->currentText());
      QFile::remove(joinPath(m_SourcePath, ruleLocationDropdown->currentText()));
      ruleLocationDropdown->removeItem(ruleLocationDropdown->currentIndex());
      if(ruleLocationDropdown->currentIndex() != -1)
      {
        on_ruleLocationDropdown_activated(ruleLocationDropdown->currentText());
      }
    }
  }
}

/// Called when the user presses the add rule button
/// It will insert a new row in the table and set the modified flag on the rules
void RuleWidget::on_addRuleButton_clicked()
{
  int row = ignoreTable->rowCount();
  ignoreTable->setRowCount(row+1);

  ignoreTable->setCellWidget(row, EXCLUDE, CreateExclude());
  ignoreTable->setCellWidget(row, FLAGS, CreateFlags());

  ignoreTable->resizeRowsToContents();
  SetRulesModified();
}

/// Called when the user clicks the remove rule button
/// Will remove the currently selected row in the rule table if any is selected, else it will do nothing.
/// If a row is removed the modified flag is set.
void RuleWidget::on_removeRuleButton_clicked()
{
  QTableWidgetItem* pcItem = ignoreTable->currentItem();
  if(pcItem != NULL)
  {
    int nRow = pcItem->row();
    ignoreTable->removeRow(nRow);
    SetRulesModified();
  }
}

/// Called when a user clicks the move rule up button
/// Will move the current rule one step up and set the modified flag
void RuleWidget::on_moveUpButton_clicked()
{
  int from = ignoreTable->currentRow();
  MoveItem(from, from-1);
}

/// Called when a user clicks the move rule down button
/// Will move the current rule one step down and set the modified flag
void RuleWidget::on_moveDownButton_clicked()
{
  int from = ignoreTable->currentRow();
  MoveItem(from, from+1);
}

void RuleWidget::MoveItem(int from, int to)
{
  //make sure the to and from makes sense, the must be different, and valid (between 0 and listsize - 1)
  if(from != to && from != -1 && from < ignoreTable->rowCount() && to != -1 && to < ignoreTable->rowCount())
  {
    //swap the pattern item
    QTableWidgetItem* fromPattern = ignoreTable->takeItem(from, PATTERN);
    ignoreTable->setItem(from, PATTERN, ignoreTable->takeItem(to, PATTERN));
    ignoreTable->setItem(to,PATTERN, fromPattern);

    //Get the from and to exclude statuses and set them on the oposite item
    int fromExcude = static_cast<QComboBox*>(ignoreTable->cellWidget(from, EXCLUDE))->currentIndex();
    int toExclude = static_cast<QComboBox*>(ignoreTable->cellWidget(to, EXCLUDE))->currentIndex();
    static_cast<QComboBox*>(ignoreTable->cellWidget(from, EXCLUDE))->setCurrentIndex(toExclude);
    static_cast<QComboBox*>(ignoreTable->cellWidget(to, EXCLUDE))->setCurrentIndex(fromExcude);

    //Get the from and to flags and set them on the oposite item
    int fromFlags = static_cast<QComboBox*>(ignoreTable->cellWidget(from, FLAGS))->currentIndex();
    int toFlags = static_cast<QComboBox*>(ignoreTable->cellWidget(to, FLAGS))->currentIndex();
    static_cast<QComboBox*>(ignoreTable->cellWidget(from, FLAGS))->setCurrentIndex(toFlags);
    static_cast<QComboBox*>(ignoreTable->cellWidget(to, FLAGS))->setCurrentIndex(fromFlags);

    ignoreTable->setCurrentItem(fromPattern);
    SetRulesModified();
  }
}


/// Called when the user presses the save rules button
/// If there are any changes and no errors in the rules the new rules will be saved to the current file location and the modified flag cleared
void RuleWidget::on_saveButton_clicked()
{
  QString location = ruleLocationDropdown->currentText();
  QSharedPointer<SyncRules> rulesOnFile(new SyncRules);
  QSharedPointer<SyncRules> rulesInTable(new SyncRules);

  if(location != defaultFile)
  {
    location = joinPath(m_SourcePath, location);
  }

  rulesOnFile->loadXmlRules(location);
  QString error = TableRulesToSyncRules(rulesInTable);
  if(!error.isEmpty())
  {
    QMessageBox::critical(this, "Save Error", QString("There are one or more errors with the current rules. Nothing will be saved, fix the errors and save again!\n%1").arg(error), QMessageBox::Ok);
    return;
  }

  if(rulesInTable != rulesOnFile)
  {
    rulesInTable->saveXmlRules(location);
  }
  SetRulesModified(false);
}

/// Called when the user presses the reload rules button
void RuleWidget::on_revertButton_clicked()
{
  QString location = ruleLocationDropdown->currentText();
  if(location != defaultFile)
  {
    location = joinPath(m_SourcePath, location);
  }
  QSharedPointer<SyncRules> rulesOnFile(new SyncRules);
  rulesOnFile->loadXmlRules(location);
  SyncRulesToTableRules(rulesOnFile);
  m_Rules[location] = rulesOnFile;
  SetRulesModified(false);
}

void RuleWidget::on_ruleLocationDropdown_activated(const QString& location)
{
  QSharedPointer<SyncRules> rules;

  //check if we have the new location cached
  auto iterator = m_Rules.find(location);
  if(iterator != m_Rules.end())
  {
    rules = iterator.value();
  }
  else
  {
    //Load the rules from file and cache them
    rules = QSharedPointer<SyncRules>(new SyncRules);
    if(location == defaultFile)
    {
      rules->loadXmlRules(defaultFile);
    }
    else
    {
      rules->loadXmlRules(joinPath(m_SourcePath, ruleLocationDropdown->currentText()));
    }
    m_Rules[location] = rules;
    m_ModifiedRules[location] = false;
  }

  //disable the delete button for the default rules
  deleteSyncrules->setDisabled(location == defaultFile);
  SyncRulesToTableRules(rules);
}

void RuleWidget::on_ignoreTable_cellChanged(int, int)
{
  if(!m_LoadingRules)
  {
    SetRulesModified();
  }
}

QComboBox* RuleWidget::CreateExclude()
{
  QComboBox* pcRules = new QComboBox(ignoreTable);
  pcRules->addItem("exclude", true);
  pcRules->addItem("include", false);

  return pcRules;
}

QComboBox* RuleWidget::CreateFlags()
{
  QComboBox* pcFlags = new QComboBox(ignoreTable);
  pcFlags->addItem("None", 0);
  pcFlags->addItem("Binary", (int)e_Binary);
  pcFlags->addItem("Executable", (int)e_Executable);
  pcFlags->addItem("Binary Executable", (int)e_Binary|e_Executable);

  return pcFlags;
}

void RuleWidget::SetRulesModified(bool modified)
{
  static QIcon icons[2] = { QIcon(":/qs.png"), QIcon(":/qsdis.png")};
  ruleLocationDropdown->setItemIcon(ruleLocationDropdown->currentIndex(), icons[modified]);
  m_ModifiedRules[ruleLocationDropdown->currentText()] = modified;
  if(modified)
  {
    auto i = m_Rules.find(ruleLocationDropdown->currentText());
    if(i != m_Rules.end())
    {
      TableRulesToSyncRules(i.value());
    }
  }
}


QString RuleWidget::TableRulesToSyncRules(QSharedPointer< SyncRules> syncRules)
{
  syncRules->clear();//make sure there are no rules left over

  //create sync rules from the table
  int rowcount = ignoreTable->rowCount();
  QString errorReport;
  for(int i=0; i < rowcount; i++)
  {
    QString error;
    QComboBox* exclude = dynamic_cast<QComboBox*>(ignoreTable->cellWidget(i, EXCLUDE));
    QComboBox* flags = dynamic_cast<QComboBox*>(ignoreTable->cellWidget(i, FLAGS));
    QTableWidgetItem* item = ignoreTable->item(i,PATTERN);
    if(exclude != NULL && item != NULL)
    {
      if(syncRules->createRule(item->text(), exclude->itemData(exclude->currentIndex()).toBool(), flags->itemData(flags->currentIndex()).toInt(), error) == -1)
      {
        //Something went wrong with this rule, put it into the errorReport
        errorReport.append(QString("Error occured with rule at index %1: %2\n").arg(i+1).arg(error));
      }
    }
  }
  return errorReport;
}

void RuleWidget::SyncRulesToTableRules(QSharedPointer<const SyncRules> syncRules)
{
  m_LoadingRules = true;
  ignoreTable->setRowCount(syncRules->getNrSyncRules());
  for(int i=0;i<syncRules->getNrSyncRules();i++)
  {
    const SyncRule* rule = syncRules->getSyncRule(i);
    if(rule)
    {
      QComboBox* pcRules = CreateExclude();
      if(rule->fExclude)
        pcRules->setCurrentIndex(0);
      else
        pcRules->setCurrentIndex(1);
      ignoreTable->setCellWidget(i, EXCLUDE, pcRules);
      QComboBox* pcFlags = CreateFlags();
      pcFlags->setCurrentIndex(rule->fFlags);
      ignoreTable->setCellWidget(i, FLAGS, pcFlags);
      ignoreTable->setItem(i,PATTERN, new QTableWidgetItem(rule->fPattern));
    }
  }
  ignoreTable->resizeRowsToContents();
  m_LoadingRules = false;
}

bool RuleWidget::HasModifiedRules() const
{
  for(auto i = m_ModifiedRules.begin(); i != m_ModifiedRules.end(); ++i)
  {
    if(i.value())
      return true;
  }
  return false;
}

bool RuleWidget::SaveAllModifiedRules(QString* errorMsg)
{
  return true;
}
