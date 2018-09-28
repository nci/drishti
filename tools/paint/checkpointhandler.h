#ifndef CHECKPOINTHANDLER_H
#define CHECKPOINTHANDLER_H

#include "commonqtclasses.h"

class CheckpointHandler
{
 public :
  static void saveCheckpoint(QString,
			     int, int, int, int,
			     uchar*, QString);
			     
  static void loadCheckpoint(QString,
			     int, int, int, int,
			     uchar*);  
};

#endif
