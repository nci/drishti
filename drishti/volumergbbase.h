#ifndef VOLUMERGBBASE_H
#define VOLUMERGBBASE_H

#include <GL/glew.h>

#include <QGLViewer/vec.h>
using namespace qglviewer;

class VolumeRGBBase : public QObject
{
  Q_OBJECT

 public :
  VolumeRGBBase();
  ~VolumeRGBBase();

  bool loadVolume(const char*, bool);

  Vec getFullVolumeSize();
  Vec getLowresVolumeSize();
  Vec getLowresTextureVolumeSize();
  int getLowresSubsamplingLevel();
  int* getLowres1dHistogram(int);
  int* getLowres2dHistogram(int);
  unsigned char* getLowresVolume();
  unsigned char* getLowresTextureVolume();

  void createLowresTextureVolume();

 protected :
  QString m_volumeFile;
  int m_depth, m_width, m_height;
  int *m_1dHistogramR, *m_2dHistogramR;
  int *m_1dHistogramG, *m_2dHistogramG;
  int *m_1dHistogramB, *m_2dHistogramB;
  int *m_1dHistogramA, *m_2dHistogramA;
  Vec m_fullVolumeSize;
  Vec m_lowresVolumeSize;
  Vec m_lowresTextureVolumeSize;
  int m_subSamplingLevel;
  unsigned char *m_lowresVolume;
  unsigned char *m_lowresTextureVolume;

  void generateHistograms(bool);
  void createLowresVolume(bool);
};

#endif
