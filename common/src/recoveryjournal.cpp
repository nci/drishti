#include "recoveryjournal.h"

#include <QDir>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QUuid>

#include <limits>

namespace
{
const int kVersion = 1;
const int kMaximumEntries = 100000;
const qint64 kMaximumJournalBytes = 16LL*1024LL*1024LL;

bool fail(QString *error, const QString& message)
{
  if (error)
    *error = message;
  return false;
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

QString normalizedScope(const QString& scope)
{
  QString value;
  for (const QChar character : scope)
    if (character.isLetterOrNumber() || character == QLatin1Char('-') ||
        character == QLatin1Char('_'))
      value += character;
  return value.left(48);
}

QString journalPathFor(const QStringList& targets, const QString& scope)
{
  if (targets.isEmpty())
    return QString();
  const QFileInfo first(absolutePath(targets.first()));
  return first.dir().filePath(
    QString(".%1.drishti-%2-recovery.json")
      .arg(first.fileName(), normalizedScope(scope)));
}

QString journalPathForDirectory(const QString& directory,
                                const QString& scope)
{
  const QDir info(absolutePath(directory));
  return info.filePath(
    QString(".drishti-%1-direct-recovery.json").arg(normalizedScope(scope)));
}

QString auxiliaryPath(const QString& target,
                      const QString& scope,
                      const QString& role,
                      const QString& transactionId,
                      int index)
{
  const QFileInfo info(absolutePath(target));
  return info.dir().filePath(
    QString(".%1.drishti-%2-%3-%4-%5")
      .arg(info.fileName(), normalizedScope(scope), role,
           transactionId).arg(index));
}

void appendError(QString& errors, const QString& detail)
{
  if (detail.isEmpty())
    return;
  if (!errors.isEmpty())
    errors += QStringLiteral("; ");
  errors += detail;
}

bool removePath(const QString& path, QString& error)
{
  if (!QFileInfo::exists(path))
    return true;
  QFile file(path);
  if (file.remove())
    return true;
  appendError(error, QStringLiteral("cannot remove '%1': %2")
              .arg(path, file.errorString()));
  return false;
}

bool renamePath(const QString& source, const QString& destination,
                QString *error)
{
  QFile file(source);
  if (file.rename(destination))
    return true;
  return fail(error, QStringLiteral("cannot rename '%1' to '%2': %3")
              .arg(source, destination, file.errorString()));
}

bool validateTargets(const QStringList& rawTargets,
                     const QString& scope,
                     QStringList& targets,
                     QString *error)
{
  targets.clear();
  const QString cleanScope = normalizedScope(scope);
  if (cleanScope.isEmpty())
    return fail(error, QStringLiteral("recovery journal scope is empty"));
  if (rawTargets.isEmpty())
    return fail(error, QStringLiteral("recovery journal has no targets"));
  if (rawTargets.size() > kMaximumEntries)
    return fail(error, QStringLiteral("recovery journal has too many targets"));

  QSet<QString> unique;
  QString directory;
  for (int index=0; index<rawTargets.size(); ++index)
    {
      if (rawTargets.at(index).isEmpty())
        return fail(error, QStringLiteral("recovery journal target %1 is empty")
                   .arg(index));
      const QString target = absolutePath(rawTargets.at(index));
      const QFileInfo info(target);
      if (directory.isEmpty())
        directory = info.absolutePath();
      else if (!samePath(directory, info.absolutePath()))
        return fail(error,
                    QStringLiteral("recovery journal targets must share a directory"));
#ifdef Q_OS_WIN
      const QString key = target.toCaseFolded();
#else
      const QString key = target;
#endif
      if (unique.contains(key))
        return fail(error, QStringLiteral("recovery journal target is duplicated: %1")
                   .arg(target));
      unique.insert(key);
      targets.append(target);
    }
  return true;
}

bool validateState(const RecoveryJournalState& state, QString *error)
{
  QStringList targets;
  for (const RecoveryJournalEntry& entry : state.entries)
    targets.append(entry.targetPath);
  QStringList normalized;
  if (state.transactionId.isEmpty() ||
      (!state.directOutputs && state.entries.isEmpty()) ||
      (state.directOutputs && state.entries.isEmpty() ?
       state.scope.isEmpty() :
       !validateTargets(targets, state.scope, normalized, error)))
    return false;
  if (state.directOutputs)
    {
      if (state.entries.isEmpty())
        return true;
      if (!samePath(state.journalPath,
                    journalPathForDirectory(
                      QFileInfo(normalized.first()).absolutePath(),
                      state.scope)))
        return fail(error, QStringLiteral("recovery journal path is invalid"));
    }
  else if (!samePath(state.journalPath,
                     journalPathFor(normalized, state.scope)))
    return fail(error, QStringLiteral("recovery journal path is invalid"));
  for (int index=0; index<state.entries.size(); ++index)
    {
      const RecoveryJournalEntry& entry = state.entries.at(index);
      const bool validPaths =
        samePath(entry.targetPath, normalized.at(index)) &&
        samePath(entry.backupPath,
                 auxiliaryPath(entry.targetPath, state.scope, "backup",
                               state.transactionId, index));
      const bool validStage = state.directOutputs ?
        entry.stagePath.isEmpty() :
        samePath(entry.stagePath,
                 auxiliaryPath(entry.targetPath, state.scope, "stage",
                               state.transactionId, index));
      if (!validPaths || !validStage)
        return fail(error, QStringLiteral("recovery journal auxiliary path is invalid"));
    }
  return true;
}

bool sameState(const RecoveryJournalState& first,
               const RecoveryJournalState& second)
{
  if (first.scope != second.scope ||
      first.transactionId != second.transactionId ||
      first.directOutputs != second.directOutputs ||
      !samePath(first.journalPath, second.journalPath) ||
      first.entries.size() != second.entries.size())
    return false;
  for (int index=0; index<first.entries.size(); ++index)
    {
      const RecoveryJournalEntry& a = first.entries.at(index);
      const RecoveryJournalEntry& b = second.entries.at(index);
      if (!samePath(a.targetPath, b.targetPath) ||
          !samePath(a.stagePath, b.stagePath) ||
          !samePath(a.backupPath, b.backupPath) ||
          a.targetExisted != b.targetExisted ||
          a.stagePresent != b.stagePresent)
        return false;
    }
  return true;
}

bool writeJournal(const RecoveryJournalState& state,
                  const QString& phase,
                  QString *error)
{
  QJsonArray entries;
  for (const RecoveryJournalEntry& entry : state.entries)
    {
      QJsonObject object;
      object.insert(QStringLiteral("target"), entry.targetPath);
      object.insert(QStringLiteral("stage"), entry.stagePath);
      object.insert(QStringLiteral("backup"), entry.backupPath);
      object.insert(QStringLiteral("existed"), entry.targetExisted);
      object.insert(QStringLiteral("stagePresent"), entry.stagePresent);
      entries.append(object);
    }
  QJsonObject root;
  root.insert(QStringLiteral("version"), kVersion);
  root.insert(QStringLiteral("scope"), state.scope);
  root.insert(QStringLiteral("phase"), phase);
  if (state.directOutputs)
    root.insert(QStringLiteral("mode"), QStringLiteral("DIRECT"));
  root.insert(QStringLiteral("transactionId"), state.transactionId);
  root.insert(QStringLiteral("entries"), entries);
  const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
  if (data.isEmpty() || data.size() > kMaximumJournalBytes)
    return fail(error, QStringLiteral("recovery journal is too large"));

  QSaveFile file(state.journalPath);
  file.setDirectWriteFallback(false);
  if (!file.open(QIODevice::WriteOnly) ||
      file.write(data) != data.size() ||
      !file.commit())
    return fail(error, QStringLiteral("cannot write recovery journal '%1': %2")
               .arg(state.journalPath, file.errorString()));
  return true;
}

bool readJournal(const QString& path,
                 const QString& expectedScope,
                 RecoveryJournalState& state,
                 QString& phase,
                 QString *error)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly) ||
      file.size() <= 0 || file.size() > kMaximumJournalBytes)
    return fail(error, QStringLiteral("cannot read recovery journal '%1'")
               .arg(path));
  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(),
                                                          &parseError);
  if (parseError.error != QJsonParseError::NoError ||
      !document.isObject())
    return fail(error, QStringLiteral("invalid recovery journal: %1")
               .arg(parseError.errorString()));
  const QJsonObject root = document.object();
  const QString recordedScope = root.value(QStringLiteral("scope")).isString() ?
    root.value(QStringLiteral("scope")).toString() :
    normalizedScope(expectedScope);
  if (root.value(QStringLiteral("version")).toInt(-1) != kVersion ||
      recordedScope != normalizedScope(expectedScope))
    return fail(error, QStringLiteral("unsupported recovery journal version or scope"));
  phase = root.value(QStringLiteral("phase")).toString();
  if (phase.isEmpty())
    phase = root.value(QStringLiteral("state")).toString();
  if (phase != QStringLiteral("STAGING") &&
      phase != QStringLiteral("PREPARED") &&
      phase != QStringLiteral("COMMITTED"))
    return fail(error, QStringLiteral("invalid recovery journal phase"));
  state = RecoveryJournalState();
  state.scope = normalizedScope(expectedScope);
  state.journalPath = absolutePath(path);
  state.directOutputs = root.value(QStringLiteral("mode")).toString() ==
                        QStringLiteral("DIRECT");
  state.transactionId = root.value(QStringLiteral("transactionId")).toString();
  const QJsonArray entries = root.value(QStringLiteral("entries")).toArray();
  if (state.transactionId.isEmpty() || entries.isEmpty() ||
      entries.size() > kMaximumEntries)
    return fail(error, QStringLiteral("invalid recovery journal entries"));
  for (const QJsonValue& value : entries)
    {
      const QJsonObject object = value.toObject();
      if (!object.value(QStringLiteral("target")).isString() ||
          !object.value(QStringLiteral("stage")).isString() ||
          !object.value(QStringLiteral("backup")).isString() ||
          !object.value(QStringLiteral("existed")).isBool())
        return fail(error, QStringLiteral("invalid recovery journal entry"));
      RecoveryJournalEntry entry;
      entry.targetPath = absolutePath(object.value(QStringLiteral("target")).toString());
      const QString stage = object.value(QStringLiteral("stage")).toString();
      entry.stagePath = stage.isEmpty() ? QString() : absolutePath(stage);
      entry.backupPath = absolutePath(object.value(QStringLiteral("backup")).toString());
      entry.targetExisted = object.value(QStringLiteral("existed")).toBool();
      entry.stagePresent = object.value(QStringLiteral("stagePresent")).isBool() ?
        object.value(QStringLiteral("stagePresent")).toBool() : true;
      state.entries.append(entry);
    }
  return validateState(state, error);
}

