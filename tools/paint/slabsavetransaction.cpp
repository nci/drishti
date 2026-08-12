#include "slabsavetransaction.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QUuid>

namespace
{
const int kJournalVersion = 1;
const int kMaximumEntries = 100000;
const qint64 kMaximumJournalBytes = 8LL*1024LL*1024LL;

bool fail(QString *error, const QString& message)
{
  if (error)
    *error = message;
  return false;
}

void appendError(QString& error, const QString& message)
{
  if (error.isEmpty())
    error = message;
  else
    error += QString("; %1").arg(message);
}

QString absolutePath(const QString& path)
{
  return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool samePath(const QString& first, const QString& second)
{
#ifdef Q_OS_WIN
  return QString::compare(absolutePath(first), absolutePath(second),
                          Qt::CaseInsensitive) == 0;
#else
  return absolutePath(first) == absolutePath(second);
#endif
}

QString journalPathFor(const QStringList& targetPaths)
{
  if (targetPaths.isEmpty())
    return QString();
  const QFileInfo first(absolutePath(targetPaths[0]));
  return first.dir().filePath(
    QString(".%1.drishti-slab-save.json").arg(first.fileName()));
}

QString auxiliaryPath(const QString& targetPath,
                      const QString& role,
                      const QString& transactionId,
                      int index)
{
  const QFileInfo target(absolutePath(targetPath));
  return target.dir().filePath(
    QString(".%1.drishti-%2-%3-%4")
      .arg(target.fileName(), role, transactionId)
      .arg(index));
}

bool validateTargets(const QStringList& targetPaths,
                     QStringList& normalizedTargets,
                     QString *error)
{
  normalizedTargets.clear();
  if (targetPaths.isEmpty())
    return fail(error, "slab save transaction: no target files");
  if (targetPaths.size() > kMaximumEntries)
    return fail(error,
                QString("slab save transaction: %1 targets exceed limit %2")
                  .arg(targetPaths.size()).arg(kMaximumEntries));

  QSet<QString> uniqueTargets;
  for(int index=0; index<targetPaths.size(); ++index)
    {
      if (targetPaths[index].isEmpty())
        return fail(error,
                    QString("slab save transaction: target %1 is empty")
                      .arg(index));
      const QString target = absolutePath(targetPaths[index]);
#ifdef Q_OS_WIN
      const QString key = target.toCaseFolded();
#else
      const QString key = target;
#endif
      if (uniqueTargets.contains(key))
        return fail(error,
                    QString("slab save transaction: duplicate target '%1'")
                      .arg(target));
      uniqueTargets.insert(key);
      normalizedTargets.append(target);
    }
  return true;
}

bool validateAuxiliaryPath(const SlabSaveTransactionEntry& entry,
                           const QString& path,
                           const QString& role,
                           const QString& transactionId,
                           int index,
                           QString *error)
{
  const QFileInfo target(entry.targetPath);
  const QFileInfo auxiliary(path);
  if (!samePath(target.absolutePath(), auxiliary.absolutePath()))
    return fail(error,
                QString("slab save transaction: %1 path escapes target directory")
                  .arg(role));
  const QString expected = auxiliaryPath(entry.targetPath, role,
                                         transactionId, index);
  if (!samePath(path, expected))
    return fail(error,
                QString("slab save transaction: unexpected %1 path '%2'")
                  .arg(role, path));
  return true;
}

bool validateTransaction(const SlabSaveTransactionState& transaction,
                         QString *error)
{
  if (transaction.transactionId.isEmpty() ||
      transaction.entries.isEmpty() ||
      transaction.entries.size() > kMaximumEntries)
    return fail(error, "slab save transaction: invalid transaction state");

  QStringList targets;
  for(int index=0; index<transaction.entries.size(); ++index)
    targets.append(transaction.entries[index].targetPath);
  QStringList normalizedTargets;
  if (!validateTargets(targets, normalizedTargets, error))
    return false;
  if (!samePath(transaction.journalPath, journalPathFor(normalizedTargets)))
    return fail(error, "slab save transaction: unexpected journal path");

  for(int index=0; index<transaction.entries.size(); ++index)
    {
      const SlabSaveTransactionEntry& entry = transaction.entries[index];
      if (!samePath(entry.targetPath, normalizedTargets[index]) ||
          !validateAuxiliaryPath(entry, entry.stagePath, "stage",
                                 transaction.transactionId, index, error) ||
          !validateAuxiliaryPath(entry, entry.backupPath, "backup",
                                 transaction.transactionId, index, error))
        return false;
    }
  return true;
}

bool sameTransaction(const SlabSaveTransactionState& first,
                     const SlabSaveTransactionState& second)
{
  if (first.transactionId != second.transactionId ||
      !samePath(first.journalPath, second.journalPath) ||
      first.entries.size() != second.entries.size())
    return false;
  for(int index=0; index<first.entries.size(); ++index)
    {
      const SlabSaveTransactionEntry& a = first.entries[index];
      const SlabSaveTransactionEntry& b = second.entries[index];
      if (!samePath(a.targetPath, b.targetPath) ||
          !samePath(a.stagePath, b.stagePath) ||
          !samePath(a.backupPath, b.backupPath) ||
          a.targetExisted != b.targetExisted)
        return false;
    }
  return true;
}

bool writeJournal(const SlabSaveTransactionState& transaction,
                  const QString& state,
                  QString *error)
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
  root.insert("version", kJournalVersion);
  root.insert("state", state);
  root.insert("transactionId", transaction.transactionId);
  root.insert("entries", entries);
  const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
  if (data.isEmpty() || data.size() > kMaximumJournalBytes)
    return fail(error, "slab save transaction: journal is too large");

  QSaveFile output(transaction.journalPath);
  output.setDirectWriteFallback(false);
  if (!output.open(QFile::WriteOnly))
    return fail(error,
                QString("slab save transaction: cannot open journal '%1': %2")
                  .arg(transaction.journalPath, output.errorString()));
  if (output.write(data) != data.size())
    {
      const QString detail = output.errorString();
      output.cancelWriting();
      return fail(error,
                  QString("slab save transaction: cannot write journal '%1': %2")
                    .arg(transaction.journalPath, detail));
    }
  if (!output.commit())
    return fail(error,
                QString("slab save transaction: cannot commit journal '%1': %2")
                  .arg(transaction.journalPath, output.errorString()));
  return true;
}

bool readJournal(const QString& journalPath,
                 const QStringList& expectedTargets,
                 SlabSaveTransactionState& transaction,
                 QString& state,
                 QString *error)
{
  QFile input(journalPath);
  if (!input.open(QFile::ReadOnly))
    return fail(error,
                QString("slab save recovery: cannot open journal '%1': %2")
                  .arg(journalPath, input.errorString()));
  if (input.size() <= 0 || input.size() > kMaximumJournalBytes)
    return fail(error,
                QString("slab save recovery: invalid journal size %1")
                  .arg(input.size()));
  const QByteArray data = input.readAll();
  if (data.size() != input.size())
    return fail(error,
                QString("slab save recovery: short read from journal '%1'")
                  .arg(journalPath));

  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject())
    return fail(error,
                QString("slab save recovery: invalid journal JSON: %1")
                  .arg(parseError.errorString()));
  const QJsonObject root = document.object();
  if (root.value("version").toInt(-1) != kJournalVersion)
    return fail(error, "slab save recovery: unsupported journal version");

