#ifndef SYNCRULEVIEWMODEL
#define SYNCRULEVIEWMODEL

class SyncRuleView : public QTableView
{
public:
  SyncRuleView(QWidget* parent=NULL);

};

class SyncRuleModel : public QAbstractTableModel
{
public:
  SyncRuleModel(QObject* parent, QSharedPointer<SyncRules> rules);

  QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
  int rowCount(const QModelIndex &) const { return m_Rules->getNrSyncRules(); }
  int columnCount(const QModelIndex &) const { return 3; }
  QVariant data(const QModelIndex &,int) const;
  QVariant index(int row, int column, const QModelIndex& parent=QModelIndex());
protected:
  QSharedPointer<SyncRules> m_Rules;
};

#endif //SYNCRULEVIEWMODEL
