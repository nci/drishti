#include "volumefilemanager.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QTemporaryDir>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}

bool writeFile(const QString& path, const QByteArray& data)
{
  QFile file(path);
  return file.open(QFile::WriteOnly | QFile::Truncate) &&
         file.write(data) == data.size() && file.flush();
}

QByteArray readFile(const QString& path)
{
  QFile file(path);
  if (!file.open(QFile::ReadOnly))
    return QByteArray();
  return file.readAll();
}

void configure(VolumeFileManager& manager, const QString& base)
{
  manager.setBaseFilename(base);
  manager.setHeaderSize(0);
  manager.setSlabSize(2);
  manager.setVoxelType(VolumeFileManager::_UChar);
  manager.setDepth(4);
  manager.setWidth(2);
  manager.setHeight(2);
}

QStringList matchingFiles(const QString& base, const QString& suffixPattern)
{
  const QFileInfo info(base);
  QDir directory(info.absolutePath());
  return directory.entryList(QStringList() << info.fileName()+suffixPattern,
                             QDir::Files, QDir::Name);
}

#ifdef Q_OS_WIN
HANDLE lockAgainstDelete(const QString& path)
{
  return CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     nullptr, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL, nullptr);
}
#endif
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
#ifdef Q_OS_WIN
  const QDir executableDirectory =
    QFileInfo(QString::fromLocal8Bit(argv[0])).absoluteDir();
  const QStringList platformCandidates = QStringList()
    << executableDirectory.filePath("platforms")
    << executableDirectory.filePath("../../drishti-release/bin/platforms")
    << executableDirectory.filePath("../../../drishti-release/bin/platforms")
    << QDir(QLibraryInfo::location(QLibraryInfo::PluginsPath))
         .filePath("platforms");
  QString platformDirectory;
  for (const QString& candidate : platformCandidates)
    if (QFileInfo::exists(QDir(candidate).filePath("qoffscreen.dll")))
      {
        platformDirectory = QDir(candidate).absolutePath();
        break;
      }
  if (platformDirectory.isEmpty())
    return fail("cannot find qoffscreen.dll; the transaction test was not run");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", QFile::encodeName(platformDirectory));
#endif
  QApplication application(argc, argv);
  QTemporaryDir temporary;
  if (!temporary.isValid())
    return fail("cannot create a temporary directory");

  QDir root(temporary.path());
  if (!root.mkdir(QString::fromUtf8("volume \xE4\xB8\xAD\xE6\x96\x87 path")))
    return fail("cannot create the Unicode transaction directory");
  const QString directory = root.filePath(
    QString::fromUtf8("volume \xE4\xB8\xAD\xE6\x96\x87 path"));
  const QByteArray oldFirst("old-first-generation");
  const QByteArray oldSecond("old-second-generation");

  const QString rollbackBase = QDir(directory).filePath("rollback.raw");
  const QString rollbackFirst = rollbackBase+".001";
  const QString rollbackSecond = rollbackBase+".002";
  if (!writeFile(rollbackFirst, oldFirst) ||
      !writeFile(rollbackSecond, oldSecond))
    return fail("cannot prepare rollback targets");

  VolumeFileManager rollbackManager;
  configure(rollbackManager, rollbackBase);
  if (!rollbackManager.createFile(false))
    return fail("cannot prepare a volume transaction: "+
                rollbackManager.lastError());
  if (matchingFiles(rollbackFirst, ".drishti-backup-*").count() != 1 ||
      matchingFiles(rollbackSecond, ".drishti-backup-*").count() != 1)
    return fail("active transaction backups are missing");

#ifdef Q_OS_WIN
  HANDLE rollbackLock = lockAgainstDelete(rollbackFirst);
  if (rollbackLock == INVALID_HANDLE_VALUE)
    return fail("cannot lock the replacement for rollback fault injection");
  if (rollbackManager.rollbackFileCreation() ||
      !rollbackManager.lastError().contains("cannot restore") ||
      matchingFiles(rollbackFirst, ".drishti-backup-*").count() != 1)
    {
      CloseHandle(rollbackLock);
      return fail("rollback failure did not retain recoverable state");
    }
  CloseHandle(rollbackLock);
#endif

  if (!rollbackManager.rollbackFileCreation() ||
      readFile(rollbackFirst) != oldFirst ||
      readFile(rollbackSecond) != oldSecond ||
      !matchingFiles(rollbackFirst, ".drishti-backup-*").isEmpty() ||
      !matchingFiles(rollbackSecond, ".drishti-backup-*").isEmpty())
    return fail("retry did not restore the original volume generation");

  const QString installBase = QDir(directory).filePath("install failure.raw");
  const QString installFirst = installBase+".001";
  const QString installSecond = installBase+".002";
  if (!writeFile(installFirst, oldFirst) ||
      !writeFile(installSecond, oldSecond))
    return fail("cannot prepare install-failure targets");
  VolumeFileManager installManager;
  configure(installManager, installBase);

#ifdef Q_OS_WIN
  HANDLE installLock = lockAgainstDelete(installSecond);
  if (installLock == INVALID_HANDLE_VALUE)
    return fail("cannot lock the second target for install fault injection");
  const bool unexpectedlyInstalled = installManager.createFile(false);
  CloseHandle(installLock);
  if (unexpectedlyInstalled)
    return fail("locked second target unexpectedly installed");
  if (readFile(installFirst) != oldFirst ||
      readFile(installSecond) != oldSecond ||
      !matchingFiles(installFirst, ".drishti-backup-*").isEmpty() ||
      !matchingFiles(installFirst, ".drishti-part-*").isEmpty())
    return fail("failed multi-slab install did not roll back cleanly");
#endif

  const QString commitBase = QDir(directory).filePath("commit warning.raw");
  const QString commitFirst = commitBase+".001";
  const QString commitSecond = commitBase+".002";
  if (!writeFile(commitFirst, oldFirst) ||
      !writeFile(commitSecond, oldSecond))
    return fail("cannot prepare commit targets");
  VolumeFileManager commitManager;
  configure(commitManager, commitBase);
  if (!commitManager.createFile(false))
    return fail("cannot prepare the commit transaction: "+
                commitManager.lastError());

  QStringList commitBackups = matchingFiles(
    commitFirst, ".drishti-backup-*");
  if (commitBackups.count() != 1)
    return fail("commit backup is missing");
  const QString lockedBackup = QDir(directory).filePath(commitBackups[0]);
#ifdef Q_OS_WIN
  HANDLE backupLock = lockAgainstDelete(lockedBackup);
  if (backupLock == INVALID_HANDLE_VALUE)
    return fail("cannot lock the old generation for commit fault injection");
  if (!commitManager.commitFileCreation())
    {
      CloseHandle(backupLock);
      return fail("backup cleanup incorrectly invalidated committed output");
    }
  if (!QFileInfo::exists(lockedBackup))
    {
      CloseHandle(backupLock);
      return fail("the locked recovery backup unexpectedly disappeared");
    }
  CloseHandle(backupLock);
  if (!QFile::remove(lockedBackup))
    return fail("cannot clean the retained commit backup after unlocking it");
#else
  if (!commitManager.commitFileCreation())
    return fail("commit failed");
#endif
  if (readFile(commitFirst) == oldFirst || readFile(commitSecond) == oldSecond)
    return fail("commit restored the old generation");

  QTextStream(stdout) << "Volume file transaction smoke passed" << Qt::endl;
  return 0;
}
