#ifndef RULEVISUALIZERWORKER_H
#define RULEVISUALIZERWORKER_H

#include "syncrules.h"

enum DataRoles { PathRole = Qt::UserRole };

class RuleVisualizerWorker : public QThread
{
  Q_OBJECT
public:
  struct TreeNode;
  typedef QMap<QString, TreeNode> Tree;

  struct TreeNode
  {
    TreeNode() : m_Size(0), m_Excluded(false), m_RuleIndex(-1) {}
    quint64 m_Size;
    bool m_Excluded;
    int m_RuleIndex;
    Tree m_Children;
    QSharedPointer<SyncRules> m_Rules;
    QString m_Path;
  };

  RuleVisualizerWorker(const QString& path, const QString& branchName);
  ~RuleVisualizerWorker();

  QTreeWidgetItem* GetToplevelItem()  { QTreeWidgetItem* ret = m_ToplevelItem; m_ToplevelItem = NULL; return ret; }
  void FillExtensions(QTableWidget* table);
  void StopThread() { m_Stop = true; }
  const QStringList& GetRuleLocations() const { return m_RuleLocations; }
protected:
  virtual void run();
  void AddFile(const QString& file, quint64 size, bool excluded, QSharedPointer<SyncRules> rules, int ruleIndex, bool dir);
  void FillTree(const RuleVisualizerWorker::Tree& tree, QTreeWidgetItem* item);
signals:
  void signalScanComplete();
protected:
  void ScanDir(const QString& path, QSharedPointer<SyncRules> rules, bool excluded, int excludeRule);
  bool m_Stop; //Set to true when the thread is told to stop
  QString m_SourcePath;
  QString m_BranchName;
  Tree m_PathTree;
  QMap<QString, QPair<quint64, quint64> > m_ExtensionToSize;
  QTreeWidgetItem* m_ToplevelItem; 
  QPair<int, int> m_FileCount; //First is total file count, second is total included files count

  QStringList m_RuleLocations;
};

#endif //RULEVISUALIZERWORKER_H
