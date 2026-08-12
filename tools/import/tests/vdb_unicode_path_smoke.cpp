#include "vdbvolume.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include <openvdb/io/Stream.h>

#include <exception>
#include <fstream>
#ifdef Q_OS_WIN
#include <filesystem>
#endif
#include <iostream>

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  QTemporaryDir temporaryDirectory;
  if (!temporaryDirectory.isValid())
    {
      std::cerr << "Cannot create the temporary test directory.\n";
      return 1;
    }

  QString unicodeDirectoryName;
  unicodeDirectoryName.append(QChar(0x663e));
  unicodeDirectoryName.append(QChar(0x5fae));
  unicodeDirectoryName.append(QChar(0x6570));
  unicodeDirectoryName.append(QChar(0x636e));
  const QString unicodeDirectory =
    QDir(temporaryDirectory.path()).filePath(unicodeDirectoryName);
  if (!QDir().mkpath(unicodeDirectory))
    {
      std::cerr << "Cannot create the Unicode test directory.\n";
      return 2;
    }

  const QString outputFile =
    QDir(unicodeDirectory).filePath(QStringLiteral("volume.vdb"));
  try
    {
      VdbVolume volume;
      float values[] = { 1.0f, 2.0f, 3.0f, 4.0f };
      volume.addSliceToVDB(values, 0, 2, 2, -1, -100.0f);
      volume.save(outputFile);

      const QFileInfo outputInfo(outputFile);
      if (!outputInfo.exists() || !outputInfo.isFile() ||
          outputInfo.size() <= 0)
        {
          std::cerr << "The Unicode-path VDB output is missing or empty.\n";
          return 3;
        }

      std::ifstream input;
#ifdef Q_OS_WIN
      input.open(std::filesystem::path(outputFile.toStdWString()),
                 std::ios::in | std::ios::binary);
#else
      input.open(outputFile.toStdString(), std::ios::in | std::ios::binary);
#endif
      if (!input.is_open())
        {
          std::cerr << "Cannot reopen the Unicode-path VDB output.\n";
          return 4;
        }

      openvdb::io::Stream archive(input);
      openvdb::GridPtrVecPtr grids = archive.getGrids();
      if (!grids || grids->size() != 1)
        {
          std::cerr << "The VDB output does not contain exactly one grid.\n";
          return 5;
        }
      openvdb::FloatGrid::Ptr grid =
        openvdb::gridPtrCast<openvdb::FloatGrid>((*grids)[0]);
      if (!grid || grid->activeVoxelCount() != 4)
        {
          std::cerr << "The VDB grid contents are incomplete.\n";
          return 6;
        }
    }
  catch (const std::exception& exception)
    {
      std::cerr << "VDB Unicode-path smoke failed: " << exception.what()
                << "\n";
      return 7;
    }

  std::cout << "VDB Unicode-path smoke passed.\n";
  return 0;
}