bool rollbackPrepared(const RecoveryJournalState& state, QString *error)
{
  QString errors;
  for (int index=state.entries.size()-1; index>=0; --index)
    {
      const RecoveryJournalEntry& entry = state.entries.at(index);
      if (entry.targetExisted)
        {
          if (QFileInfo::exists(entry.backupPath))
            {
              if (QFileInfo::exists(entry.targetPath))
                (void)removePath(entry.targetPath, errors);
              QString detail;
              if (!renamePath(entry.backupPath, entry.targetPath, &detail))
                appendError(errors, detail);
            }
          else if (!QFileInfo::exists(entry.targetPath))
            appendError(errors, QStringLiteral("original target is missing: %1")
                        .arg(entry.targetPath));
        }
      else
        {
          (void)removePath(entry.targetPath, errors);
          (void)removePath(entry.backupPath, errors);
        }
      (void)removePath(entry.stagePath, errors);
    }
  if (errors.isEmpty())
    (void)removePath(state.journalPath, errors);
  return errors.isEmpty() ? true :
    fail(error, QStringLiteral("recovery journal rollback is incomplete: %1")
         .arg(errors));
}

bool cleanupCommitted(const RecoveryJournalState& state, QString *error)
{
  QString errors;
  for (const RecoveryJournalEntry& entry : state.entries)
    {
      if (entry.stagePresent && !QFileInfo::exists(entry.targetPath))
        appendError(errors, QStringLiteral("committed target is missing: %1")
                    .arg(entry.targetPath));
      (void)removePath(entry.stagePath, errors);
      (void)removePath(entry.backupPath, errors);
    }
  if (errors.isEmpty())
    (void)removePath(state.journalPath, errors);
  return errors.isEmpty() ? true :
    fail(error, QStringLiteral("recovery journal cleanup is incomplete: %1")
         .arg(errors));
}

