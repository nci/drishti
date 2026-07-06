#ifndef PYWORKER_H
#define PYWORKER_H

#include <pybind11/embed.h> // Everything needed for embedding

#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>

class PyWorker : public QObject
{
  Q_OBJECT

  public :
    PyWorker(QString);
    ~PyWorker();

    void init(uchar*, ushort*, uchar*, int, int, int);
    QString scriptName();

    bool hasInit() {return m_hasInit;}
    bool hasDataAllocator() {return m_hasDataAllocator;}
    bool hasSliceProcessor() {return m_hasSliceProcessor;}
    bool hasVolumeProcessor() {return m_hasVolumeProcessor;}

  public slots :
    void initScript();
    void process_volume();
    void process_slice(uchar*, ushort*, int, int, int);
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
    QString m_script;
    bool m_hasInit;
    bool m_hasDataAllocator;
    bool m_hasSliceProcessor;
    bool m_hasVolumeProcessor;
};

#endif // PYWORKER_H