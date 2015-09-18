#include "PreCompile.h"
#include "rulevisualizerworker.h"
#include "utils.h"
#include "windows.h"

RuleVisualizerWorker::RuleVisualizerWorker(const QString& path, const QString& branchName) :
  m_SourcePath(path),
  m_BranchName(branchName),
  m_ToplevelItem(NULL)
{

}

RuleVisualizerWorker::~RuleVisualizerWorker()
{
  delete m_ToplevelItem;
}

void RuleVisualizerWorker::run()
{
  m_PathTree["."]; //create the root node
  m_Stop = false;
  QSharedPointer<SyncRules> rules(new SyncRules);
  rules->loadXmlRules(defaultFile);

  QString rulePath = joinPath(m_SourcePath, "syncrules.xml");
  if(QFile::exists(rulePath))
  {
    QSharedPointer<SyncRules> loadRules(new SyncRules);
    if(loadRules->loadXmlRules(rulePath))
    {
      m_RuleLocations.push_back("syncrules.xml");
      rules = loadRules;
    }
  }

  ScanDir(".", rules, false, -1);
  if(m_Stop)
    return;
  FillTree(m_PathTree, NULL);
  if(m_Stop)
    return;
  if(m_ToplevelItem)
    m_ToplevelItem->setText(2, QString("Total: %1, Included: %2, Excluded %3").arg(m_FileCount.first).arg(m_FileCount.second).arg(m_FileCount.first-m_FileCount.second));
  emit signalScanComplete();
}

void RuleVisualizerWorker::ScanDir(const QString& path, QSharedPointer<SyncRules> rules, bool excluded, int excludeRule)
{
  QString search = joinPath(m_SourcePath, path);
  QDir searchDir(search);

  QFileInfoList fileList = searchDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
  for (auto info : fileList)
  {
    if (info.isDir())
    {
      QString dirStr = joinPath(path, info.fileName());
      if (excluded)
      {
        //Excluded by previous rule, keep scanning with the exclude flag and rule the same as before
        AddFile(dirStr, 0, excluded, rules, excludeRule, true);
        ScanDir(dirStr, rules, excluded, excludeRule);
      }
      else
      {
        SyncRuleFlags_e flags;
        if (rules->CheckFile(dirStr.toLower(), flags, excludeRule))
        {
          //We only read new rules in folders that have not already been excluded
          QString rulePath = joinPath(m_SourcePath, dirStr, "syncrules.xml");
          if (QFile::exists(rulePath))
          {
            QSharedPointer<SyncRules> loadRules(new SyncRules);
            if (loadRules->loadXmlRules(rulePath))
            {
              m_RuleLocations.push_back(joinPath(dirStr, "syncrules.xml"));
              rules = loadRules;
            }
          }

          AddFile(dirStr, 0, false, rules, excludeRule, true);
          ScanDir(dirStr, rules, false, excludeRule);
        }
        else
        {
          AddFile(dirStr, 0, true, rules, excludeRule, true);
          ScanDir(dirStr, rules, true, excludeRule);
        }
      }
    }
    else
    {
      QString fileStr = joinPath(path, info.fileName());
      if (fileStr.endsWith("syncrules.xml"))
        continue;

      if (excluded)
      {
        AddFile(fileStr, info.size(), true, rules, excludeRule, false);
      }
      else
      {
        SyncRuleFlags_e flags;
        if (rules->CheckFile(fileStr.toLower(), flags, excludeRule))
        {
          AddFile(fileStr, info.size(), false, rules, excludeRule, false);
        }
        else
        {
          AddFile(fileStr, info.size(), true, rules, excludeRule, false);
        }
      }
    }
  } 
}