bool rollbackDirectState(const RecoveryJournalState& state, QString *error)
{
  QString errors;
  for (int index=state.entries.size()-1; index>=0; --index)
    {
      const RecoveryJournalEntry& entry = state.entries.at(index);
      if (entry.targetExisted)
        {
          if (QFileInfo::exists(entry.backupPath))
            {
              if (QFileInfo::exists(entry.targetPath))
                (void)removePath(entry.targetPath, errors);
              QString detail;
              if (!renamePath(entry.backupPath, entry.targetPath, &detail))
                appendError(errors, detail);
            }
          else if (!QFileInfo::exists(entry.targetPath))
            appendError(errors, QStringLiteral("original target is missing: %1")
                        .arg(entry.targetPath));
        }
      else
        {
          (void)removePath(entry.targetPath, errors);
          (void)removePath(entry.backupPath, errors);
        }
      (void)removePath(entry.stagePath, errors);
    }
  if (errors.isEmpty())
    (void)removePath(state.journalPath, errors);
  return errors.isEmpty() ? true :
    fail(error, QStringLiteral("direct recovery rollback is incomplete: %1")
         .arg(errors));
}

bool cleanupDirectState(const RecoveryJournalState& state, QString *error)
{
  QString errors;
  for (const RecoveryJournalEntry& entry : state.entries)
    {
      if (!QFileInfo::exists(entry.targetPath))
        appendError(errors, QStringLiteral("committed target is missing: %1")
                    .arg(entry.targetPath));
      (void)removePath(entry.stagePath, errors);
      (void)removePath(entry.backupPath, errors);
    }
  if (errors.isEmpty())
    (void)removePath(state.journalPath, errors);
  return errors.isEmpty() ? true :
    fail(error, QStringLiteral("direct recovery cleanup is incomplete: %1")
         .arg(errors));
}

