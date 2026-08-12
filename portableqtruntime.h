#ifndef PORTABLEQTRUNTIME_H
#define PORTABLEQTRUNTIME_H

#include <QtGlobal>

inline void configurePortableQtRuntime()
{
#if defined(Q_OS_WIN)
  // A system-wide Qt/Anaconda environment can otherwise take precedence over
  // qt.conf and load an ABI-incompatible platform plugin.  Clear only plugin
  // search overrides; QT_QPA_PLATFORM remains available for diagnostics.
  qunsetenv("QT_PLUGIN_PATH");
  qunsetenv("QT_QPA_PLATFORM_PLUGIN_PATH");

  // qt.conf beside each executable resolves plugins without converting the
  // installation path through the active Windows code page.
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    qputenv("QT_QPA_PLATFORM", "windows:dpiawareness=0");
#endif
}

#endif