  state = root.value("state").toString();
  if (state != "STAGING" &&
      state != "PREPARED" && state != "COMMITTED")
    return fail(error,
                QString("slab save recovery: invalid journal state '%1'")
                  .arg(state));
  transaction = SlabSaveTransactionState();
  transaction.transactionId = root.value("transactionId").toString();
  transaction.journalPath = absolutePath(journalPath);

  const QJsonArray entries = root.value("entries").toArray();
  if (entries.isEmpty() || entries.size() != expectedTargets.size() ||
      entries.size() > kMaximumEntries)
    return fail(error,
                QString("slab save recovery: journal has %1 entries, expected %2")
                  .arg(entries.size()).arg(expectedTargets.size()));
  for(int index=0; index<entries.size(); ++index)
    {
      if (!entries[index].isObject())
        return fail(error,
                    QString("slab save recovery: entry %1 is invalid")
                      .arg(index));
      const QJsonObject object = entries[index].toObject();
      SlabSaveTransactionEntry entry;
      entry.targetPath = object.value("target").toString();
      entry.stagePath = object.value("stage").toString();
      entry.backupPath = object.value("backup").toString();
      if (!object.value("existed").isBool())
        return fail(error,
                    QString("slab save recovery: entry %1 has no existence flag")
                      .arg(index));
      entry.targetExisted = object.value("existed").toBool();
      transaction.entries.append(entry);
    }

