#ifndef SOURCEFILESPROVIDER_H
#define SOURCEFILESPROVIDER_H

#include <QStringList>
#include <QtPlugin>

// Optional plugin capability. Keeping provenance outside VolInterface avoids
// changing the long-lived import plugin vtable for legacy third-party DLLs.
class SourceFilesProvider
{
 public:
  virtual ~SourceFilesProvider() {}
  virtual QStringList sourceFiles() const = 0;
};

Q_DECLARE_INTERFACE(SourceFilesProvider,
                    "drishti.import.SourceFilesProvider/1.0");

#endif
