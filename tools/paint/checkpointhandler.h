#ifndef CHECKPOINTHANDLER_H
#define CHECKPOINTHANDLER_H

#include "commonqtclasses.h"

class CheckpointHandler
{
 public :
  static bool saveCheckpoint(QString,
			     int, int, int, int,
			     uchar*, QString);

  static bool loadCheckpoint(QString,
			     int, int, int, int,
			     uchar*);

  // Non-interactive, zero-based record selection for tests and automation.
  static bool loadCheckpointRecord(QString,
			           int, int, int, int,
			           uchar*, int);

  static bool deleteCheckpoint(QString,
			       int, int, int, int,
			       uchar*);

  static bool deleteCheckpointRecord(QString,
			             int, int, int, int,
			             uchar*, int);

  static QString lastError();
};

#endif