void RuleVisualizerWorker::AddFile(const QString& file, quint64 size, bool excluded, QSharedPointer<SyncRules> rules, int ruleIndex, bool dir)
{
  if(!dir)
  {
    //increment the total number of files
    m_FileCount.first++;
    if(!excluded)
    {
      //increment total included files
      m_FileCount.second++;
    }
  }
  QStringList parts = file.split('/', QString::SkipEmptyParts);
  RuleVisualizerWorker::TreeNode* node = NULL;
  for(QStringList::const_iterator i = parts.begin(); i != parts.end(); i++)
  {
    if(node)
    {
      RuleVisualizerWorker::TreeNode* newNode = &(node->m_Children[*i]);
      if(!excluded)
        newNode->m_Size += size;
      node = newNode;
    }
    else
    {
      node = &(m_PathTree[*i]);
      if(!excluded)
        node->m_Size += size;
    }
  }
  if(node)
  {
    if(excluded)
      node->m_Size += size;
    node->m_Excluded = excluded;
    node->m_Rules = rules;
    node->m_RuleIndex = ruleIndex;
    node->m_Path = file;
  }

  if(!excluded && !dir)
  {
    //extract the extension of the file
    int dotPos = file.lastIndexOf('.');
    int slashPos = file.lastIndexOf('/');
    if(dotPos != -1 && dotPos > slashPos)
    {
      QPair<quint64, quint64>& ext = m_ExtensionToSize[file.right(file.length() - dotPos)];
      ext.first += size;
      ext.second++;
    }
    else
    {
      QPair<quint64, quint64>& ext = m_ExtensionToSize["no extension"];
      ext.first += size;
      ext.second++;
    }
  }
}

void RuleVisualizerWorker::FillExtensions(QTableWidget* table)
{
  class SortableTableItem : public QTableWidgetItem
  {
  public:
    SortableTableItem(const QString& label, quint64 size) : QTableWidgetItem(label) { setData(Qt::UserRole, size); }
    virtual bool operator<(const QTableWidgetItem& rhs) const { return data(Qt::UserRole).toULongLong() < rhs.data(Qt::UserRole).toULongLong(); }
  };

  table->setRowCount(m_ExtensionToSize.size());

  QMap<QString, QPair<quint64, quint64> >::const_iterator i = m_ExtensionToSize.begin();
  QMap<QString, QPair<quint64, quint64> >::const_iterator iEnd = m_ExtensionToSize.end();
  for(int row=0; i != iEnd; i++, row++)
  {
    table->setItem(row, 0, new QTableWidgetItem(i.key()));
    table->setItem(row, 1, new SortableTableItem(GetHumanReadableSize(i.value().first), i.value().first));
    table->setItem(row, 2, new SortableTableItem(QString("%1").arg(i.value().second), i.value().second));
  }

}

void RuleVisualizerWorker::FillTree(const RuleVisualizerWorker::Tree& tree, QTreeWidgetItem* item)
{
  RuleVisualizerWorker::Tree::const_iterator i = tree.begin();
  RuleVisualizerWorker::Tree::const_iterator iEnd = tree.end();

  for(; i != iEnd && !m_Stop; i++)
  {
    if(item)
    {
      QTreeWidgetItem* newItem = new QTreeWidgetItem();
      newItem->setText(0, i.key());
      newItem->setData(0, PathRole, i->m_Path);
      if(i->m_Excluded)
      {
        newItem->setBackgroundColor(0, QColor::fromRgb(255, 192, 192));
      }
      newItem->setText(1, GetHumanReadableSize(i.value().m_Size));
      const SyncRule* rule = i->m_Rules->getSyncRule(i->m_RuleIndex);
      if(rule)
      {
        if(!i->m_Excluded)
        {
          newItem->setBackgroundColor(0, QColor::fromRgb(192, 255, 192));
        }
        newItem->setText(2, rule->fPattern);
        newItem->setText(3, i->m_Rules->getRuleLocation());
      }
      else
      {
        newItem->setText(2, "Fallthrough");
        newItem->setText(3, "");
      }
      FillTree(i.value().m_Children, newItem);
      item->addChild(newItem);
    }
    else
    {
      m_ToplevelItem = new QTreeWidgetItem();
      m_ToplevelItem->setText(0, m_BranchName);
      m_ToplevelItem->setData(0, PathRole, i->m_Path);
      m_ToplevelItem->setText(1, QString("%1").arg(GetHumanReadableSize(i.value().m_Size)));
      FillTree(i.value().m_Children, m_ToplevelItem);
    }
  }
}