  if (!validateTransaction(transaction, error))
    return false;
  for(int index=0; index<expectedTargets.size(); ++index)
    if (!samePath(transaction.entries[index].targetPath,
                  expectedTargets[index]))
      return fail(error,
                  QString("slab save recovery: journal target %1 does not match '%2'")
                    .arg(index).arg(expectedTargets[index]));
  return true;
}

bool removeFile(const QString& path, QString& errors)
{
  if (!QFileInfo::exists(path))
    return true;
  QFile file(path);
  if (file.remove())
    return true;
  appendError(errors,
              QString("cannot remove '%1': %2")
                .arg(path, file.errorString()));
  return false;
}

bool renameFile(const QString& source,
                const QString& destination,
                QString *error)
{
  QFile file(source);
  if (file.rename(destination))
    return true;
  return fail(error,
              QString("cannot rename '%1' to '%2': %3")
                .arg(source, destination, file.errorString()));
}

bool rollbackPrepared(const SlabSaveTransactionState& transaction,
                      QString *error)
{
  QString errors;
  for(int index=transaction.entries.size()-1; index>=0; --index)
    {
      const SlabSaveTransactionEntry& entry = transaction.entries[index];
      if (entry.targetExisted)
        {
          if (QFileInfo::exists(entry.backupPath))
            {
              if (QFileInfo::exists(entry.targetPath) &&
                  !removeFile(entry.targetPath, errors))
                continue;
              QString renameError;
              if (!renameFile(entry.backupPath, entry.targetPath,
                              &renameError))
                appendError(errors, renameError);
            }
          else if (!QFileInfo::exists(entry.targetPath))
            appendError(errors,
                        QString("original slab is missing for '%1'")
                          .arg(entry.targetPath));
        }
      else
        {
          (void)removeFile(entry.targetPath, errors);
          (void)removeFile(entry.backupPath, errors);
        }
    }

  for(int index=0; index<transaction.entries.size(); ++index)
    (void)removeFile(transaction.entries[index].stagePath, errors);

  if (errors.isEmpty())
    {
      if (QFileInfo::exists(transaction.journalPath))
        {
          QFile journal(transaction.journalPath);
          if (!journal.remove())
            appendError(errors,
                        QString("cannot remove journal '%1': %2")
                          .arg(transaction.journalPath,
                               journal.errorString()));
        }
    }
  if (!errors.isEmpty())
    return fail(error,
                QString("slab save rollback is incomplete: %1").arg(errors));
  return true;
}

bool cleanupCommitted(const SlabSaveTransactionState& transaction,
                      QString *error)
{
  for(int index=0; index<transaction.entries.size(); ++index)
    if (!QFileInfo::exists(transaction.entries[index].targetPath))
      return fail(error,
                  QString("slab save cleanup: committed target '%1' is missing")
                    .arg(transaction.entries[index].targetPath));

  QString errors;
  for(int index=0; index<transaction.entries.size(); ++index)
    {
      (void)removeFile(transaction.entries[index].stagePath, errors);
      (void)removeFile(transaction.entries[index].backupPath, errors);
    }
  if (!errors.isEmpty())
    return fail(error,
                QString("slab save cleanup is incomplete: %1").arg(errors));

  if (QFileInfo::exists(transaction.journalPath))
    {
      QFile journal(transaction.journalPath);
      if (!journal.remove())
        return fail(error,
                    QString("slab save cleanup: cannot remove journal '%1': %2")
                      .arg(transaction.journalPath, journal.errorString()));
    }
  return true;
}
}

bool
SlabSaveTransaction::recover(const QStringList& targetPaths, QString *error)
{
  if (error)
    error->clear();
  QStringList targets;
  if (!validateTargets(targetPaths, targets, error))
    return false;
  const QString journalPath = journalPathFor(targets);
  if (!QFileInfo::exists(journalPath))
    return true;

  SlabSaveTransactionState transaction;
  QString state;
  if (!readJournal(journalPath, targets, transaction, state, error))
    return false;
  if (state == "STAGING")
    return discardStages(transaction, error);
  if (state == "PREPARED")
    return rollbackPrepared(transaction, error);
  return cleanupCommitted(transaction, error);
}

