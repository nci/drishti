#ifndef VOLINTERFACE_H
#define VOLINTERFACE_H

#include <QtCore>

class VolInterface
{
 public :
  virtual ~VolInterface() {}

  // register plugin with the main program
  // tells whether accepts files, directory or both
  virtual QStringList registerPlugin() = 0;

  // initialize various objects
  virtual void init() = 0;

  // set 4D volume flag
  virtual void set4DVolume(bool) = 0;

  // reinitialize various objects
  virtual void clear() = 0;

  // check and load data, generate min/max values and histogram
  virtual bool setFile(QStringList) = 0;

  // choose another file when handling timeseries data
  virtual void replaceFile(QString) = 0;

  virtual void gridSize(int&, int&, int&) = 0;
  virtual void voxelSize(float&, float&, float&) = 0;
  virtual QString description() = 0;
  virtual int voxelUnit() = 0;
  virtual int voxelType() = 0;
  virtual int headerBytes() = 0;

  virtual QList<uint> histogram() = 0;
  
  virtual void setMinMax(float, float) = 0;
  virtual float rawMin() = 0;
  virtual float rawMax() = 0;

  virtual void generateHistogram() = 0;

  virtual void getDepthSlice(int, uchar*) = 0;
  virtual void getWidthSlice(int, uchar*) = 0;
  virtual void getHeightSlice(int, uchar*) = 0;

  virtual QVariant rawValue(int, int, int) = 0;

  virtual void saveTrimmed(QString,
			   int, int, int, int, int, int) = 0;

};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(VolInterface,
		    "drishti.import.Plugin.VolInterface/1.0");
QT_END_NAMESPACE

#endif
