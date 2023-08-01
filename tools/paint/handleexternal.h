#ifndef HANDLE_EXTERNAL_H
#define HANDLE_EXTERNAL_H

#include <QObject>
#include <QUdpSocket>

#include <QString>

class HandleExternalCMD : public QObject
{
  Q_OBJECT

 public :
  HandleExternalCMD(int port=7770);

  public slots :
    void readSocket();
  
 signals :
    void loadRAW(QString);
    void loadSc(QString);

    
 private :
  QUdpSocket *m_listeningSocket;
  int m_port;

  void initSocket();
  void processCommand(QString);

};

#endif
