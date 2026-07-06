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

    
    bool hasInit() {return m_hasInit;}
    bool hasDataAllocator() {return m_hasDataAllocator;}
    bool hasSliceProcessor() {return m_hasSliceProcessor;}
    bool hasVolumeProcessor() {return m_hasVolumeProcessor;}

  public slots :
    void initScript();
    void process_volume();
    void process_slice(uchar*, ushort*, int, int, int);
    
  signals :
    void initDone(QString);
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

 public slots :
  void processSlice(uchar*, ushort*, int, int, int);
  void processVolume();
  void initDone(QString);
  void sliceProcessed();
  void volumeProcessed();

 signals :
  void pyWidgetClosed();
  void initScript();
  void process_volume();
  void process_slice(uchar*, ushort*, int, int, int);
    
 private slots :
   void closeEvent(QCloseEvent*);
   void runCommand(QString, QHash<QString, QVariant>);
  
 private :
   QString m_fileName;
   QString m_maskName;

   PyWidgetMenu *m_menu;
  
   QStringList m_scripts;
  
   MyThread* m_thread;
   PyWorker* m_worker;

  void loadScripts();
  void populateArguments();
};

#endif