bool recoverLegacyBatchJournals(const QString& directory, QString *error)
{
  const QDir dir(absolutePath(directory));
  const QStringList journalNames = dir.entryList(
    QStringList() << QStringLiteral(".drishti-batch-journal-*"),
    QDir::Files | QDir::NoSymLinks, QDir::Name);
  QString errors;
  for (const QString& journalName : journalNames)
    {
      const QString journalPath = dir.absoluteFilePath(journalName);
      QFile file(journalPath);
      if (!file.open(QIODevice::ReadOnly))
        {
          appendError(errors, QStringLiteral("cannot read legacy batch journal '%1'")
                      .arg(journalPath));
          continue;
        }
      QDataStream input(&file);
      quint32 version = 0;
      quint8 committed = 0;
      quint32 count = 0;
      input >> version >> committed >> count;
      if (version != 2 || (committed != 0 && committed != 1) ||
          count == 0 || count > kMaximumEntries)
        {
          appendError(errors, QStringLiteral("invalid legacy batch journal '%1'")
                      .arg(journalPath));
          continue;
        }
      struct LegacyEntry
      {
        QString target;
        QString backup;
        bool targetExisted;
      };
      QVector<LegacyEntry> entries;
      for (quint32 index=0; index<count && input.status() == QDataStream::Ok;
           ++index)
        {
          LegacyEntry entry;
          input >> entry.target >> entry.backup >> entry.targetExisted;
          entries.append(entry);
        }
      file.close();
      if (input.status() != QDataStream::Ok)
        {
          appendError(errors, QStringLiteral("truncated legacy batch journal '%1'")
                      .arg(journalPath));
          continue;
        }

      bool valid = true;
      QSet<QString> targets;
      for (const LegacyEntry& entry : entries)
        {
          const QString target = absolutePath(entry.target);
          const QString backup = absolutePath(entry.backup);
          if (!samePath(QFileInfo(target).absolutePath(), dir.absolutePath()) ||
              !samePath(QFileInfo(backup).absolutePath(), dir.absolutePath()) ||
              samePath(target, journalPath) || samePath(backup, journalPath) ||
              samePath(target, backup) ||
              targets.contains(target))
            {
              valid = false;
              break;
            }
          targets.insert(target);
        }
      if (!valid)
        {
          appendError(errors, QStringLiteral("legacy batch journal paths are invalid: %1")
                      .arg(journalPath));
          continue;
        }

      for (int index=entries.size()-1; index>=0; --index)
        {
          const LegacyEntry& entry = entries.at(index);
          const QString target = absolutePath(entry.target);
          const QString backup = absolutePath(entry.backup);
          if (committed)
            {
              (void)removePath(backup, errors);
            }
          else if (!entry.targetExisted)
            {
              (void)removePath(target, errors);
            }
          else if (QFileInfo::exists(backup))
            {
              (void)removePath(target, errors);
              QString detail;
              if (!renamePath(backup, target, &detail))
                appendError(errors, detail);
            }
          else if (!QFileInfo::exists(target))
            {
              appendError(errors, QStringLiteral("legacy batch original target is missing: %1")
                          .arg(target));
            }
        }
      if (errors.isEmpty())
        (void)removePath(journalPath, errors);
    }
  return errors.isEmpty() ? true :
    fail(error, QStringLiteral("legacy batch journal recovery is incomplete: %1")
         .arg(errors));
}
}

