// Copyright (C) 2005 Jesper Hansen <jesper@jesperhansen.net>
// Content of this file is subject to the GPL v2
#ifndef QUICKSYNC_CLIENTAPP_H
#define QUICKSYNC_CLIENTAPP_H

//-----------------------------------------------------------------------------
class ClientWindow;

class ClientApp : public QApplication
{
  Q_OBJECT
public:
  ClientApp( int argc, char **argv );
  virtual ~ClientApp();

private slots:
  
private:
  ClientWindow *m_Window;
};

//-----------------------------------------------------------------------------

#endif
