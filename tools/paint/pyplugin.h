#ifndef PYPLUGIN_H
#define PYPLUGIN_H

#include <QObject>

#include "pyplugininterface.h"

class PyPlugin : public QObject
{
  Q_OBJECT

  public :
    PyPlugin();
    ~PyPlugin();

    void clear();
    bool init(QString, QString, uchar*, ushort*, uchar*, int, int, int);
    QString scriptName();

    bool hasInit();
    bool hasDataAllocator();
    bool hasSliceProcessor();
    bool hasVolumeProcessor();

  public slots :
    void initScript();
    bool process_volume();
    bool process_slice(uchar*, ushort*, int, int, int);
    void setMask(ushort*);
    void populateArguments(QHash<QString, QVariant>);
    
  signals :
    void initDone(QString);
    void sliceProcessed();
    void volumeProcessed();
    void sliceProcessingDone();
    void volumeProcessingDone();
    void finished();

  private :
    PyPluginInterface* m_plugin;
    QString m_script;
    bool m_hasInit;
    bool m_hasDataAllocator;
    bool m_hasSliceProcessor;
    bool m_hasVolumeProcessor;
};

#endif // PYPLUGIN_H