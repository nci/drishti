#ifndef PYWIDGET_H
#define PYWIDGET_H

#include <QWidget>
#include <QProcess>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QThread>

#include "pywidgetmenu.h"
#include "pyplugin.h"

class PyWidget : public QWidget
{
  Q_OBJECT

 public :
   PyWidget(QWidget *parent=0);
   ~PyWidget();
   void setPyVersion(QString);
   void setFilename(QString);
   void init(uchar*, ushort*, uchar*, uchar*, int, int, int);
   void setMask(ushort*);

 public slots :
   bool processSlice(uchar*, ushort*, int, int, int, bool);
   bool processVolume();
   void initDone(QString);

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
   QString m_pyversionflnm;
   QString m_fileName;
   QString m_maskName;

   uchar *m_volume;
   ushort *m_mask;
   uchar *m_lut;
   uchar *m_tag;
   int m_depth, m_width, m_height;

   PyWidgetMenu *m_menu;
  
   QStringList m_scripts;
  
   PyPlugin *m_plugin;

   void loadScripts();
   void populateArguments();
};

#endif
