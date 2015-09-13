#ifndef RULEVISUALIZERWIDGET_H
#define RULEVISUALIZERWIDGET_H

#include "ui_rulevisualizer.h"

class RuleVisualizerWorker;

class RuleVisualizerWidget : public QWidget, public Ui_RuleVisualizer
{
  Q_OBJECT
public:
  RuleVisualizerWidget(QWidget* parent=NULL);
  ~RuleVisualizerWidget();

public slots:
  void slotScanComplete();
  void on_buttonRefresh_clicked();
  void on_branchSelector_currentIndexChanged();
  void on_treeWidget_signalItemRightClicked(const QModelIndex& item);

  void actionSetRoot();
  void actionExcludeDir(const QString& exclude);
  void slotScanCanceled();
signals:
  void signalSetSourcePath(const QString& path);
  void signalNewRuleLocations(const QStringList& locations);
  void signalSelectedRuleRoot(const QString& path);
  void scanComplete();
protected:
  QString m_SourcePath;
  RuleVisualizerWorker* m_WorkerThread;
  QTimer m_WorkTimer;

  QString m_CurrentRulePath;
  QTreeWidgetItem* m_CurrentRuleItem;
};

class RuleAction : public QAction
{
  Q_OBJECT
public:
  RuleAction(const QString& text, const QString& data, QWidget* parent, const QObject *receiver, const char* member) : 
    QAction(text, parent),
    m_Data(data)
  {
    if(receiver != NULL && member != NULL)
      QObject::connect(this, SIGNAL(signalTriggered(const QString&)), receiver, member);
    connect(this, SIGNAL(triggered()), SLOT(slotTriggered()));
  };

protected slots:
  void slotTriggered()
  {
    emit signalTriggered(m_Data);
  }

signals:
  void signalTriggered(const QString& str);

private:
  QString m_Data;
};

class WorkingDialog : public QDialog
{
  Q_OBJECT
public:
  WorkingDialog(QWidget* parent);

public slots:
  void slotTick();
  void slotCancelClicked();
signals:
  void signalCancel();
private:
  QLabel* m_WorkingText;
  QLabel* m_TimeText;
  QTime m_StartWorkTime;
};
#endif //RULEVISUALIZERWIDGET_H

