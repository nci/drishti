#ifndef PYWIDGET_H
#define PYWIDGET_H

#include <pybind11/embed.h> // Everything needed for embedding

#include <QWidget>
#include <QProcess>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QThread>

#include "pywidgetmenu.h"

namespace py = pybind11;

class MyThread : public QThread {
  Q_OBJECT
  protected:
    void run() override {
      py::gil_scoped_acquire gil;
      exec(); // Start event loop
    }
};

class PyWorker : public QObject
{
  Q_OBJECT

  public :
    PyWorker(QString);
    ~PyWorker() {}

  public slots :
    void initScript();
    void process_slice(int);
    void process_volume();

  signals :
    void initDone();
    void sliceProcessed();
    void volumeProcessed();
    void finished();

  private :
    QString m_script;
    bool m_hasInit;
    bool m_hasDataAllocator;
    bool m_hasSliceProcessor;
    bool m_hasVolumeProcessor;
};

class PyWidget : public QWidget
{
  Q_OBJECT

 public :
  PyWidget(QWidget *parent=0);
  ~PyWidget();

  void setFilename(QString);
  void setSize(int, int, int);
  void setVolPtr(uchar*);
  void setMaskPtr(uchar*);

 public slots :
  void processSlice(int);
  void initDone();
  void sliceProcessed();
  void volumeProcessed();

 signals :
  void pyWidgetClosed();
  void process_slice(int);
  void process_volume();
    
 private slots :
   void closeEvent(QCloseEvent*);
   void runCommand(QString);
  
 private :
   QString m_fileName;
   QString m_maskName;
   
   int m_d, m_w, m_h;
   uchar *m_volPtr;
   uchar *m_maskPtr;

   PyWidgetMenu *m_menu;
  
   QStringList m_scripts;
  
   MyThread* m_thread;
   PyWorker* m_worker;

  void loadScripts();
};

#endif