bool RecoveryJournal::recover(const QStringList& rawTargets,
                              const QString& scope,
                              QString *error)
{
  if (error)
    error->clear();
  QStringList targets;
  if (!validateTargets(rawTargets, scope, targets, error))
    return false;
  const QString path = journalPathFor(targets, scope);
  if (!QFileInfo::exists(path))
    return true;
  RecoveryJournalState state;
  QString phase;
  if (!readJournal(path, scope, state, phase, error))
    return false;
  if (state.directOutputs)
    return fail(error, QStringLiteral("direct recovery journal requires a directory recovery request"));
  for (int index=0; index<targets.size(); ++index)
    if (!samePath(state.entries.at(index).targetPath, targets.at(index)))
      return fail(error, QStringLiteral("recovery journal targets do not match request"));
  if (phase == QStringLiteral("COMMITTED"))
    return cleanupCommitted(state, error);
  return rollbackPrepared(state, error);
}

bool RecoveryJournal::recoverDirectory(const QString& directory,
                                       const QString& scope,
                                       QString *error)
{
  if (error)
    error->clear();
  const QString cleanDirectory = absolutePath(directory);
  const QString path = journalPathForDirectory(cleanDirectory, scope);
  if (!QFileInfo::exists(path))
    return recoverLegacyBatchJournals(cleanDirectory, error);
  RecoveryJournalState state;
  QString phase;
  if (!readJournal(path, scope, state, phase, error))
    return false;
  if (!state.directOutputs)
    return fail(error, QStringLiteral("unexpected non-direct recovery journal in directory"));
  if (state.entries.isEmpty() ||
      !samePath(QFileInfo(state.entries.first().targetPath).absolutePath(),
                cleanDirectory))
    return fail(error, QStringLiteral("direct recovery journal directory does not match request"));
  if (phase == QStringLiteral("COMMITTED"))
    {
      if (!cleanupDirectState(state, error))
        return false;
    }
  else if (!rollbackDirectState(state, error))
    return false;
  return recoverLegacyBatchJournals(cleanDirectory, error);
}

bool RecoveryJournal::begin(const QStringList& rawTargets,
                            const QString& scope,
                            RecoveryJournalState& state,
                            QString *error)
{
  state = RecoveryJournalState();
  if (error)
    error->clear();
  QStringList targets;
  if (!validateTargets(rawTargets, scope, targets, error) ||
      !recover(targets, scope, error))
    return false;
  state.scope = normalizedScope(scope);
  state.transactionId =
    QUuid::createUuid().toString(QUuid::WithoutBraces);
  state.journalPath = journalPathFor(targets, state.scope);
  for (int index=0; index<targets.size(); ++index)
    {
      RecoveryJournalEntry entry;
      entry.targetPath = targets.at(index);
      entry.stagePath = auxiliaryPath(entry.targetPath, state.scope, "stage",
                                      state.transactionId, index);
      entry.backupPath = auxiliaryPath(entry.targetPath, state.scope, "backup",
                                       state.transactionId, index);
      entry.targetExisted = QFileInfo::exists(entry.targetPath);
      entry.stagePresent = true;
      if (QFileInfo::exists(entry.stagePath) ||
          QFileInfo::exists(entry.backupPath))
        return fail(error, QStringLiteral("recovery journal auxiliary path collision"));
      state.entries.append(entry);
    }
  if (!validateState(state, error))
    return false;
  return writeJournal(state, QStringLiteral("STAGING"), error);
}

