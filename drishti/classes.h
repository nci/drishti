#ifndef CLASSES_H
#define CLASSES_H

#include <GL/glew.h>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class ViewAlignedPolygon
{
 public :
  ViewAlignedPolygon();
  ViewAlignedPolygon& operator=(const ViewAlignedPolygon&);

  void scale(float);

  int edges;
  Vec vertex[20];
  Vec texcoord[20];
  float tx[20], ty[20];
};


class VoxelizedPath
{
 public :
  VoxelizedPath();
  ~VoxelizedPath();
  VoxelizedPath& operator=(const VoxelizedPath&);

  QList<Vec> voxels;
  QList<Vec> normals;
  QList<uint> index;
};

class BatchJob
{
 public :
  BatchJob();
  BatchJob& operator=(const BatchJob&);

  bool startProject;
  bool backgroundrender;
  bool renderFrames;
  bool plugin;
  bool image;
  bool movie;
  bool imagesize;
  bool shading;
  bool skipEmptySpace;
  bool dragonly;
  bool dragonlyforshadows;
  bool depthcue;
  float stepSize;
  int startFrame, endFrame, stepFrame;
  int imgWidth, imgHeight, frameRate;
  int imageMode;
  QString projectFilename;
  QString imageFilename, movieFilename;  
  QString pluginName;
};

#endif
