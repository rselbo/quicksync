#ifndef QUICKSYNC_COPIEDFILESDIALOG_H
#define QUICKSYNC_COPIEDFILESDIALOG_H

#include "ui_copiedfilesdialog.h"

class CopiedFilesDialog_c : public QWidget, private Ui::CopiedFilesWidget
{
  Q_OBJECT
public:
  CopiedFilesDialog_c(QWidget* i_pcParent=NULL);
  ~CopiedFilesDialog_c();

  void closeEvent(QCloseEvent*);
  void AddFile(const QString& i_cFilename, const QDateTime& i_cModified, bool deleted, bool success);
public slots:
  void saveWindowPos();
  void timerTick();
protected:
private:
  QByteArray m_TempStorage;
  QBuffer m_TempWriter;
  QTimer m_Timer;
};


#endif //QUICKSYNC_COPIEDFILESDIALOG_H
