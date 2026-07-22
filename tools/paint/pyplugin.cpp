#include "pyplugin.h"
#include <QPluginLoader>
#include <QMessageBox>
#include <iostream>

PyPlugin::PyPlugin()
{
    m_plugin = nullptr;
    m_script = "";
    m_hasInit = false;
    m_hasDataAllocator = false;
    m_hasSliceProcessor = false;
    m_hasVolumeProcessor = false;
}

PyPlugin::~PyPlugin()
{
    if (m_plugin)
        {
            m_plugin->clear();
            delete m_plugin;
        }
    m_plugin = nullptr;
}

QString PyPlugin::scriptName() { return m_script; }
bool PyPlugin::hasInit() { return m_hasInit;}
bool PyPlugin::hasDataAllocator() { return m_hasDataAllocator; }
bool PyPlugin::hasSliceProcessor() { return m_hasSliceProcessor; }
bool PyPlugin::hasVolumeProcessor() { return m_hasVolumeProcessor; }

void PyPlugin::clear()
{
    if (m_plugin)
    {
        m_plugin->clear();
        delete m_plugin;
        m_plugin = nullptr;
    }
}

bool 
PyPlugin::init(QString pluginflnm, QString script, 
               uchar* data, ushort* mask, uchar* lut, uchar* tag, 
               int width, int height, int depth)
{
    QPluginLoader loader(pluginflnm);

    QObject *plugin = loader.instance();

    if (plugin)
    {
        m_plugin = qobject_cast<PyPluginInterface*>(plugin);
        if (m_plugin)
        {
            m_plugin->init(script, data, mask, lut, tag, width, height, depth);
            std::cout << "Python version loaded - " << pluginflnm.toLatin1().data() << "\n";
            return true;
        }
    }   

    QMessageBox::information(nullptr, "Plugin", "Failed to load python version : " + pluginflnm + "\nfor script: " + script);
    std::cout << "** Failed to load python version - " << pluginflnm.toLatin1().data() << "\n";

    return false;
}

void 
PyPlugin::initScript()
{
    if (m_plugin)
        {
            QString mesg = m_plugin->initScript();
            m_script = m_plugin->scriptName();
            m_hasInit = m_plugin->hasInit();
            m_hasDataAllocator = m_plugin->hasDataAllocator();
            m_hasSliceProcessor = m_plugin->hasSliceProcessor();
            m_hasVolumeProcessor = m_plugin->hasVolumeProcessor();
            emit initDone(mesg);
        }
}

bool 
PyPlugin::process_volume()
{
    if (m_plugin)
        return m_plugin->process_volume();

    return false;
}

bool 
PyPlugin::process_slice(uchar* data, ushort* mask, int width, int height, int depth)
{
    if (m_plugin)
        return m_plugin->process_slice(data, mask, width, height, depth);

    return false;
}

void 
PyPlugin::setMask(ushort* mask)
{
    if (m_plugin)
        m_plugin->setMask(mask);
}

void 
PyPlugin::populateArguments(QHash<QString, QVariant> args)
{
    if (m_plugin)
        m_plugin->populateArguments(args);
}

