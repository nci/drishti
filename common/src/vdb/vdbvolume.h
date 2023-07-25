#ifndef VDBVOLUME
#define VDBVOLUME

#include <QApplication>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QProgressBar>
#include <QString>

// To jointly use QT and OpenVDB use the following preprocessor instruction
// before including openvdb.h.  The problem arises because Qt defines a Q_FOREACH
// macro which conflicts with the foreach methods in 'openvdb/util/NodeMask.h'.
// To remove this conflict, just un-define this macro wherever both openvdb and Qt
// are being included together. 
#ifdef foreach
  #undef foreach
#endif
// tbb/profiling.h has a function called emit()
// hence need to undef emit keyword in Qt
#undef emit
// a workaround to avoid imath_half_to_float_table linker error
#define IMATH_HALF_NO_LOOKUP_TABLE
#include <openvdb/openvdb.h>
#include <openvdb/Grid.h>
#include <math.h>

class VdbVolume
{
 public :
  VdbVolume();
  ~VdbVolume();
  
  VdbVolume(const VdbVolume&);
  VdbVolume&  operator=(const VdbVolume&);

  void save(QString);

  openvdb::FloatGrid::Accessor getAccessor() { return m_vdbGrid->getAccessor(); }

  
  uint64_t activeVoxels();
  
  void convertToLevelSet(float, int type=0);

  void mean(int, int);
  void gaussian(int, int);
  void offset(float);
  void resample(float);

  template <class T>
  void addSliceToVDB(T* data,
		     int x, int nY, int nZ,
		     int bType, float bValue1, float bValue2=0)
  {
    openvdb::FloatGrid::Accessor accessor = m_vdbGrid->getAccessor();
    
    openvdb::Coord ijk;
    int &d = ijk[0];
    int &w = ijk[1];
    int &h = ijk[2];
    
    d = x;
    if (bType == -1) // anything less than bValue is background
      {
	for (w=0; w<nY; w++)				
	  for (h=0; h<nZ; h++)			
	    {						
	      float value = data[w*nZ + h];		
	      if (value >= bValue1)			
		accessor.setValue(ijk, value);
	    }
      }
    else if (bType == 0) // anything equal to bValue is background
      {
	for (w=0; w<nY; w++)
	  for (h=0; h<nZ; h++)			
	    {						
	      float value = data[w*nZ + h]; 
	      if (qAbs(value-bValue1) > 0.00001)
		accessor.setValue(ijk, value);
	    }	
      }
    else if (bType == 1) // anything less than bValue is background
      {
	for (w=0; w<nY; w++)
	  for (h=0; h<nZ; h++)			
	    {						
	      float value = data[w*nZ + h];		
	      if (value <= bValue1)
		accessor.setValue(ijk, value);
	    } 
      }
    else if (bType == 2) // anything greater than bValue and less than bValue2 is background
      {
	for (w=0; w<nY; w++)
	  for (h=0; h<nZ; h++)			
	    {						
	      float value = data[w*nZ + h];		
	      if (value <= bValue1 || value >= bValue2)
		accessor.setValue(ijk, value);
	    } 
      }
    else if (bType == 3) // anything less than bValue1 or greater than bValue2 is background
      {
	for (w=0; w<nY; w++)
	  for (h=0; h<nZ; h++)			
	    {						
	      float value = data[w*nZ + h];		
	      if (value >= bValue1 && value <= bValue2)
		accessor.setValue(ijk, value);
	    } 
      }
    else if (bType == 4) // anything not bValue1 is background (treating value and bValue1 as integers)
      {
	for (w=0; w<nY; w++)
	  for (h=0; h<nZ; h++)			
	    {
	      float value = data[w*nZ + h];		
	      if ((int)value == (int)bValue1)
		accessor.setValue(ijk, value);
	    } 
      }
  };



