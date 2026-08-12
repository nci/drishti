#ifndef TIFFINPUTROUTING_H
#define TIFFINPUTROUTING_H

#include <QStringList>

namespace TiffInputRouting
{
inline QStringList standardImageNameFilters()
{
  return QStringList()
    << QStringLiteral("*.bmp")
    << QStringLiteral("*.gif")
    << QStringLiteral("*.jpg")
    << QStringLiteral("*.jpeg")
    << QStringLiteral("*.png")
    << QStringLiteral("*.pbm")
    << QStringLiteral("*.pgm")
    << QStringLiteral("*.ppm")
    << QStringLiteral("*.tif")
    << QStringLiteral("*.tiff")
    << QStringLiteral("*.xbm")
    << QStringLiteral("*.xpm");
}

bool allFilesAreTiff(const QStringList& fileNames);
bool directoryContainsOnlyTiffImages(const QString& directoryName);
bool allFilesAreNativeGrayscaleTiff(const QStringList& fileNames);
bool directoryContainsOnlyNativeGrayscaleTiffImages(
  const QString& directoryName);

int routedPluginIndex(int selectedPluginIndex,
                      const QStringList& pluginDescriptions,
                      const QStringList& pluginLibraries,
                      const QStringList& inputs,
                      bool directoryInput);
}

#endif
