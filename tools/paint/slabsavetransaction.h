#ifndef SLABSAVETRANSACTION_H
#define SLABSAVETRANSACTION_H

#include <QString>
#include <QStringList>
#include <QVector>

struct SlabSaveTransactionEntry
{
  QString targetPath;
  QString stagePath;
  QString backupPath;
  bool targetExisted;
  bool stagePresent;
};

struct SlabSaveTransactionState
{
  QString transactionId;
  QString journalPath;
  QVector<SlabSaveTransactionEntry> entries;
};

class SlabSaveTransaction
{
 public:
  static bool recover(const QStringList& targetPaths, QString *error);
  static bool begin(const QStringList& targetPaths,
                    SlabSaveTransactionState& transaction,
                    QString *error);
  static bool commit(const SlabSaveTransactionState& transaction,
                     QString *error);
  static bool setStagePresent(SlabSaveTransactionState& transaction,
                              int index, bool present, QString *error);
  static bool discardStages(const SlabSaveTransactionState& transaction,
                            QString *error);
};

#endif