  template <class T>
  void addSliceToVDB(T* data, int x, int nY, int nZ)
  {
    openvdb::FloatGrid::Accessor accessor = m_vdbGrid->getAccessor();
    
    openvdb::Coord ijk;
    int &d = ijk[0];
    int &w = ijk[1];
    int &h = ijk[2];
    
    d = x;
    for (w=0; w<nY; w++) 
      for (h=0; h<nZ; h++)			
	{						
	  float value = data[w*nZ + h];		
	  accessor.setValue(ijk, value);
	}
  };


  template <class T>
  void generateVDB(T *data,
		   int nX, int nY, int nZ,
		   int bType, float bValue1, float bValue2,
		   QProgressBar *progress = NULL)
  {
    openvdb::FloatGrid::Accessor accessor = m_vdbGrid->getAccessor();
    
    openvdb::Coord ijk;
    int &d = ijk[0];
    int &w = ijk[1];
    int &h = ijk[2];
    

    if (bType == -1) // anything less than bValue is background
      {
	for (d=0; d<nX; d++)
	  {
	    if (progress)
	      {
		progress->setValue((int)(100.0*(float)d/(float)(nZ)));
		qApp->processEvents();
	      }
	    for (w=0; w<nY; w++)
	      {
		for (h=0; h<nZ; h++)
		  {
		    float value = data[d*(long int)(nY*nZ) + (long int)(w*nZ) + h];
		    if (value >= bValue1)
		      accessor.setValue(ijk, value);
		  }
	      }
	  }
      }
    if (bType == 0) // bValue is background
      {
	for (d=0; d<nX; d++)
	  {
	    if (progress)
	      {
		progress->setValue((int)(100.0*(float)d/(float)(nZ)));
		qApp->processEvents();
	      }
	    for (w=0; w<nY; w++)
	      {
		for (h=0; h<nZ; h++)
		  {
		    float value = data[d*(long int)(nY*nZ) + (long int)(w*nZ) + h];
		    if (value != bValue1)
		      accessor.setValue(ijk, value);
		  }
	      }
	  }
      }
    if (bType == 1) // anything greater than bValue is background
      {
	for (d=0; d<nX; d++)
	  {
	    if (progress)
	      {
		progress->setValue((int)(100.0*(float)d/(float)(nZ)));
		qApp->processEvents();
	      }
	    for (w=0; w<nY; w++)
	      {
		for (h=0; h<nZ; h++)
		  {
		    float value = data[d*(long int)(nY*nZ) + (long int)(w*nZ) + h];
		    if (value <= bValue1)
		      accessor.setValue(ijk, value);
		  }
	      }
	  }
      }
    if (bType == 2) // anything greater than bValue and less than bValue2 is background
      {
	for (d=0; d<nX; d++)
	  {
	    if (progress)
	      {
		progress->setValue((int)(100.0*(float)d/(float)(nZ)));
		qApp->processEvents();
	      }
	    for (w=0; w<nY; w++)
	      {
		for (h=0; h<nZ; h++)
		  {
		    float value = data[d*(long int)(nY*nZ) + (long int)(w*nZ) + h];
		    if (value <= bValue1 || value >= bValue2)
		      accessor.setValue(ijk, value);
		  }
	      }
	  }
      }
    if (bType == 3) // anything less than bValue1 or greater than bValue2 is background
      {
	for (d=0; d<nX; d++)
	  {
	    if (progress)
	      {
		progress->setValue((int)(100.0*(float)d/(float)(nZ)));
		qApp->processEvents();
	      }
	    for (w=0; w<nY; w++)
	      {
		for (h=0; h<nZ; h++)
		  {
		    float value = data[d*(long int)(nY*nZ) + (long int)(w*nZ) + h];
		    if (value >= bValue1 && value <= bValue2)
		      accessor.setValue(ijk, value);
		  }
	      }
	  }
      }
    
    
    if (progress)
      {
	progress->setValue(100);
	qApp->processEvents();
      }
        
  }

  
  void generateMesh(int, float, float,
		    QVector<QVector3D>&, QVector<QVector3D>&, QVector<int>&);

  QVector<openvdb::FloatGrid::Ptr> segmentActiveVoxels();

 private :
  openvdb::FloatGrid::Ptr m_vdbGrid; 
};


#endif VDBVOLUME
