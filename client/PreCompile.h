// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#include <algorithm>
#include <stdio.h>
#include <stdexcept>
#include <math.h>

#ifdef CODE_ANALYSIS
  #include <codeanalysis\warnings.h>
  #pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#endif //CODE_ANALYSIS
#ifdef WINDOWS
#pragma warning( push, 1 )
#endif
#include <algorithm>

#include <QtCore/QtDebug>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QAbstractTableModel>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QHash>
#include <QtCore/QBuffer>
#include <QtCore/QCryptographicHash>
#include <QtCore/QTextStream>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtCore/QtGlobal>
#include <QtCore/QSharedPointer>
#include <QtXml/QXmlSimpleReader>
#include <QtXml/QXmlInputSource>
#include <QtXml/QDomDocument>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
//#include <QtNetwork/QHttp>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QComboBox>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QTableWidget>

#define PCRE_STATIC
#include "pcre.h"

#ifdef WINDOWS
#include "syncrules.h"

#pragma warning(pop)
#endif

extern const QString EmptyQString;