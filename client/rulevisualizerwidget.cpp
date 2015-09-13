#include "PreCompile.h"
#include "rulevisualizerwidget.h"
#include "clientsettings.h"
#include "utils.h"
#include "rulevisualizerworker.h"

RuleVisualizerWidget::RuleVisualizerWidget(QWidget* parent) :
  QWidget(parent),
  m_WorkerThread(NULL),
  m_CurrentRuleItem(NULL)
{
  setupUi(this);
  QSettings settings;
  //Select the currently selected branch in the main gui
  QString currentSelectedBranch = settings.value("CurrentlySelectedBranch").toString();

  //Load currently configured branches
  settings.beginGroup(ClientSettings::branchesStr);
  QStringList branches = settings.childGroups();
  branchSelector->addItems(branches);

  int index = branchSelector->findText(currentSelectedBranch);
  index = std::max<int>(index, 0);
  branchSelector->setCurrentIndex(index);

  settings.beginGroup(branchSelector->currentText());
  m_SourcePath = settings.value(ClientSettings::srcPathStr).toString();
  settings.endGroup(); //end group branchSelector->currentText()
  settings.endGroup(); //end group ClientSettings::branchesStr

  treeWidget->setColumnWidth(0, 200);
  m_WorkTimer.setInterval(1000);

  connect(this, SIGNAL(signalSetSourcePath(const QString&)), ruleWidget, SLOT(slotSourcePath(const QString&)));
  connect(this, SIGNAL(signalNewRuleLocations(const QStringList&)), ruleWidget, SLOT(slotNewRuleLocations(const QStringList&)));
}

RuleVisualizerWidget::~RuleVisualizerWidget()
{
  if(m_WorkerThread)
  {
    m_WorkerThread->StopThread();
    m_WorkerThread->wait();
    delete m_WorkerThread;
  }
}

///slot called when the worker thread has completed the scan of the filesystem 
void RuleVisualizerWidget::slotScanComplete()
{
  //emit scanComplete signal so the working dialog box can close
  emit scanComplete();
  m_WorkTimer.stop();
  treeWidget->clear();
  QTreeWidgetItem* toplevelItem = m_WorkerThread->GetToplevelItem();
  treeWidget->addTopLevelItem(toplevelItem);
  m_WorkerThread->FillExtensions(tableExtensionSize);
  tableExtensionSize->resizeRowsToContents();
  emit signalNewRuleLocations(m_WorkerThread->GetRuleLocations());

  delete m_WorkerThread;
  m_WorkerThread = NULL;
}

///slot called when the refresh button is called. It will start a new scan and show the working dialog.
void RuleVisualizerWidget::on_buttonRefresh_clicked()
{
  tableExtensionSize->setRowCount(0);
  treeWidget->clear();
  m_WorkTimer.start();

  if(!m_SourcePath.isEmpty())
  {
    //Create the working dialog box, hook up the signals and show it (it is modal)
    QDialog* dialog = new WorkingDialog(this);
    connect(&m_WorkTimer, SIGNAL(timeout()), dialog, SLOT(slotTick()));
    connect(this, SIGNAL(scanComplete()), dialog, SLOT(accept()));
    connect(dialog, SIGNAL(signalCancel()), this, SLOT(slotScanCanceled()));
    dialog->show();

    //Create the new thread, connect to its completed signal and start the thread
    m_WorkerThread = new RuleVisualizerWorker(m_SourcePath, branchSelector->currentText());
    connect(m_WorkerThread, SIGNAL(signalScanComplete()), this, SLOT(slotScanComplete()));
    m_WorkerThread->start();
    m_WorkerThread->moveToThread(m_WorkerThread);
    emit signalSetSourcePath(m_SourcePath);
  }
}

void RuleVisualizerWidget::on_branchSelector_currentIndexChanged()
{
  QSettings settings;
  //Load currently configured branches
  settings.beginGroup(ClientSettings::branchesStr);
  settings.beginGroup(branchSelector->currentText());
  m_SourcePath = settings.value(ClientSettings::srcPathStr).toString();
  settings.endGroup(); //end group branchSelector->currentText()
  settings.endGroup(); //end group ClientSettings::branchesStr

}

void RuleVisualizerWidget::on_treeWidget_signalItemRightClicked(const QModelIndex& item)
{
  //disabled for now. might work on it later
  //QMenu menu(treeWidget);
  //QString path = item.data(PathRole).toString();
  //QFileInfo fileInfo(joinPath(m_SourcePath,path));
  //if(fileInfo.isDir())
  //{
  //  menu.addAction("Set rule root", this, SLOT(actionSetRoot()));
  //  path = path.right(path.length() - std::max(0, path.lastIndexOf('/', 1)));
  //  menu.addAction(new RuleAction(QString("Exclude \"%1\"").arg(path), path, &menu, this, SLOT(actionExcludeDir(const QString&))));
  //}
  //else
  //{
  //  menu.addAction(QString("Exclude extension \"%1\"").arg(fileInfo.completeSuffix()));
  //  menu.addAction(QString("Include extension \"%1\"").arg(fileInfo.completeSuffix()));
  //}
  //menu.exec(QCursor::pos());
}

void RuleVisualizerWidget::actionSetRoot()
{
  QTreeWidgetItem* currentItem = treeWidget->currentItem();
  if(currentItem == NULL)
    return;

  QString path = currentItem->data(0, PathRole).toString();
  if(path == m_CurrentRulePath)
    return;

  QBrush selectedBrush(QColor(0, 255, 0));
  QBrush defaultBrush(QColor(255, 255, 255));

  if(m_CurrentRuleItem)
    m_CurrentRuleItem->setBackground(0, defaultBrush);

  currentItem->setBackground(0, selectedBrush);
  m_CurrentRuleItem = currentItem;
  m_CurrentRulePath = path;

  emit signalSelectedRuleRoot(path);
}

void RuleVisualizerWidget::actionExcludeDir(const QString& exclude)
{
  //int i=0; i=i;
}

void RuleVisualizerWidget::slotScanCanceled()
{
  if(m_WorkerThread)
  {
    m_WorkerThread->StopThread();
    m_WorkerThread->wait();
    delete m_WorkerThread;
    m_WorkerThread = NULL;
  }
}

WorkingDialog::WorkingDialog(QWidget* parent) : QDialog(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  m_WorkingText = new QLabel(this);
  m_TimeText = new QLabel(this);
  QPushButton* button = new QPushButton("Cancel", this);
  connect(button, SIGNAL(clicked()), this, SLOT(slotCancelClicked()));
  layout->addWidget(m_WorkingText);
  layout->addWidget(m_TimeText);
  layout->addWidget(button);
  setModal(true);
  m_StartWorkTime.start();
  slotTick();
}

void WorkingDialog::slotCancelClicked()
{
  emit signalCancel();
  accept();
}

void WorkingDialog::slotTick()
{
  static QString workStrings[] = {"Working.", "Working..", "Working..."};
  static int index = 0;

  QString time = QString("%1 seconds").arg(m_StartWorkTime.elapsed()/1000.0, 0, 'f', 0);

  m_WorkingText->setText(workStrings[index]);
  m_TimeText->setText(time);
  if(++index >= 3)
    index = 0;

}