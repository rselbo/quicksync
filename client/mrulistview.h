#ifndef QUICKSYNC_MRULISTVIEW_H
#define QUICKSYNC_MRULISTVIEW_H

#include "ui_mruwidget.h"

struct MRUInfo 
{
  QString filename; 
  bool binary;
  bool executable;
};
typedef QList<MRUInfo> MRUList_t;

class MRUListView_c : public QWidget, private Ui::MruWidget
{
  Q_OBJECT
public:
  MRUListView_c(int i_nMaxMRU, QWidget* i_pcParent=NULL);
  ~MRUListView_c();

  void closeEvent(QCloseEvent*);

  void setData(const MRUList_t& i_cMRUList);
  void moveItem(int i_nFrom, int i_nTo);
  void addFile(const QString& i_cFilename);
  void removeLast();

public slots:
  void saveWindowPos();

signals:
  void closing();
private:
  int m_nMaxMRU;
};

#endif //QUICKSYNC_MRULISTVIEW_H
