#include "PreCompile.h"
#include "syncruleviewmodel.h"


SyncRuleView::SyncRuleView(QWidget* parent) : QTableView(parent)
{

}

SyncRuleModel::SyncRuleModel(QObject* parent, QSharedPointer<SyncRules> rules) :
QAbstractTableModel(parent),
  m_Rules(rules)
{

}

QVariant SyncRuleModel::headerData (int section, Qt::Orientation orientation, int role) const
{
  if(role == Qt::DisplayRole)
  {
    if(orientation == Qt::Horizontal)
    {
      switch(section)
      {
      case 0:
        return "+/-";
      case 1:
        return "Flags";
      case 2:
        return "Pattern";
      }
    }
    else
      return section+1;
  }
  return QVariant();
}
QVariant SyncRuleModel::index(int row, int column, const QModelIndex& parent)
{
  return "test";
}
QVariant SyncRuleModel::data(const QModelIndex & modelIndex, int displayRole) const
{
  switch(displayRole)
  {
  case Qt::DisplayRole:
    if(modelIndex.column() == 0) //include/exclude
    {

    }
    else if(modelIndex.column() == 1) //flags
    {

    }
    else if(modelIndex.column() == 2) //rulepattern
    {
      return m_Rules->getSyncRule(modelIndex.row())->fPattern;
    }

  }
  return QVariant();
}