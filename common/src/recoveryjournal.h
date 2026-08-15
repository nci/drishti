#ifndef DRISHTI_RECOVERYJOURNAL_H
#define DRISHTI_RECOVERYJOURNAL_H

#include <QString>
#include <QStringList>
#include <QVector>

struct RecoveryJournalEntry
{
  QString targetPath;
  QString stagePath;
  QString backupPath;
  bool targetExisted;
  bool stagePresent;
};

struct RecoveryJournalState
{
  QString scope;
  QString transactionId;
  QString journalPath;
  QVector<RecoveryJournalEntry> entries;
  // Direct-output transactions protect the previous generation in place;
  // callers continue writing to targetPath and commit only removes backups.
  bool directOutputs = false;
};

class RecoveryJournal
{
 public:
  static bool begin(const QStringList& targets,
                    const QString& scope,
                    RecoveryJournalState& state,
                    QString *error);
  static bool commit(const RecoveryJournalState& state,
                     QString *error);
  static bool recover(const QStringList& targets,
                      const QString& scope,
                      QString *error);
  static bool setStagePresent(RecoveryJournalState& state,
                              int index,
                              bool present,
                              QString *error);
  static bool discardStages(const RecoveryJournalState& state,
                            QString *error);

  // Import-style operations write directly to their final paths.  These
  // helpers keep the same JSON journal and recovery protocol while preserving
  // that public API contract.
  static bool beginDirect(const QString& directory,
                          const QString& scope,
                          RecoveryJournalState& state,
                          QString *error);
  static bool addDirectTarget(RecoveryJournalState& state,
                              const QString& target,
                              QString *error);
  static bool commitDirect(const RecoveryJournalState& state,
                           QString *error);
  static bool rollbackDirect(const RecoveryJournalState& state,
                             QString *error);
  static bool recoverDirectory(const QString& directory,
                               const QString& scope,
                               QString *error);
};

#endif
