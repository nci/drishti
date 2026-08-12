#ifndef METAIMAGEPATHUTILS_H
#define METAIMAGEPATHUTILS_H

#include <QDir>
#include <QFileInfo>
#include <QString>

namespace MetaImagePathUtils
{
inline QString elementDataFileReference(const QString& headerFileName,
                                        const QString& dataFileName)
{
  const QFileInfo headerInfo(headerFileName);
  const QFileInfo dataInfo(dataFileName);
  return QDir::fromNativeSeparators(
    headerInfo.absoluteDir().relativeFilePath(dataInfo.absoluteFilePath()));
}
}

#endif
