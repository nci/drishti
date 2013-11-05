#ifndef CHECKUPDATES_H
#define CHECKUPDATES_H

#include <QtGui>
#include <QtNetwork>

class CheckUpdate : public QObject
{
  Q_OBJECT

 public :
  CheckUpdate(QString);
  ~CheckUpdate();

 public slots :
  void kill();

 private slots :
  void finishedReading(int, bool);
  void readyRead(const QHttpResponseHeader&);

 private :
  QString m_versionString;
  QHttp *m_http;
};

#endif
