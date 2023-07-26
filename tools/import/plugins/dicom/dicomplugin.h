#ifndef DICOMPLUGIN_H
#define DICOMPLUGIN_H

#include <QObject>
#include "volinterface.h"

#include "itkImage.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageFileReader.h"
#include "itkImageSeriesReader.h"
#include "itkExtractImageFilter.h"

class DicomPlugin : public QObject, VolInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.VolInterface/1.0")
  Q_INTERFACES(VolInterface)

 public :
  QStringList registerPlugin();

  void init();
  void clear();

  void setValue(QString, float) {};

  bool setFile(QStringList);
  void replaceFile(QString);

  void gridSize(int&, int&, int&);
  void voxelSize(float&, float&, float&);
  QString description();
  int voxelUnit();
  int voxelType();
  int headerBytes();

  QList<uint> histogram();
  
  void setMinMax(float, float);
  float rawMin();
  float rawMax();
   
  void getDepthSlice(int, uchar*);
  //void getWidthSlice(int, uchar*);
  //void getHeightSlice(int, uchar*);

  QVariant rawValue(int, int, int);

  //void saveTrimmed(QString, int, int, int, int, int, int);

  void generateHistogram();
  void set4DVolume(bool);
 private :
  typedef itk::Image<short, 3> ImageType;
  typedef itk::ImageSeriesReader<ImageType> ReaderType;

  ReaderType::Pointer m_reader;
  ImageType::Pointer m_dimg;

  QStringList m_fileName;
  bool m_4dvol;
  int m_depth, m_width, m_height;
  int m_voxelUnit;
  int m_voxelType;
  int m_headerBytes;
  float m_voxelSizeX;
  float m_voxelSizeY;
  float m_voxelSizeZ;
  QString m_description;
  
  float m_rawMin, m_rawMax;
  QList<uint> m_histogram;

  int m_bytesPerVoxel;

  QList<QString> m_imageList;

  void findMinMaxandGenerateHistogram();
};

#endif
