#include "PreCompile.h"
#include "patcher.h"
#include "syncrules.h"
#include "utils.h"
#include <process.h>

Patcher::Patcher() : m_State(e_Idle), m_CurrentRequest(0), m_bCloseOnExit(true), m_needReboot(false)
{
  setupUi(this);

  //Load patcher settings into member variables
  QSettings settings;
  settings.beginGroup("patcher");
  QString patchURI = settings.value("hostname", "").toString();
  QString patchPort = settings.value("port", "0").toString();
  m_bCloseOnExit = settings.value("autoclose", true).toBool();
  settings.endGroup();

  quint16 patcherPort = static_cast<quint16>(patchPort.toInt());
  if(patcherPort == 0)
    patcherPort = 80;

  
  if(patchURI.isEmpty())
  {
    QTimer::singleShot(10000, this, SLOT(accept()));
    textEdit->setText("No patchserver defined, use the settings in quicksync to define one or disable patching. This dialog will close in 10 seconds.");
  }
  else
  {
    connect(&m_Http, SIGNAL(dataReadProgress(int, int)), SLOT(slotDataReadProgress(int, int)));
    connect(&m_Http, SIGNAL(requestFinished(int, bool)), SLOT(slotRequestFinished(int, bool)));

    //get the server URI without the protocol string
    m_ServerURI = patchURI;
    m_ServerURI.remove("http://");

    //if it has a path strip it off the server string and store it in m_Path
    int pos = m_ServerURI.indexOf('/');
    if(pos != -1)
    {
      m_Path = m_ServerURI.mid(pos);
      if(!m_Path.endsWith('/'))
        m_Path = m_Path + '/';
      m_ServerURI = m_ServerURI.left(pos);
    }

    m_State = e_ConnectingToHost;
    m_Http.setHost(m_ServerURI, patcherPort);
    m_CurrentRequest = m_Http.currentId();
    textEdit->append(QString("Trying to connect to http://%1").arg(m_ServerURI));
  }
}

Patcher::~Patcher()
{
}

void Patcher::slotDataReadProgress ( int done, int total )
{
  qDebug() << "[Patcher.slotDataReadProgress] " << done << ", " << total <<")";
  //Update the progressbar
  progressBar->setRange(0, total);
  progressBar->setValue(done);
}

void Patcher::slotRequestFinished ( int id, bool error )
{
  qDebug() << "[Patcher.requestFinished] " << id<< ", " << error <<")";

  //This slot should not block for long since the underlying code will resend it if this function does not return.
  //That is why the handleStateChange is delayed 1 ms, to avoid the dialog box to cause a resend of RequestFinished
  if(!error)
  {
    QTimer::singleShot(1, this, SLOT(handleStateChange()));
  }
  else
  {
    m_State = e_Error;
    QTimer::singleShot(1, this, SLOT(handleStateChange()));
    qDebug() << "[Patcher.requestFinished] " << QString("error: %1").arg(m_Http.errorString());
  }
}

void Patcher::handleStateChange()
{
  switch(m_State)
  {
    case e_ConnectingToHost:
    {
      m_State = e_MD5Sums;
      QString get = joinPath(m_Path, "md5sums");
      m_CurrentRequest = m_Http.get(get, &m_Buffer);
      break;
    }
    case e_MD5Sums:
    {
      //we have connected
      textEdit->append(QString("Connected to http://%1").arg(m_ServerURI));
      QString get = joinPath(m_Path, "md5sums");
      textEdit->append(QString("Downloading %1").arg(get));
      handleMD5Sums();
      break;
    }
    case e_NewDownload:
    {
      //check if we have more files to download
      if(!m_DownloadList.empty())
      {
        QString fileName = m_DownloadList.at(0).first;
        if(!fileName.endsWith(".qsr"))
        { //we dont need to restart quicksync for the rules definition
          m_needReboot = true;
        }
        m_FileBuffer = new QFile(fileName + ".tmp");
        if(!m_FileBuffer->open(QIODevice::ReadWrite))
        {
          m_State = e_Error;
          handleStateChange();
          return;
        }
        QString get = joinPath(m_Path, fileName);
        m_State = e_FileDownloading;
        m_Http.get(get, m_FileBuffer);
        textEdit->append(QString("Downloading %1").arg(get));
      }
      else
      {
        if(m_bCloseOnExit)
        {
          accept(); //press ok
        }
      }
      break;
    }
    case e_FileDownloading:
    {
      m_FileBuffer->close();
      delete m_FileBuffer;
      m_FileBuffer = NULL;
      QString fileName = m_DownloadList.at(0).first;
      if(verifyMD5(fileName+".tmp", m_DownloadList.at(0).second, true))
      {
        if(QFile::exists(fileName) && !QFile::rename(fileName, fileName+".old"))
        {
          textEdit->append(QString("Could not rename %1 to %1.old, this is needed to swap in the new version of the file we just downloaded").arg(fileName));
          return;
        }
        if(!QFile::rename(fileName+".tmp", fileName))
        {
          textEdit->append(QString("Could not rename %1.tmp to %1, this means the new version of the file is still called .tmp and quicksync will most likely not work properly after the restart. Please try to rename the file manually").arg(fileName));
          return;
        }
        if(fileName.endsWith(".qsr"))
        {
          //quicksync ignore rules. Ask if the user wants to use it
          int reply = QMessageBox::question(this, "Quicksync Ignore Rules", "A new ignore rules definition is available, do you want to use it?<br>\
                                                                Yes will overwrite the current rules, No will do nothing<br><br>\
                                                                WARNING: Answering Yes will overwrite any local changes with the new rules",
                                                                QMessageBox::Yes|QMessageBox::No,
                                                                QMessageBox::Yes);
          if(reply == QMessageBox::Yes)
          {
            SyncRules rules;
            //rules.importRules(fileName);
            rules.save();
          }
        }
      }
      m_DownloadList.removeFirst();
      m_State = e_NewDownload;
      handleStateChange();
      break;
    }
    case e_Error:
    {
      m_needReboot = false;
      textEdit->append(QString("An error occured: %1").arg(m_Http.errorString()));
      break;
    }
  }
}

