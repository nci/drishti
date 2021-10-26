#ifndef GLOBAL_H
#define GLOBAL_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

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

  static int thickness();
  static void setThickness(int);

  static bool closed();
  static void setClosed(bool);

  static int boxSize();
  static void setBoxSize(int);

  static int lambda();
  static void setLambda(int);

  static int spread();
  static void setSpread(int);

  static int selectionPrecision();
  static void setSelectionPrecision(int);

  static Vec relativeVoxelScaling();
  static Vec voxelScaling();
  static void setVoxelScaling(Vec);

  static QString voxelUnit();
  static void setVoxelUnit(QString);

  static GLuint spriteTexture();
  static void removeSpriteTexture();

  static GLuint hollowSpriteTexture();
  static void removeHollowSpriteTexture();

  static void setBytesPerVoxel(int);
  static int bytesPerVoxel();

  static QWidget* mainWindow();
  static void setMainWindow(QWidget*);

  static void setShowBox3D(bool);
  static bool showBox3D();
  static Vec boxSize3D();
  static void setBoxSize3D(Vec);
  static QList<Vec> boxList3D();
  static void addToBoxList3D(Vec);
  static void clearBoxList3D();

  static void setShowBox2D(bool);
  static bool showBox2D();
  static int boxSize2D();
  static void setBoxSize2D(int);
  static QList<Vec> boxList2D();
  static void addToBoxList2D(Vec, Vec);
  static void clearBoxList2D();
  
 private :
  static QWidget* m_mainWindow;
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
  static int m_thickness;
  static bool m_closed;
  static int m_selpres;
  static Vec m_voxelScaling;
  static Vec m_relativeVoxelScaling;
  static QString m_voxelUnit;
  static int m_bytesPerVoxel;

  static GLuint m_spriteTexture;
  static GLuint m_hollowSpriteTexture;

  static bool m_showBox2D;
  static int m_boxSize2D;
  static QList<Vec> m_boxList2D;

  static bool m_showBox3D;
  static Vec m_boxSize3D;
  static QList<Vec> m_boxList3D;
};

#endif
