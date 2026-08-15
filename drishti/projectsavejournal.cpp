#include "projectsavejournal.h"

#include "../common/src/recoveryjournal.h"

namespace
{
const QString kScope = QStringLiteral("project");

void copyFromCore(const RecoveryJournalState& source,
                  ProjectSaveJournalState& destination)
{
  destination = ProjectSaveJournalState();
  destination.journalPath = source.journalPath;
  destination.transactionId = source.transactionId;
  for (const RecoveryJournalEntry& entry : source.entries)
    {
      destination.targets.append(entry.targetPath);
      destination.stages.append(entry.stagePath);
      destination.backups.append(entry.backupPath);
      destination.targetExisted.append(entry.targetExisted);
    }
}

bool copyToCore(const ProjectSaveJournalState& source,
                RecoveryJournalState& destination,
                QString *error)
{
  destination = RecoveryJournalState();
  destination.scope = kScope;
  destination.journalPath = source.journalPath;
  destination.transactionId = source.transactionId;
  if (source.targets.isEmpty() ||
      source.targets.size() != source.stages.size() ||
      source.targets.size() != source.backups.size() ||
      source.targets.size() != source.targetExisted.size())
    {
      if (error)
        *error = QStringLiteral("project save journal state is incomplete");
      return false;
    }
  for (int index=0; index<source.targets.size(); ++index)
    {
      RecoveryJournalEntry entry;
      entry.targetPath = source.targets.at(index);
      entry.stagePath = source.stages.at(index);
      entry.backupPath = source.backups.at(index);
      entry.targetExisted = source.targetExisted.at(index);
      entry.stagePresent = true;
      destination.entries.append(entry);
    }
  return true;
}
}

bool ProjectSaveJournal::recover(const QStringList& targets, QString *error)
{
  return RecoveryJournal::recover(targets, kScope, error);
}

bool ProjectSaveJournal::begin(const QStringList& targets,
                               ProjectSaveJournalState& state,
                               QString *error)
{
  RecoveryJournalState core;
  if (!RecoveryJournal::begin(targets, kScope, core, error))
    {
      state = ProjectSaveJournalState();
      return false;
    }
  copyFromCore(core, state);
  return true;
}

bool ProjectSaveJournal::commit(const ProjectSaveJournalState& state,
                                QString *error)
{
  RecoveryJournalState core;
  if (!copyToCore(state, core, error))
    return false;
  return RecoveryJournal::commit(core, error);
}
