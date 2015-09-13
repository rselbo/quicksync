#ifndef RULEWIDGET_H
#define RULEWIDGET_H

#pragma warning(push, 1)
#include "ui_rulewidget.h"
#pragma warning(pop)
#include "syncrules.h"

class RuleWidget : public QWidget, public Ui::RuleWidget
{
  static const int EXCLUDE = 0; //Exclude/include table col
  static const int FLAGS = 1; //flags table col
  static const int PATTERN = 2; //pattern table col

  Q_OBJECT
public:
  RuleWidget(QWidget* parent=NULL);
public slots:
  void on_deleteSyncrules_clicked();
  void on_addRuleButton_clicked();
  void on_removeRuleButton_clicked();
  void on_moveUpButton_clicked();
  void on_moveDownButton_clicked();
  void on_saveButton_clicked();
  void on_revertButton_clicked();
  void on_ruleLocationDropdown_activated(const QString& location);
  void on_ignoreTable_cellChanged(int, int);

  void slotNewRuleLocations(const QStringList& locations);
  void slotSourcePath(const QString& sourcePath) { m_SourcePath = sourcePath; }
  bool HasModifiedRules() const;
  bool SaveAllModifiedRules(QString* errorMsg=NULL);
protected:
  QComboBox* CreateExclude();
  QComboBox* CreateFlags();
  void MoveItem(int from, int to);

  void SetRulesModified(bool modified=true);
  
  QString TableRulesToSyncRules(QSharedPointer<SyncRules> table);
  void SyncRulesToTableRules(QSharedPointer<const SyncRules> syncRules);

  QStringList m_RuleLocations;
  QString m_SourcePath;
  bool m_LoadingRules;

  QMap<QString, QSharedPointer<SyncRules> > m_Rules;
  QMap<QString, bool> m_ModifiedRules;
};

extern QString defaultFile;


#endif //RULEWIDGET_H
