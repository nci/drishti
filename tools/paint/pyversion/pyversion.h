#ifndef PYVERSION_H
#define PYVERSION_H

#include "pyplugininterface.h"

class PyVersion : public QObject, public PyPluginInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.paint.Plugin.PyPluginInterface/1.0")
  Q_INTERFACES(PyPluginInterface)

  public :
    void clear();

    void init(QString, uchar*, ushort*, uchar*, int, int, int);
    QString scriptName();

    bool hasInit();
    bool hasDataAllocator();
    bool hasSliceProcessor();
    bool hasVolumeProcessor();

    QString initScript();
    bool process_volume();
    bool process_slice(uchar*, ushort*, int, int, int);
    void setMask(ushort*);
    void populateArguments(QHash<QString, QVariant>);

  private :
    QString m_script;
    bool m_hasInit;
    bool m_hasDataAllocator;
    bool m_hasSliceProcessor;
    bool m_hasVolumeProcessor;
};

#endif // PYVERSION_H