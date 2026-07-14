#ifndef PYPLUGININTERFACE_H
#define PYPLUGININTERFACE_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>

class PyPluginInterface
{
  public :
    virtual ~PyPluginInterface() {}

    virtual void clear() = 0;
    virtual void init(QString, uchar*, ushort*, uchar*, int, int, int) = 0;
    virtual QString scriptName() = 0;

    virtual bool hasInit() = 0;
    virtual bool hasDataAllocator() = 0;
    virtual bool hasSliceProcessor() = 0;
    virtual bool hasVolumeProcessor() = 0;

    virtual QString initScript() = 0;
    virtual bool process_volume() = 0;
    virtual bool process_slice(uchar*, ushort*, int, int, int) = 0;
    virtual void setMask(ushort*) = 0;
    virtual void populateArguments(QHash<QString, QVariant>) = 0;
};

Q_DECLARE_INTERFACE(PyPluginInterface,
		    "drishti.paint.Plugin.PyPluginInterface/1.0")

#endif // PYPLUGININTERFACE_H