bool RecoveryJournal::commit(const RecoveryJournalState& state,
                             QString *error)
{
  if (error)
    error->clear();
  if (!validateState(state, error) ||
      !QFileInfo::exists(state.journalPath))
    return fail(error, QStringLiteral("recovery journal state is not active"));
  QStringList targets;
  for (const RecoveryJournalEntry& entry : state.entries)
    targets.append(entry.targetPath);
  RecoveryJournalState recorded;
  QString phase;
  if (!readJournal(state.journalPath, state.scope, recorded, phase, error) ||
      phase != QStringLiteral("STAGING") ||
      !sameState(state, recorded))
    return fail(error, QStringLiteral("recovery journal does not match active transaction"));

  for (const RecoveryJournalEntry& entry : state.entries)
    {
      if (entry.stagePresent && !QFileInfo(entry.stagePath).isFile())
        return fail(error, QStringLiteral("staged target is missing: %1")
                   .arg(entry.stagePath));
      if (QFileInfo::exists(entry.backupPath))
        return fail(error, QStringLiteral("backup path already exists: %1")
                   .arg(entry.backupPath));
      if (entry.targetExisted != QFileInfo::exists(entry.targetPath))
        return fail(error, QStringLiteral("target changed while staging: %1")
                   .arg(entry.targetPath));
    }
  if (!writeJournal(state, QStringLiteral("PREPARED"), error))
    return false;
  QString switchError;
  for (const RecoveryJournalEntry& entry : state.entries)
    {
      if (entry.targetExisted &&
          !renamePath(entry.targetPath, entry.backupPath, &switchError))
        break;
      if (entry.stagePresent &&
          !renamePath(entry.stagePath, entry.targetPath, &switchError))
        break;
    }
  if (!switchError.isEmpty())
    {
      QString rollbackError;
      if (!rollbackPrepared(state, &rollbackError))
        switchError += QStringLiteral("; ") + rollbackError;
      return fail(error, QStringLiteral("recovery journal switch failed: %1")
                  .arg(switchError));
    }
  if (!writeJournal(state, QStringLiteral("COMMITTED"), error))
    {
      QString rollbackError;
      if (!rollbackPrepared(state, &rollbackError))
        *error += QStringLiteral("; ") + rollbackError;
      return false;
    }
  return cleanupCommitted(state, error);
}

bool RecoveryJournal::setStagePresent(RecoveryJournalState& state,
                                      int index,
                                      bool present,
                                      QString *error)
{
  if (error)
    error->clear();
  if (index < 0 || index >= state.entries.size() ||
      !validateState(state, error))
    return false;
  QString phase;
  RecoveryJournalState recorded;
  QStringList targets;
  for (const RecoveryJournalEntry& entry : state.entries)
    targets.append(entry.targetPath);
  if (!readJournal(state.journalPath, state.scope, recorded, phase, error) ||
      phase != QStringLiteral("STAGING") ||
      !sameState(state, recorded))
    return fail(error, QStringLiteral("recovery journal does not match active transaction"));
  state.entries[index].stagePresent = present;
  return writeJournal(state, QStringLiteral("STAGING"), error);
}

bool RecoveryJournal::discardStages(const RecoveryJournalState& state,
                                    QString *error)
{
  if (error)
    error->clear();
  if (!validateState(state, error))
    return false;
  QString errors;
  for (const RecoveryJournalEntry& entry : state.entries)
    {
      if (QFileInfo::exists(entry.backupPath))
        appendError(errors, QStringLiteral("unexpected backup exists: %1")
                    .arg(entry.backupPath));
      (void)removePath(entry.stagePath, errors);
    }
  (void)removePath(state.journalPath, errors);
  return errors.isEmpty() ? true :
    fail(error, QStringLiteral("recovery journal staging cleanup failed: %1")
         .arg(errors));
}

