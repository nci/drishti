#ifndef PYWIDGET_H
#define PYWIDGET_H

#include <pyworker.h>
//#include <pybind11/embed.h> // Everything needed for embedding

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

class PyWidget : public QWidget
{
  Q_OBJECT

 public :
    PyWidget(QWidget *parent=0);
    ~PyWidget();
  
    void setFilename(QString);
    void init(uchar*, ushort*, uchar*, int, int, int);
    void setMask(ushort*);

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
    void volumeProcessingDone();
    void sliceProcessingDone();
  
 private slots :
    void closeEvent(QCloseEvent*);
    void runCommand(QString, QHash<QString, QVariant>);
  
 private :
    QString m_fileName;
    QString m_maskName;

    uchar *m_volume;
    ushort *m_mask;
    uchar *m_lut;
    int m_depth, m_width, m_height;

    PyWidgetMenu *m_menu;
  
    QStringList m_scripts;
  
    MyThread* m_thread;
    PyWorker* m_worker;

    void loadScripts();
    void populateArguments();
};

#endif
