#include "slabsavetransaction.h"

#include "../../common/src/recoveryjournal.h"

namespace
{
const QString kScope = QStringLiteral("paint");

void copyFromCore(const RecoveryJournalState& source,
                  SlabSaveTransactionState& destination)
{
  destination = SlabSaveTransactionState();
  destination.transactionId = source.transactionId;
  destination.journalPath = source.journalPath;
  for (const RecoveryJournalEntry& entry : source.entries)
    {
      SlabSaveTransactionEntry converted;
      converted.targetPath = entry.targetPath;
      converted.stagePath = entry.stagePath;
      converted.backupPath = entry.backupPath;
      converted.targetExisted = entry.targetExisted;
      converted.stagePresent = entry.stagePresent;
      destination.entries.append(converted);
    }
}

bool copyToCore(const SlabSaveTransactionState& source,
                RecoveryJournalState& destination,
                QString *error)
{
  destination = RecoveryJournalState();
  destination.scope = kScope;
  destination.transactionId = source.transactionId;
  destination.journalPath = source.journalPath;
  if (source.transactionId.isEmpty() || source.entries.isEmpty())
    {
      if (error)
        *error = QStringLiteral("paint save transaction state is incomplete");
      return false;
    }
  for (const SlabSaveTransactionEntry& entry : source.entries)
    {
      RecoveryJournalEntry converted;
      converted.targetPath = entry.targetPath;
      converted.stagePath = entry.stagePath;
      converted.backupPath = entry.backupPath;
      converted.targetExisted = entry.targetExisted;
      converted.stagePresent = entry.stagePresent;
      destination.entries.append(converted);
    }
  return true;
}
}

bool SlabSaveTransaction::recover(const QStringList& targetPaths,
                                  QString *error)
{
  return RecoveryJournal::recover(targetPaths, kScope, error);
}

bool SlabSaveTransaction::begin(const QStringList& targetPaths,
                                SlabSaveTransactionState& transaction,
                                QString *error)
{
  RecoveryJournalState core;
  if (!RecoveryJournal::begin(targetPaths, kScope, core, error))
    {
      transaction = SlabSaveTransactionState();
      return false;
    }
  copyFromCore(core, transaction);
  return true;
}

bool SlabSaveTransaction::commit(const SlabSaveTransactionState& transaction,
                                 QString *error)
{
  RecoveryJournalState core;
  if (!copyToCore(transaction, core, error))
    return false;
  return RecoveryJournal::commit(core, error);
}

bool SlabSaveTransaction::setStagePresent(
  SlabSaveTransactionState& transaction,
  int index,
  bool present,
  QString *error)
{
  RecoveryJournalState core;
  if (!copyToCore(transaction, core, error))
    return false;
  if (!RecoveryJournal::setStagePresent(core, index, present, error))
    return false;
  copyFromCore(core, transaction);
  return true;
}

bool SlabSaveTransaction::discardStages(
  const SlabSaveTransactionState& transaction,
  QString *error)
{
  RecoveryJournalState core;
  if (!copyToCore(transaction, core, error))
    return false;
  return RecoveryJournal::discardStages(core, error);
}
