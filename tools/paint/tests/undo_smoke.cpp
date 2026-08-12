#include "filehandler.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QVector>

#include <cstring>
#include <limits>

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}

QByteArray readAll(const QString& path)
{
  QFile input(path);
  if (!input.open(QFile::ReadOnly))
    return QByteArray();
  return input.readAll();
}

void configure(FileHandler& handler,
               const QString& path,
               int voxelType,
               int depth, int width, int height,
               uchar *data, qint64 capacity)
{
  handler.setFilenameList(QStringList() << path);
  handler.setDepth(depth);
  handler.setWidth(width);
  handler.setHeight(height);
  handler.setVoxelType(voxelType);
  handler.setVolData(data, capacity);
}

template<typename T>
bool runCase(const QString& directory,
             const QString& name,
             int voxelType,
             QString& error)
{
  QTextStream(stdout) << "Running " << name << Qt::endl;
  const int depth = 17;
  const int width = 29;
  const int height = 31;
  const int count = depth*width*height;
  QVector<T> original(count);
  QVector<T> changed(count);
  QVector<T> live(count);
  QVector<T> disk(count);
  for(int index=0; index<count; ++index)
    {
      original[index] = static_cast<T>((index*37+11) &
                                       std::numeric_limits<T>::max());
      changed[index] = static_cast<T>((index*101+73) &
                                      std::numeric_limits<T>::max());
    }

  const qint64 bytes = static_cast<qint64>(count)*sizeof(T);
  const QString target = directory+"/"+name+".mask.sc";
  const QString undoPath = target+".tmp";

  FileHandler handler;
  live = original;
  configure(handler, target, voxelType, depth, width, height,
            reinterpret_cast<uchar*>(live.data()), bytes);
  if (!handler.saveMemFile(1) || !handler.genUndo())
    {
      error = handler.lastError();
      return false;
    }
  QTextStream(stdout) << name << ": undo created" << Qt::endl;

  memcpy(live.data(), changed.constData(), static_cast<size_t>(bytes));
  if (!handler.saveMemFile(2))
    {
      error = handler.lastError();
      return false;
    }
  QTextStream(stdout) << name << ": changed state written" << Qt::endl;
  QVector<T> staged(count);
  FileHandler preflight;
  configure(preflight, undoPath, voxelType, depth, width, height,
            reinterpret_cast<uchar*>(staged.data()), bytes);
  if (!preflight.loadMemFile() || staged != original)
    {
      error = preflight.lastError().isEmpty() ?
        QString("undo preflight did not round trip") : preflight.lastError();
      return false;
    }
  QTextStream(stdout) << name << ": undo preflight verified" << Qt::endl;
  if (!handler.undo() || live != original)
    {
      error = handler.lastError().isEmpty() ?
        QString("valid undo did not restore memory") : handler.lastError();
      return false;
    }
  QTextStream(stdout) << name << ": valid undo restored" << Qt::endl;

  FileHandler loader;
  configure(loader, target, voxelType, depth, width, height,
            reinterpret_cast<uchar*>(disk.data()), bytes);
  if (!loader.loadMemFile() || disk != original)
    {
      error = loader.lastError().isEmpty() ?
        QString("valid undo did not restore disk") : loader.lastError();
      return false;
    }
  QTextStream(stdout) << name << ": disk verified" << Qt::endl;

  memcpy(live.data(), changed.constData(), static_cast<size_t>(bytes));
  if (!handler.saveMemFile(3))
    {
      error = handler.lastError();
      return false;
    }
  QTextStream(stdout) << name << ": changed state saved" << Qt::endl;
  const QByteArray committed = readAll(target);
  QFile corruptUndo(undoPath);
  if (committed.isEmpty() || !corruptUndo.open(QFile::ReadWrite) ||
      !corruptUndo.resize(9))
    {
      error = "cannot prepare the corrupt undo case";
      return false;
    }
  corruptUndo.close();

  if (handler.undo() || handler.lastError().isEmpty() ||
      live != changed || readAll(target) != committed)
    {
      error = "corrupt undo changed memory or the committed mask";
      return false;
    }
  QTextStream(stdout) << name << ": corrupt undo rejected" << Qt::endl;
  return true;
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("cannot create a temporary directory");

  QString error;
  if (!runCase<uchar>(directory.path(), "labels8", 0, error))
    return fail(QString("8-bit undo: %1").arg(error));
  if (!runCase<ushort>(directory.path(), "labels16", 2, error))
    return fail(QString("16-bit undo: %1").arg(error));

  QTextStream(stdout) << "Undo smoke passed" << Qt::endl;
  return 0;
}