bool RecoveryJournal::beginDirect(const QString& directory,
                                  const QString& scope,
                                  RecoveryJournalState& state,
                                  QString *error)
{
  state = RecoveryJournalState();
  state.directOutputs = true;
  if (error)
    error->clear();
  const QString cleanDirectory = absolutePath(directory);
  const QFileInfo directoryInfo(cleanDirectory);
  if (!directoryInfo.isDir())
    return fail(error, QStringLiteral("direct recovery directory is invalid: %1")
                .arg(cleanDirectory));
  if (normalizedScope(scope).isEmpty())
    return fail(error, QStringLiteral("recovery journal scope is empty"));
  if (!recoverDirectory(cleanDirectory, scope, error))
    return false;
  state.scope = normalizedScope(scope);
  state.transactionId =
    QUuid::createUuid().toString(QUuid::WithoutBraces);
  state.journalPath = journalPathForDirectory(cleanDirectory, state.scope);
  return true;
}

bool RecoveryJournal::addDirectTarget(RecoveryJournalState& state,
                                      const QString& rawTarget,
                                      QString *error)
{
  if (error)
    error->clear();
  if (!state.directOutputs || state.transactionId.isEmpty() ||
      state.journalPath.isEmpty())
    return fail(error, QStringLiteral("direct recovery journal state is not active"));
  if (!state.entries.isEmpty() && !validateState(state, error))
    return false;
  const QString target = absolutePath(rawTarget);
  const QFileInfo targetInfo(target);
  if (target.isEmpty() ||
      !samePath(targetInfo.absolutePath(), QFileInfo(state.journalPath).absolutePath()))
    return fail(error, QStringLiteral("direct recovery targets must share a directory"));
  for (const RecoveryJournalEntry& existing : state.entries)
    if (samePath(existing.targetPath, target))
      return true;

  RecoveryJournalEntry entry;
  entry.targetPath = target;
  entry.stagePath = QString();
  entry.backupPath = auxiliaryPath(target, state.scope, "backup",
                                   state.transactionId,
                                   state.entries.size());
  entry.targetExisted = QFileInfo::exists(target);
  entry.stagePresent = false;
  if (QFileInfo::exists(entry.backupPath))
    return fail(error, QStringLiteral("recovery journal auxiliary path collision"));
  state.entries.append(entry);
  if (!validateState(state, error) ||
      !writeJournal(state, QStringLiteral("STAGING"), error))
    {
      state.entries.removeLast();
      return false;
    }
  if (entry.targetExisted && !QFile::rename(entry.targetPath, entry.backupPath))
    {
      state.entries.removeLast();
      QString ignored;
      (void)writeJournal(state, QStringLiteral("STAGING"), &ignored);
      return fail(error, QStringLiteral("cannot preserve existing batch output '%1'")
                  .arg(target));
    }
  return writeJournal(state, QStringLiteral("STAGING"), error);
}

bool RecoveryJournal::commitDirect(const RecoveryJournalState& state,
                                   QString *error)
{
  if (error)
    error->clear();
  if (!state.directOutputs || !validateState(state, error) ||
      !QFileInfo::exists(state.journalPath))
    return fail(error, QStringLiteral("direct recovery journal state is not active"));
  RecoveryJournalState recorded;
  QString phase;
  if (!readJournal(state.journalPath, state.scope, recorded, phase, error) ||
      phase != QStringLiteral("STAGING") || !sameState(state, recorded))
    return fail(error, QStringLiteral("direct recovery journal does not match active transaction"));
  for (const RecoveryJournalEntry& entry : state.entries)
    {
      if (!QFileInfo::exists(entry.targetPath))
        return fail(error, QStringLiteral("batch output is missing: %1")
                    .arg(entry.targetPath));
      if (entry.targetExisted && !QFileInfo::exists(entry.backupPath))
        return fail(error, QStringLiteral("batch backup is missing: %1")
                    .arg(entry.backupPath));
    }
  if (!writeJournal(state, QStringLiteral("PREPARED"), error))
    return false;
  if (!writeJournal(state, QStringLiteral("COMMITTED"), error))
    {
      QString rollbackError;
      if (!rollbackDirectState(state, &rollbackError))
        *error += QStringLiteral("; ") + rollbackError;
      return false;
    }
  return cleanupDirectState(state, error);
}

bool RecoveryJournal::rollbackDirect(const RecoveryJournalState& state,
                                     QString *error)
{
  if (error)
    error->clear();
  if (!state.directOutputs || !validateState(state, error))
    return fail(error, QStringLiteral("direct recovery journal state is not active"));
  return rollbackDirectState(state, error);
}
