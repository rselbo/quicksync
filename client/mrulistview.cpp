#include "PreCompile.h"
#include "mrulistview.h"

MRUListView_c::MRUListView_c(int i_nMaxMRU, QWidget* i_pcParent) :
  QWidget(i_pcParent),
  m_nMaxMRU(i_nMaxMRU)
{
  setupUi(this);
  setWindowFlags(Qt::Tool);

  QSettings settings;
  //Load window size and position
  settings.beginGroup("Windows/MRUWindow");
  resize(settings.value("size", QSize(180, 160)).toSize());
  move(settings.value("pos", QPoint(200, 200)).toPoint());
  settings.endGroup();
}

MRUListView_c::~MRUListView_c()
{
}

void MRUListView_c::closeEvent(QCloseEvent* pcEvent)
{
  emit closing();
  QWidget::closeEvent(pcEvent);
}

void MRUListView_c::saveWindowPos()
{
  QSettings settings;
  settings.beginGroup("Windows/MRUWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.endGroup();
}

void MRUListView_c::setData(const MRUList_t& i_cMRUList)
{
  listWidget->clear();
  MRUList_t::const_iterator ci = i_cMRUList.begin();
  MRUList_t::const_iterator end = i_cMRUList.end();
  for(int i=0; ci != end;ci++,i++)
  {
    listWidget->insertItem(0, ci->filename);
  }
  labelMRUFiles->setText(tr("%1/%2").arg(listWidget->count()).arg(m_nMaxMRU));
}

void MRUListView_c::moveItem(int i_nFrom, int i_nTo)
{
  QListWidgetItem* pcItem = listWidget->takeItem(i_nFrom);
  listWidget->insertItem(i_nTo, pcItem);
}

void MRUListView_c::addFile(const QString& i_cFilename)
{
  listWidget->insertItem(0, i_cFilename);
  labelMRUFiles->setText(tr("%1/%2").arg(listWidget->count()).arg(m_nMaxMRU));
}

void MRUListView_c::removeLast()
{
  delete listWidget->takeItem(listWidget->count()-1);
  labelMRUFiles->setText(tr("%1/%2").arg(listWidget->count()).arg(m_nMaxMRU));
}
