#include "classes.h"

//------------------------------------------------
ViewAlignedPolygon::ViewAlignedPolygon()
{
  edges = 0;
}
ViewAlignedPolygon&
ViewAlignedPolygon::operator=(const ViewAlignedPolygon& vap)
{
  edges = vap.edges;
  memcpy(vertex, vap.vertex, 100*sizeof(Vec));
  memcpy(texcoord, vap.texcoord, 100*sizeof(Vec));
  memcpy(tx, vap.tx, 100*sizeof(float));
  memcpy(ty, vap.ty, 100*sizeof(float));

  return *this;
}
void
ViewAlignedPolygon::scale(float scl)
{
  float cenx, ceny;
  cenx = 0.0; ceny = 0.0;
  for(int pi=0; pi<edges; pi++)
    {
      cenx += tx[pi];
      ceny += ty[pi];
    }
  cenx /= edges;
  ceny /= edges;
  for(int pi=0; pi<edges; pi++)
    {
      tx[pi] += scl*(tx[pi]-cenx);
      ty[pi] += scl*(ty[pi]-ceny);
    }
}
//------------------------------------------------


//------------------------------------------------
VoxelizedPath::VoxelizedPath() { voxels.clear(); normals.clear(); index.clear(); }
VoxelizedPath::~VoxelizedPath() { voxels.clear(); normals.clear(); index.clear(); }
VoxelizedPath&
VoxelizedPath::operator=(const VoxelizedPath& vp)
{
  voxels = vp.voxels;
  normals = vp.normals;
  index = vp.index;
  return *this;
}
//------------------------------------------------


//------------------------------------------------

BatchJob::BatchJob()
{
  startProject = false;
  backgroundrender = true;
  renderFrames = false;
  image = false;
  movie = false;
  imagesize = false;
  shading = false;
  skipEmptySpace = false;
  dragonly = false;
  dragonlyforshadows = false;
  depthcue = false;
  stepSize = 0.9f;
  startFrame = 0;
  endFrame = 0;
  stepFrame = 1;
  imgWidth = 720;
  imgHeight = 576;
  frameRate = 25;
  imageMode = 0;
  projectFilename.clear();
  imageFilename.clear();
  movieFilename.clear();
}
BatchJob&
BatchJob::operator=(const BatchJob& bj)
{
  startProject = bj.startProject;
  backgroundrender = bj.backgroundrender;
  renderFrames = bj.renderFrames;
  image = bj.image;
  movie = bj.movie;
  imagesize = bj.imagesize;
  shading = bj.shading;
  skipEmptySpace = bj.skipEmptySpace;
  dragonly = bj.dragonly;
  dragonlyforshadows = bj.dragonlyforshadows;
  depthcue = bj.depthcue;
  stepSize = bj.stepSize;
  startFrame = bj.startFrame;
  endFrame = bj.endFrame;
  stepFrame = bj.stepFrame;
  imgWidth = bj.imgWidth;
  imgHeight = bj.imgHeight;
  frameRate = bj.frameRate;
  imageMode = bj.imageMode;

  projectFilename = bj.projectFilename;
  imageFilename = bj.imageFilename;
  movieFilename = bj.movieFilename;
  
  return *this;
}