void Patcher::handleMD5Sums()
{
  QTextStream textStream(m_Buffer.data());
  QString headerMD5Str;
  QString headerString;
  //read the header and verify
  textStream >> headerMD5Str >> headerString;
  if(!verifyHeader(headerMD5Str, headerString))
  {
    //not a proper md5sums, dump the content in the text edit, usually a 404 message
    textEdit->append(QString(m_Buffer.data()));
    return;
  }
  //We are appy with the md5
  textEdit->append(QString("Successfully downloaded a proper md5sums file"));

  QString fileMD5;
  QString fileName;
  //loop while we have lines to read
  while(textStream.status() == QTextStream::Ok)
  {
    textStream >> fileMD5 >> fileName;
    if(fileMD5.isEmpty() || fileName.isEmpty())
      break; //no more data, break out of the loop
    if(!verifyMD5(fileName, fileMD5))
    {
      //MD5 not verified. add to download
      m_DownloadList.append(QPair<QString, QString>(fileName, fileMD5));
    }
    //check if a old version of the file exists and try to delete it
    if(QFile::exists(fileName+".old") && !QFile::remove(fileName+".old"))
    {
      textEdit->append(QString("Could not remove %1.old, this is not a problem now but it could cause a problem for the next patch, try to delete it manually").arg(fileName));
    }

  }
  //go into download state 
  m_State = e_NewDownload;
  handleStateChange();
}

bool Patcher::verifyMD5(const QString& fileName, const QString& remoteMD5Str, bool verifyDownload)
{
  //Check if file exists
  if(QFile::exists(fileName))
  {
    //File is here, try to open and read it
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly))
    {
      //File is open, read it
      QByteArray fileData = file.readAll();
      QCryptographicHash md5Hasher(QCryptographicHash::Md5);
      md5Hasher.addData(fileData);
      QByteArray localMD5 = md5Hasher.result();
      QByteArray remoteMD5 = QByteArray::fromHex(remoteMD5Str.toLatin1());

      //now we have both the remote and local md5, check if they match or if they need to be downloaded
      if(localMD5 == remoteMD5)
      {
        //we have a match, nothing needs to be done
        if(verifyDownload)
        {
          textEdit->append(QString("%1 was successfully downloaded").arg(fileName.left(fileName.length()-4)));
          return true;
        }
        else
        {
          textEdit->append(QString("%1 is up to date").arg(fileName));
          return true;
        }
      }
      else
      {
        //MD5 has changed. Add the file to the download queue
        //textEdit->append(QString("File %1 has changed MD5 Local:%2 Remote:%3").arg(filename).arg(QString(localMD5.toHex())).arg(QString(remoteMD5.toHex())));
        textEdit->append(QString("New version of %1 is available, adding to download list").arg(fileName));
        return false;
      }
    }
    else
    {
      //could not open file, raise exception?
      return false;
    }
  }
  else
  {
    //file does not exist so the MD5 is 0
    textEdit->append(QString("No version of %1 was found locally, adding to download list").arg(fileName));
    return false;
  }
}

//The header of a proper md5sums file contains the md5hash of the string "qspatch-header" followed by the string "qspatch-header"
bool Patcher::verifyHeader(const QString& headerMD5Str, const QString& headerStr)
{
  //create a bytearray of the md5hash
  QByteArray md5 = QByteArray::fromHex(headerMD5Str.toLatin1());
  //md5 the string and get the bytearray of the md5
  QCryptographicHash md5Hasher(QCryptographicHash::Md5);
  md5Hasher.addData(headerStr.toLatin1());
  QByteArray resultmd5 = md5Hasher.result();
  //if the 2 md5hashes are the same this looks like a proper md5sums file
  if(resultmd5 == md5)
    return true;
  //not equal, not a proper file
  return false;
}

void Patcher::accept()
{
  if(m_needReboot)
  {
    QMessageBox::information(this, "Patching done", "Quicksync was sucessfully patched.");
    _flushall();
    system("syncclient.exe");

    exit(0);
  }
  QDialog::accept();
}
