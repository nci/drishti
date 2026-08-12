#include "../slabsavetransaction.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>

namespace
{
bool writeFile(const QString& path, const QByteArray& data)
{
  QFile output(path);
  return output.open(QFile::WriteOnly) &&
         output.write(data) == data.size() && output.flush();
}

QByteArray readFile(const QString& path)
{
  QFile input(path);
  if (!input.open(QFile::ReadOnly))
    return QByteArray();
  return input.readAll();
}

bool writeJournal(const SlabSaveTransactionState& transaction,
                  const QString& state)
{
  QJsonArray entries;
  for(int index=0; index<transaction.entries.size(); ++index)
    {
      const SlabSaveTransactionEntry& entry = transaction.entries[index];
      QJsonObject object;
      object.insert("target", entry.targetPath);
      object.insert("stage", entry.stagePath);
      object.insert("backup", entry.backupPath);
      object.insert("existed", entry.targetExisted);
      entries.append(object);
    }
  QJsonObject root;
  root.insert("version", 1);
  root.insert("state", state);
  root.insert("transactionId", transaction.transactionId);
  root.insert("entries", entries);
  return writeFile(transaction.journalPath,
                   QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool renameFile(const QString& source, const QString& destination)
{
  QFile file(source);
  return file.rename(destination);
}

bool verifyTargets(const QStringList& targets, const QByteArray& prefix)
{
  for(int index=0; index<targets.size(); ++index)
    if (readFile(targets[index]) != prefix+QByteArray::number(index))
      return false;
  return true;
}

bool verifyClean(const SlabSaveTransactionState& transaction)
{
  if (QFileInfo::exists(transaction.journalPath))
    return false;
  for(int index=0; index<transaction.entries.size(); ++index)
    if (QFileInfo::exists(transaction.entries[index].stagePath) ||
        QFileInfo::exists(transaction.entries[index].backupPath))
      return false;
  return true;
}

bool createTargets(const QStringList& targets, const QByteArray& prefix)
{
  for(int index=0; index<targets.size(); ++index)
    if (!writeFile(targets[index], prefix+QByteArray::number(index)))
      return false;
  return true;
}

bool createStages(const SlabSaveTransactionState& transaction,
                  const QByteArray& prefix)
{
  for(int index=0; index<transaction.entries.size(); ++index)
    if (!writeFile(transaction.entries[index].stagePath,
                   prefix+QByteArray::number(index)))
      return false;
  return true;
}

bool fail(const QString& message, const QString& detail = QString())
{
  QTextStream stream(stderr);
  stream << "FAILED: " << message;
  if (!detail.isEmpty())
    stream << ": " << detail;
  stream << Qt::endl;
  return false;
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("cannot create temporary directory") ? 0 : 1;

  QStringList targets;
  targets << directory.filePath("volume.001")
          << directory.filePath("volume.002")
          << directory.filePath("volume.003");
  QString error;

  if (!createTargets(targets, "old-"))
    return fail("cannot create initial targets") ? 0 : 1;

  SlabSaveTransactionState committed;
  if (!SlabSaveTransaction::begin(targets, committed, &error) ||
      !createStages(committed, "new-") ||
      !SlabSaveTransaction::commit(committed, &error))
    return fail("normal commit", error) ? 0 : 1;
  if (!verifyTargets(targets, "new-") || !verifyClean(committed))
    return fail("normal commit result is inconsistent") ? 0 : 1;

  SlabSaveTransactionState stagingCrash;
  if (!SlabSaveTransaction::begin(targets, stagingCrash, &error) ||
      !QFileInfo::exists(stagingCrash.journalPath) ||
      !writeFile(stagingCrash.entries[0].stagePath, "partial-stage"))
    return fail("cannot prepare STAGING crash state", error) ? 0 : 1;
  if (!SlabSaveTransaction::recover(targets, &error) ||
      !verifyTargets(targets, "new-") || !verifyClean(stagingCrash))
    return fail("STAGING recovery did not discard partial output", error) ? 0 : 1;

  SlabSaveTransactionState incomplete;
  if (!SlabSaveTransaction::begin(targets, incomplete, &error) ||
      !writeFile(incomplete.entries[0].stagePath, "ignored"))
    return fail("cannot prepare incomplete staging case", error) ? 0 : 1;
  if (SlabSaveTransaction::commit(incomplete, &error))
    return fail("incomplete staging unexpectedly committed") ? 0 : 1;
  QString discardError;
  if (!SlabSaveTransaction::discardStages(incomplete, &discardError) ||
      !verifyTargets(targets, "new-") || !verifyClean(incomplete))
    return fail("incomplete staging changed targets", discardError) ? 0 : 1;

  SlabSaveTransactionState preparedCrash;
  if (!SlabSaveTransaction::begin(targets, preparedCrash, &error) ||
      !createStages(preparedCrash, "crash-") ||
      !writeJournal(preparedCrash, "PREPARED") ||
      !renameFile(preparedCrash.entries[0].targetPath,
                  preparedCrash.entries[0].backupPath) ||
      !renameFile(preparedCrash.entries[0].stagePath,
                  preparedCrash.entries[0].targetPath) ||
      !renameFile(preparedCrash.entries[1].targetPath,
                  preparedCrash.entries[1].backupPath))
    return fail("cannot construct PREPARED crash state", error) ? 0 : 1;
  if (!SlabSaveTransaction::recover(targets, &error) ||
      !verifyTargets(targets, "new-") || !verifyClean(preparedCrash))
    return fail("PREPARED recovery did not restore old generation", error) ? 0 : 1;

  SlabSaveTransactionState committedCrash;
  if (!SlabSaveTransaction::begin(targets, committedCrash, &error) ||
      !createStages(committedCrash, "final-") ||
      !writeJournal(committedCrash, "PREPARED"))
    return fail("cannot construct COMMITTED crash state", error) ? 0 : 1;
  for(int index=0; index<committedCrash.entries.size(); ++index)
    if (!renameFile(committedCrash.entries[index].targetPath,
                    committedCrash.entries[index].backupPath) ||
        !renameFile(committedCrash.entries[index].stagePath,
                    committedCrash.entries[index].targetPath))
      return fail("cannot switch COMMITTED crash target") ? 0 : 1;
  if (!writeJournal(committedCrash, "COMMITTED") ||
      !SlabSaveTransaction::recover(targets, &error) ||
      !verifyTargets(targets, "final-") || !verifyClean(committedCrash))
    return fail("COMMITTED recovery did not preserve new generation", error) ? 0 : 1;

  SlabSaveTransactionState corrupt;
  if (!SlabSaveTransaction::begin(targets, corrupt, &error) ||
      !writeFile(corrupt.journalPath, "not-json"))
    return fail("cannot construct corrupt journal case", error) ? 0 : 1;
  if (SlabSaveTransaction::recover(targets, &error) ||
      !verifyTargets(targets, "final-"))
    return fail("corrupt journal was accepted or changed targets", error) ? 0 : 1;

  QTextStream(stdout) << "Slab save transaction smoke passed" << Qt::endl;
  return 0;
}
