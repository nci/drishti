#include "checkpointhandler.h"
#include "global.h"
#include "staticfunctions.h"
#include "volumefilemanager.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>

bool
StaticFunctions::checkExtension(QString filename, const char *extension)
{
  return filename.endsWith(QString::fromLatin1(extension),
                           Qt::CaseInsensitive);
}

QString Global::previousDirectory()
{
  return QString();
}

bool CheckpointHandler::saveCheckpoint(QString, int, int, int, int,
                                       uchar*, QString)
{
  return false;
}

bool CheckpointHandler::loadCheckpoint(QString, int, int, int, int, uchar*)
{
  return false;
}

bool CheckpointHandler::deleteCheckpoint(QString, int, int, int, int, uchar*)
{
  return false;
}

QString CheckpointHandler::lastError()
{
  return QString("checkpoint functions are not used by this smoke test");
}

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication application(argc, argv);
  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("cannot create a temporary directory");

  const int depth = 9;
  const int width = 11;
  const int height = 13;
  const int changedIndex = (2*width+3)*height+4;
  QDir root(directory.path());
  if (!root.mkdir("storage"))
    return fail("cannot create the mask storage directory");
  const QString validPath = directory.filePath("storage/labels.mask.sc");

  VolumeFileManager manager;
  manager.setFilenameList(QStringList() << validPath);
  manager.setDepth(depth);
  manager.setWidth(width);
  manager.setHeight(height);
  manager.setHeaderSize(13);
  manager.setSlabSize(depth+1);
  manager.setVoxelType(VolumeFileManager::_UShort);
  if (!manager.setMemMapped(true) || !manager.createFile(false))
    return fail(QString("cannot create test mask: %1").arg(manager.lastError()));

  if (!manager.setValueMem(2, 3, 4, 321))
    return fail(manager.lastError());
  uchar *const dirtyBuffer = manager.memVolDataPtr();
  if (!root.rename("storage", "offline"))
    return fail("cannot make the save target temporarily unavailable");
  if (manager.reset())
    return fail("reset unexpectedly succeeded with an invalid save target");
  if (manager.memVolDataPtr() != dirtyBuffer || !manager.isMemMapped() ||
      manager.memVolDataPtrUS()[changedIndex] != 321)
    return fail("failed reset released or changed the dirty mask buffer");

  if (!root.rename("offline", "storage"))
    return fail("cannot restore the save target after failed reset");
  if (!manager.flushPendingChanges() || !manager.createUndo() ||
      !QFileInfo::exists(validPath+".tmp"))
    return fail(QString("cannot flush or create undo: %1")
                  .arg(manager.lastError()));

  if (!manager.setValueMem(1, 2, 3, 654))
    return fail(manager.lastError());
  uchar *const secondDirtyBuffer = manager.memVolDataPtr();
  if (!root.rename("storage", "offline"))
    return fail("cannot make the offload target temporarily unavailable");
  if (manager.setMemMapped(false))
    return fail("offload unexpectedly succeeded with an invalid save target");
  if (manager.memVolDataPtr() != secondDirtyBuffer || !manager.isMemMapped())
    return fail("failed offload released the dirty mask buffer");

  if (!root.rename("offline", "storage"))
    return fail("cannot restore the save target after failed offload");
  if (!manager.setMemMapped(false) || manager.memVolDataPtr() ||
      manager.isMemMapped())
    return fail(QString("retry offload failed: %1").arg(manager.lastError()));

  QTextStream(stdout) << "VFM lifecycle smoke passed" << Qt::endl;
  return 0;
}
