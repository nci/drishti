#include "tiffinputrouting.h"
#include "tiffpagevalidation.h"

#include <QDir>
#include <QFileInfo>

namespace
{
bool isTiffFileName(const QString& fileName)
{
  const QString suffix = QFileInfo(fileName).suffix();
  return suffix.compare(QStringLiteral("tif"), Qt::CaseInsensitive) == 0 ||
         suffix.compare(QStringLiteral("tiff"), Qt::CaseInsensitive) == 0;
}

bool appendNativeGrayscaleTiffPages(
  const QString& fileName,
  TiffPageValidation::PageMetadata *baseline,
  bool *haveBaseline)
{
  const QFileInfo fileInfo(fileName);
  if (!baseline || !haveBaseline || !isTiffFileName(fileName) ||
      !fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
    return false;

  QVector<TiffPageValidation::PageMetadata> pages;
  QString error;
  if (!TiffPageValidation::inspectFile(fileName, &pages, &error))
    return false;

  for (const TiffPageValidation::PageMetadata& page : pages)
    {
      if (!*haveBaseline)
        {
          *baseline = page;
          *haveBaseline = true;
          continue;
        }
      QString difference;
      if (!TiffPageValidation::samePageLayout(*baseline, page, &difference))
        return false;
    }
  return !pages.isEmpty();
}

bool isExpectedPluginLibrary(const QString& libraryPath,
                             const QString& expectedBaseName)
{
  QString baseName = QFileInfo(libraryPath).completeBaseName().toLower();
  if (baseName.startsWith(QStringLiteral("lib")))
    baseName.remove(0, 3);

  const QString expected = expectedBaseName.toLower();
  return baseName == expected || baseName == expected+QStringLiteral("d");
}
}

bool
TiffInputRouting::allFilesAreTiff(const QStringList& fileNames)
{
  if (fileNames.isEmpty())
    return false;

  for (const QString& fileName : fileNames)
    if (!isTiffFileName(fileName))
      return false;

  return true;
}

bool
TiffInputRouting::directoryContainsOnlyTiffImages(
  const QString& directoryName)
{
  const QDir directory(directoryName);
  if (!directory.exists())
    return false;

  const QFileInfoList images = directory.entryInfoList(
    standardImageNameFilters(),
    QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable | QDir::Files,
    QDir::NoSort);
  if (images.isEmpty())
    return false;

  for (const QFileInfo& image : images)
    if (!isTiffFileName(image.fileName()))
      return false;

  return true;
}

bool
TiffInputRouting::allFilesAreNativeGrayscaleTiff(
  const QStringList& fileNames)
{
  if (fileNames.isEmpty())
    return false;

  TiffPageValidation::PageMetadata baseline;
  bool haveBaseline = false;
  for (const QString& fileName : fileNames)
    if (!appendNativeGrayscaleTiffPages(fileName, &baseline, &haveBaseline))
      return false;

  return haveBaseline;
}

bool
TiffInputRouting::directoryContainsOnlyNativeGrayscaleTiffImages(
  const QString& directoryName)
{
  const QDir directory(directoryName);
  if (!directory.exists())
    return false;

  const QFileInfoList images = directory.entryInfoList(
    standardImageNameFilters(),
    QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable | QDir::Files,
    QDir::NoSort);
  if (images.isEmpty())
    return false;

  QStringList fileNames;
  for (const QFileInfo& image : images)
    {
      if (!isTiffFileName(image.fileName()))
        return false;
      fileNames.append(image.absoluteFilePath());
    }

  if (!allFilesAreNativeGrayscaleTiff(fileNames))
      return false;

  return true;
}

int
TiffInputRouting::routedPluginIndex(
  int selectedPluginIndex,
  const QStringList& pluginDescriptions,
  const QStringList& pluginLibraries,
  const QStringList& inputs,
  bool directoryInput)
{
  if (selectedPluginIndex < 0 ||
      selectedPluginIndex >= pluginDescriptions.size() ||
      selectedPluginIndex >= pluginLibraries.size())
    return selectedPluginIndex;

  const QString imageStackDescription = directoryInput ?
    QStringLiteral("Standard Image Directory") :
    QStringLiteral("Standard Image Files");
  if (pluginDescriptions[selectedPluginIndex] != imageStackDescription ||
      !isExpectedPluginLibrary(pluginLibraries[selectedPluginIndex],
                               QStringLiteral("imagestackplugin")))
    return selectedPluginIndex;

  const bool allTiff = directoryInput ?
    (inputs.size() == 1 &&
     directoryContainsOnlyNativeGrayscaleTiffImages(inputs[0])) :
    allFilesAreNativeGrayscaleTiff(inputs);
  if (!allTiff)
    return selectedPluginIndex;

  const QString tiffDescription = directoryInput ?
    QStringLiteral("Grayscale TIFF Image Directory") :
    QStringLiteral("Grayscale TIFF Image Files");
  const int count = qMin(pluginDescriptions.size(), pluginLibraries.size());
  for (int index=0; index<count; ++index)
    if (pluginDescriptions[index] == tiffDescription &&
        isExpectedPluginLibrary(pluginLibraries[index],
                                QStringLiteral("tiffplugin")))
      return index;

  return selectedPluginIndex;
}
