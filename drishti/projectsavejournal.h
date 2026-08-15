#ifndef PROJECTSAVEJOURNAL_H
#define PROJECTSAVEJOURNAL_H

#include <QString>
#include <QStringList>
#include <QList>

struct ProjectSaveJournalState
{
  QString journalPath;
  QString transactionId;
  QStringList targets;
  QStringList stages;
  QStringList backups;
  QList<bool> targetExisted;
};

class ProjectSaveJournal
{
 public:
  static bool begin(const QStringList& targets,
                    ProjectSaveJournalState& state,
                    QString *error);
  static bool commit(const ProjectSaveJournalState& state,
                     QString *error);
  static bool recover(const QStringList& targets, QString *error);
};

#endif
