#ifndef GLOBAL_H
#define GLOBAL_H

#include "commonqtclasses.h"
#include <QStatusBar>
#include <QProgressBar>
#include <QProgressDialog>
#include <QMessageBox>

class Global
{
 public :
  static QString documentationPath();

  static int lutSize();
  static void setLutSize(int);

  static uchar* lut();
  static void setLut(uchar*);

  static int filteredData();
  static void setFilteredData(int);

  static bool use1D();
  static void setUse1D(bool);

  static QString previousDirectory();
  static void setPreviousDirectory(QString);

  static void setTagColors(uchar*);
  static uchar* tagColors();

  static int maxRecentFiles();
  static QStringList recentFiles();
  static void setRecentFiles(QStringList);
  static void addRecentFile(QString);
  static QString recentFile(int);

  static bool line();
  static void setLine(bool);

  static int tag();
  static void setTag(int);

  static bool tagSimilar();
  static void setTagSimilar(bool);

  static bool copyPrev();
  static void setCopyPrev(bool);

  static int prevErode();
  static void setPrevErode(int);

  static int smooth();
  static void setSmooth(int);

  static int boxSize();
  static void setBoxSize(int);

  static int lambda();
  static void setLambda(int);

  static int spread();
  static void setSpread(int);

 private :
  static QString m_documentationPath;
  static int m_lutSize;
  static uchar* m_lut;
  static QString m_previousDirectory;
  static bool m_use1D;
  static uchar *m_tagColors;
  static int m_maxRecentFiles;
  static QStringList m_recentFiles;
  static bool m_line;
  static int m_tag;
  static int m_boxSize;
  static int m_lambda;
  static int m_spread;
  static bool m_tagSimilar;
  static bool m_copyPrev;
  static int m_prevErode;
  static int m_smooth;
};

#endif
