#include "handleexternal.h"


HandleExternalCMD::HandleExternalCMD(int port) :
  QObject()
{
  m_port = port;
  initSocket();
}

void
HandleExternalCMD::initSocket()
{
  m_listeningSocket = new QUdpSocket(this);
  if (m_listeningSocket->bind(QHostAddress::LocalHost, m_port))
    {
      connect(m_listeningSocket, SIGNAL(readyRead()),
	      this, SLOT(readSocket()),
	      Qt::DirectConnection);
    }
}

void
HandleExternalCMD::readSocket()
{
  while (m_listeningSocket->hasPendingDatagrams())
    {
      QByteArray datagram;
      datagram.resize(m_listeningSocket->pendingDatagramSize());
      QHostAddress sender;
      quint16 senderPort;

      m_listeningSocket->readDatagram(datagram.data(), datagram.size(),
				      &sender, &senderPort);

      processCommand(QString(datagram));
    }
}

void
HandleExternalCMD::processCommand(QString cmd)
{
  bool ok;
  QString ocmd = cmd;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  
  if (list[0] == "loadraw")
    {
      if (list.size() == 2)
	emit loadRAW(list[1]);
      return;
    } 

//  if (list[0] == "loadsc")
//    {
//      if (list.size() == 2)
//	emit loadSC(list[1]);
//      return;
//    } 
}
