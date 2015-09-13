#ifndef PATCHER_H_
#define PATCHER_H_
#include "ui_patcher.h"

class Patcher : public QDialog, public Ui_QuicksyncPatcher
{
  enum PatcherState_e { e_Idle, e_ConnectingToHost, e_MD5Sums, e_NewDownload, e_FileDownloading, e_Error};
  Q_OBJECT
public:
  Patcher();
  virtual ~Patcher();  

  void handleMD5Sums();
  bool verifyMD5(const QString& filename, const QString& remoteMD5Str, bool verifyDownload=false);
  bool verifyHeader(const QString& headerMD5Str, const QString& headerStr);

  void accept();
public slots:
  void handleStateChange();
  void slotDataReadProgress ( int done, int total );
  void slotRequestFinished ( int id, bool error );

private:
  QString m_ServerURI;
  QString m_Path;
  PatcherState_e m_State;
  int m_CurrentRequest;
  QHttp m_Http;
  QBuffer m_Buffer;
  QFile* m_FileBuffer;
  QList<QPair<QString, QString> > m_DownloadList;
  bool m_bCloseOnExit;
  bool m_needReboot;
};

#endif //PATCHER_H_