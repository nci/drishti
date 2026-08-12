#include "../metaimagepathutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include <iostream>

namespace
{
int fail(const char *message)
{
  std::cerr << message << std::endl;
  return 1;
}

QString resolvedReference(const QString& headerFile,
                          const QString& reference)
{
  return QFileInfo(QFileInfo(headerFile).absoluteDir().filePath(reference))
    .absoluteFilePath();
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  QTemporaryDir temporary;
  if (!temporary.isValid())
    return fail("Cannot create the MetaImage path fixture directory");

  const QString headerDirectory = QDir(temporary.path()).filePath(
    QString::fromUtf8("headers \xe4\xb8\xad\xe6\x96\x87 space"));
  const QString rawDirectory = QDir(temporary.path()).filePath(
    QString::fromUtf8("raw \xe4\xb8\xad\xe6\x96\x87 space"));
  const QString headerFile = QDir(headerDirectory).filePath("volume.mhd");
  const QString sameDirectoryRaw = QDir(headerDirectory).filePath("volume.raw");
  const QString otherDirectoryRaw = QDir(rawDirectory).filePath("volume.raw");

  const QString sameReference =
    MetaImagePathUtils::elementDataFileReference(headerFile, sameDirectoryRaw);
  if (sameReference != QStringLiteral("volume.raw"))
    return fail("Same-directory RAW reference is not a plain file name");

  const QString otherReference =
    MetaImagePathUtils::elementDataFileReference(headerFile, otherDirectoryRaw);
  if (QDir::cleanPath(resolvedReference(headerFile, otherReference)) !=
      QDir::cleanPath(QFileInfo(otherDirectoryRaw).absoluteFilePath()))
    return fail("Cross-directory RAW reference resolves to the wrong file");
  if (!otherReference.contains(QString::fromUtf8("\xe4\xb8\xad\xe6\x96\x87")))
    return fail("Unicode was lost from the RAW reference");

  const QString alternateDriveRaw = QStringLiteral("Z:/raw data/volume.raw");
  const QString alternateDriveReference =
    MetaImagePathUtils::elementDataFileReference(headerFile, alternateDriveRaw);
  if (QFileInfo(headerFile).absolutePath().left(2).compare(
        alternateDriveRaw.left(2), Qt::CaseInsensitive) != 0 &&
      !QFileInfo(alternateDriveReference).isAbsolute())
    return fail("Cross-drive RAW reference is not absolute");

  std::cout << "MetaImage path smoke passed" << std::endl;
  return 0;
}