bool
SlabSaveTransaction::begin(const QStringList& targetPaths,
                           SlabSaveTransactionState& transaction,
                           QString *error)
{
  if (error)
    error->clear();
  transaction = SlabSaveTransactionState();
  QStringList targets;
  if (!validateTargets(targetPaths, targets, error) ||
      !recover(targets, error))
    return false;

  transaction.transactionId =
    QUuid::createUuid().toString(QUuid::WithoutBraces);
  transaction.journalPath = journalPathFor(targets);
  for(int index=0; index<targets.size(); ++index)
    {
      SlabSaveTransactionEntry entry;
      entry.targetPath = targets[index];
      entry.stagePath = auxiliaryPath(entry.targetPath, "stage",
                                      transaction.transactionId, index);
      entry.backupPath = auxiliaryPath(entry.targetPath, "backup",
                                       transaction.transactionId, index);
      entry.targetExisted = QFileInfo::exists(entry.targetPath);
      if (QFileInfo::exists(entry.stagePath) ||
          QFileInfo::exists(entry.backupPath))
        return fail(error,
                    QString("slab save transaction: auxiliary path collision for '%1'")
                      .arg(entry.targetPath));
      transaction.entries.append(entry);
    }
  if (!validateTransaction(transaction, error))
    return false;
  return writeJournal(transaction, "STAGING", error);
}

bool
SlabSaveTransaction::commit(const SlabSaveTransactionState& transaction,
                            QString *error)
{
  if (error)
    error->clear();
  if (!validateTransaction(transaction, error))
    return false;
  if (!QFileInfo::exists(transaction.journalPath))
    return fail(error,
                QString("slab save transaction: staging journal '%1' is missing")
                  .arg(transaction.journalPath));

  QStringList targets;
  for(int index=0; index<transaction.entries.size(); ++index)
    targets.append(transaction.entries[index].targetPath);
  SlabSaveTransactionState recordedTransaction;
  QString recordedState;
  if (!readJournal(transaction.journalPath, targets,
                   recordedTransaction, recordedState, error))
    return false;
  if (recordedState != "STAGING" ||
      !sameTransaction(transaction, recordedTransaction))
    return fail(error,
                "slab save transaction: staging journal does not match the active transaction");

  for(int index=0; index<transaction.entries.size(); ++index)
    {
      const SlabSaveTransactionEntry& entry = transaction.entries[index];
      if (!QFileInfo(entry.stagePath).isFile())
        return fail(error,
                    QString("slab save transaction: stage '%1' is missing")
                      .arg(entry.stagePath));
      if (QFileInfo::exists(entry.backupPath))
        return fail(error,
                    QString("slab save transaction: backup '%1' already exists")
                      .arg(entry.backupPath));
      if (entry.targetExisted != QFileInfo::exists(entry.targetPath))
        return fail(error,
                    QString("slab save transaction: target '%1' changed while staging")
                      .arg(entry.targetPath));
    }

  if (!writeJournal(transaction, "PREPARED", error))
    return false;

  QString switchError;
  for(int index=0; index<transaction.entries.size(); ++index)
    {
      const SlabSaveTransactionEntry& entry = transaction.entries[index];
      if (entry.targetExisted &&
          !renameFile(entry.targetPath, entry.backupPath, &switchError))
        break;
      if (!renameFile(entry.stagePath, entry.targetPath, &switchError))
        break;
    }

  if (!switchError.isEmpty())
    {
      QString rollbackError;
      if (!rollbackPrepared(transaction, &rollbackError))
        switchError += QString("; %1").arg(rollbackError);
      return fail(error,
                  QString("slab save transaction could not switch files: %1")
                    .arg(switchError));
    }

  QString committedError;
  if (!writeJournal(transaction, "COMMITTED", &committedError))
    {
      QString rollbackError;
      if (!rollbackPrepared(transaction, &rollbackError))
        committedError += QString("; %1").arg(rollbackError);
      return fail(error,
                  QString("slab save transaction could not record commit: %1")
                    .arg(committedError));
    }

  return cleanupCommitted(transaction, error);
}

bool
SlabSaveTransaction::discardStages(
  const SlabSaveTransactionState& transaction,
  QString *error)
{
  if (error)
    error->clear();
  if (!validateTransaction(transaction, error))
    return false;
  QString errors;
  for(int index=0; index<transaction.entries.size(); ++index)
    if (QFileInfo::exists(transaction.entries[index].backupPath))
      appendError(errors,
                  QString("unexpected backup exists while discarding staging '%1'")
                    .arg(transaction.entries[index].backupPath));
  for(int index=0; index<transaction.entries.size(); ++index)
    (void)removeFile(transaction.entries[index].stagePath, errors);
  if (errors.isEmpty() && QFileInfo::exists(transaction.journalPath))
    (void)removeFile(transaction.journalPath, errors);
  if (!errors.isEmpty())
    return fail(error,
                QString("slab save transaction: cannot discard stages: %1")
                  .arg(errors));
  return true;
}
