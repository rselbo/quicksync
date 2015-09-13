#include "PreCompile.h"
#include "filestabledialog.h"
#include "utils.h"

CopiedFilesDialog_c::CopiedFilesDialog_c(QWidget* i_pcParent) :
  QWidget(i_pcParent),
  m_TempStorage(),
  m_TempWriter(&m_TempStorage)
{
  setupUi(this);
  setWindowFlags(Qt::Tool);

  QSettings settings;
  //Load window size and position
  settings.beginGroup("Windows/CopiedFilesWindow");
  QSize size = settings.value("size", QSize(200, 200)).toSize();
  std::max(size.height(), 200);
  std::max(size.width(), 200);
  QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
  std::max(pos.x(), 0);
  std::max(pos.y(), 0);

  resize(size);
  move(pos);
  settings.endGroup();

  m_TempStorage.reserve(1<<14);
  m_TempWriter.open(QIODevice::ReadWrite);

  m_Timer.setInterval(1000);
  connect(&m_Timer, SIGNAL(timeout()), SLOT(timerTick()));
  m_Timer.start();
}

CopiedFilesDialog_c::~CopiedFilesDialog_c()
{
}

void CopiedFilesDialog_c::closeEvent(QCloseEvent* pcEvent)
{
  QSettings settings;
  settings.beginGroup("Windows/CopiedFilesWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  pcEvent->ignore();
  hide();
}

void CopiedFilesDialog_c::saveWindowPos()
{
  QSettings settings;
  settings.beginGroup("Windows/CopiedFilesWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.endGroup();
}

void CopiedFilesDialog_c::AddFile(const QString& i_cFilename, const QDateTime& i_cModified, bool /*deleted*/, bool /*success*/)
{
  QString str = QString("%1 %2\n").arg(i_cModified.toString()).arg(i_cFilename);
  quint64 s = m_TempWriter.write(str.toUtf8());
}

void CopiedFilesDialog_c::timerTick()
{
  if(!m_TempStorage.isEmpty() && m_TempStorage.at(0) != '\0')
  {
    textEdit->append(m_TempStorage);
    m_TempWriter.seek(0);
    m_TempStorage.fill('\0');
  }
}