#ifndef BITMAPTHREAD_H
#define BITMAPTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "volumefilemanager.h"

class BitmapThread : public QThread
{
  Q_OBJECT

public :
  BitmapThread(QObject *parent = 0);
  ~BitmapThread();

  void setFiles(QString, QString,
		int, int, int, int);
  void createBitmask(uchar*);
  QBitArray bitmask();

 signals :
  void progressChanged(int);
  void progressReset();

 protected:
  void run();

 private:
  QWaitCondition condition;
  QMutex mutex;
  bool m_interrupt;
  VolumeFileManager m_pvlFileManager;
  //VolumeFileManager m_gradFileManager;
  int m_depth, m_width, m_height;
  uchar m_lut[4*256*256];
  QBitArray m_bitmask;
};

#endif
