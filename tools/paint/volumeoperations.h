#ifndef VOLUMEOPERATIONS_H
#define VOLUMEOPERATIONS_H

#include "getmemorysize.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <limits>

enum class PaintVolumeAlgorithm
{
  ConnectedComponents,
  DistanceTransform,
  LocalThickness,
  Watershed
};

struct PaintVolumeAlgorithmMemoryProfile
{
  std::uint64_t bytesPerVoxel;
  std::uint64_t fixedOverheadBytes;
  std::uint64_t distanceTaskBytesPerScanline;
  std::uint64_t visibilityTaskBytesPerSlice;
};

inline std::uint32_t
paintVolumeMaximumMaskLabel()
{
  return std::numeric_limits<std::uint16_t>::max();
}

inline bool
paintVolumeOffsetLabelRangeFits(std::uint32_t startingLabel,
                                std::uint64_t maximumSourceLabel)
{
  const std::uint64_t maximumMaskLabel = paintVolumeMaximumMaskLabel();
  return startingLabel <= maximumMaskLabel &&
         maximumSourceLabel <= maximumMaskLabel-startingLabel;
}

class PaintVolumeUniqueLabelTracker
{
 public:
  PaintVolumeUniqueLabelTracker()
    : m_uniqueLabelCount(0), m_maximumLabel(0)
  {}

  bool add(std::uint32_t label)
  {
    if (label > paintVolumeMaximumMaskLabel())
      return false;
    if (label == 0 || m_seen.test(static_cast<std::size_t>(label)))
      return true;

    m_seen.set(static_cast<std::size_t>(label));
    ++m_uniqueLabelCount;
    if (label > m_maximumLabel)
      m_maximumLabel = label;
    return true;
  }

  std::uint32_t uniqueLabelCount() const { return m_uniqueLabelCount; }
  std::uint32_t maximumLabel() const { return m_maximumLabel; }

 private:
  std::bitset<65536> m_seen;
  std::uint32_t m_uniqueLabelCount;
  std::uint32_t m_maximumLabel;
};

inline bool
paintVolumeCheckedMultiply(std::uint64_t first,
                           std::uint64_t second,
                           std::uint64_t& result)
{
  if (first == 0 || second == 0)
    {
      result = 0;
      return true;
    }
  if (first > std::numeric_limits<std::uint64_t>::max()/second)
    return false;
  result = first*second;
  return true;
}

inline bool
paintVolumeCheckedAdd(std::uint64_t first,
                      std::uint64_t second,
                      std::uint64_t& result)
{
  if (first > std::numeric_limits<std::uint64_t>::max()-second)
    return false;
  result = first+second;
  return true;
}

inline PaintVolumeAlgorithmMemoryProfile
paintVolumeAlgorithmMemoryProfile(PaintVolumeAlgorithm algorithm)
{
  const std::uint64_t mebibyte = 1024ULL*1024ULL;
  switch (algorithm)
    {
    case PaintVolumeAlgorithm::ConnectedComponents:
      // QMap/QMultiMap nodes can approach one entry for every other voxel.
      return { 192ULL, 64ULL*mebibyte, 0ULL, 512ULL };
    case PaintVolumeAlgorithm::DistanceTransform:
      return { 32ULL, 64ULL*mebibyte, 512ULL, 0ULL };
    case PaintVolumeAlgorithm::LocalThickness:
      return { 64ULL, 64ULL*mebibyte, 512ULL, 0ULL };
    case PaintVolumeAlgorithm::Watershed:
      return { 160ULL, 128ULL*mebibyte, 512ULL, 0ULL };
    }

  return { 0, 0, 0, 0 };
}

inline PaintAlgorithmMemoryAdmission
evaluatePaintVolumeAlgorithmMemoryAdmission(
  PaintVolumeAlgorithm algorithm,
  std::uint64_t depth,
  std::uint64_t width,
  std::uint64_t height,
  PaintMemoryStatusProvider provider = nullptr,
  void *providerContext = nullptr)
{
  const PaintVolumeAlgorithmMemoryProfile profile =
    paintVolumeAlgorithmMemoryProfile(algorithm);

  std::uint64_t fixedOverheadBytes = profile.fixedOverheadBytes;
  if (profile.visibilityTaskBytesPerSlice > 0)
    {
      // getVisibleRegion materializes one QVariant task record per Z slice.
      // A 1x1xD ROI therefore needs a separate shape term in addition to the
      // component-container bytes-per-voxel estimate.
      std::uint64_t visibilityTaskBytes = 0;
      if (!paintVolumeCheckedMultiply(
            depth, profile.visibilityTaskBytesPerSlice,
            visibilityTaskBytes) ||
          !paintVolumeCheckedAdd(fixedOverheadBytes,
                                 visibilityTaskBytes,
                                 fixedOverheadBytes))
        {
          PaintAlgorithmMemoryAdmission overflow;
          overflow.reason = PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow;
          return overflow;
        }
    }
  if (profile.distanceTaskBytesPerScanline > 0)
    {
      // BinaryDistanceTransform currently materializes one eight-QVariant
      // task record for every Y/Z scanline.  This term is shape-dependent:
      // for a one-voxel-wide ROI the task count approaches the voxel count.
      std::uint64_t scanlineCount = 0;
      std::uint64_t taskBytes = 0;
      std::uint64_t scratchBytes = 0;
      const std::uint64_t maximumDimension =
        depth > width ? (depth > height ? depth : height) :
                        (width > height ? width : height);
      if (!paintVolumeCheckedMultiply(depth, width, scanlineCount) ||
          !paintVolumeCheckedMultiply(
            scanlineCount, profile.distanceTaskBytesPerScanline, taskBytes) ||
          !paintVolumeCheckedMultiply(maximumDimension, 1024ULL,
                                      scratchBytes) ||
          !paintVolumeCheckedAdd(fixedOverheadBytes, taskBytes,
                                 fixedOverheadBytes) ||
          !paintVolumeCheckedAdd(fixedOverheadBytes, scratchBytes,
                                 fixedOverheadBytes))
        {
          PaintAlgorithmMemoryAdmission overflow;
          overflow.reason = PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow;
          return overflow;
        }
    }

  return evaluatePaintAlgorithmMemoryAdmission(
    depth, width, height,
    profile.bytesPerVoxel, fixedOverheadBytes,
    provider, providerContext);
}

// Memory-profile smoke tests do not need Paint's Qt/OpenGL declarations.
#ifndef DRISHTI_VOLUMEOPERATIONS_MEMORY_PROFILE_ONLY

#include "commonqtclasses.h"
#include <QProgressDialog>
#include <QMap>
#include <QString>

#include <QGLViewer/vec.h>
using namespace qglviewer;

#include "mybitarray.h"


struct VOXEL
{
  int x, y, z;
  VOXEL() : x(0),y(0),z(0) {}
  VOXEL(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
};

struct VOXEL_H
{
  int x, y, z, h;
  VOXEL_H() : x(0),y(0),z(0),h(0) {}
  VOXEL_H(int x_, int y_, int z_, float h_) : x(x_), y(y_), z(z_), h(h_) {}
  bool operator>(const VOXEL_H& o) const { return h < o.h; } // higher priority means larger value 
};

struct VisibilityMapDirtyStruct
{
  int ds, ws, hs;
  int de, we, he;
  int tag;
  bool checkZero;
  int gradType;
  float minGrad, maxGrad;
  bool dirtyBit;
  
  void setBit(bool flag) { dirtyBit = flag; }
  void set(int ods, int ows, int ohs,
	   int ode, int owe, int ohe,
	   int otag, bool ocheckZero,
	   int ogradType, float ominGrad, float omaxGrad)
  {
    ds = ods;
    de = ode;
    ws = ows;
    we = owe;
    hs = ohs;
    he = ohe;
    tag = otag;
    checkZero = ocheckZero;
    gradType = ogradType;
    minGrad = ominGrad;
    maxGrad = omaxGrad;
    dirtyBit = false;
  }
  bool dirty() { return dirtyBit; }
  bool check(int ods, int ows, int ohs,
	     int ode, int owe, int ohe,
	     int otag, bool ocheckZero,
	     int ogradType, float ominGrad, float omaxGrad)
  {
    if (ds == ods && de == ode &&
	ws == ows && we == owe &&
	hs == ohs && he == ohe &&
	tag == otag &&
	checkZero == ocheckZero &&
	gradType == ogradType &&
	minGrad == ominGrad &&
	maxGrad == omaxGrad)
      return true;

    return false;
  }
};

class VolumeOperations
{
 public :
  static void setVolData(uchar*);
  static void setMaskData(uchar*);
  static void setGridSize(int, int, int);
  static void setClip(QList<Vec>, QList<Vec>);

  static QList<Vec> getSurfaceVoxels(qint64, qint64, qint64,
				     MyBitArray&);
  
  static void hatchConnectedRegion(int, int, int,
				   Vec, Vec,
				   int, int,
				   int, int,
				   int&, int&,
				   int&, int&,
				   int&, int&);

  static void connectedRegion(int, int, int,
			      Vec, Vec,
			      int, int,
			      int&, int&,
			      int&, int&,
			      int&, int&,
			      int, float, float);

  static void smoothConnectedRegion(int, int, int,
				    Vec, Vec,
				    int,
				    int&, int&,
				    int&, int&,
				    int&, int&,
				    int, float, float,
				    int);

  static void smoothAllRegion(Vec, Vec,
			      int,
			      int&, int&,
			      int&, int&,
			      int&, int&,
			      int, float, float,
			      int);

  static void removeSmallerComponents(Vec, Vec,
				      int,
				      int&, int&,
				      int&, int&,
				      int&, int&,
				      int, float, float);
  static void removeLargestComponents(Vec, Vec,
				      int,
				      int&, int&,
				      int&, int&,
				      int&, int&,
				      int, float, float);
  static void connectedComponents(Vec, Vec,
				  int,
				  int&, int&,
				  int&, int&,
				  int&, int&,
				  int, float, float);
  

  static void watershed(Vec, Vec, int,
			int,
			int&, int&,
			int&, int&,
			int&, int&,
			int, float, float);

  static void watershedPlus(Vec, Vec,
			    int&, int&,
			    int&, int&,
			    int&, int&,
			    int, float, float);

  static void watershedPriorityQueue(Vec, Vec, int,
				     int,
				     int&, int&,
				     int&, int&,
				     int&, int&,
				     int, float, float);


  static void distanceTransform(Vec, Vec, int,
				int&, int&,
				int&, int&,
				int&, int&,
				int, float, float);

  static void localThickness(Vec, Vec, int,
			     int&, int&,
			     int&, int&,
			     int&, int&,
			     int, float, float);

  static void resetTag(Vec, Vec, int,
		       int&, int&,
		       int&, int&,
		       int&, int&);

  static void shrinkwrap(Vec, Vec, int,
			 bool, int,
			 bool,
			 int, int, int, int,
			 int&, int&,
			 int&, int&,
			 int&, int&,
			 int, float, float);

  static void poreCharacterization(Vec, Vec,
				   int, int, int, int,
				   bool,
				   int, int, int, int,
				   int&, int&,
				   int&, int&,
				   int&, int&,
				   int, float, float);

  static void tagTubes(Vec, Vec, int,
		       bool,
		       int, int, int, int,
		       int&, int&,
		       int&, int&,
		       int&, int&,
		       int, float, float);

  static void setVisible(Vec, Vec,
			 int, bool,
			 int&, int&,
			 int&, int&,
			 int&, int&,
			 int, float, float);

  static void stepTags(Vec, Vec,
		       int, int,
		       int&, int&,
		       int&, int&,
		       int&, int&);
  
  static void mergeTags(Vec bmin, Vec bmax,
			int tag1, int tag2, bool useTF,
			int&, int&,
			int&, int&,
			int&, int&);

  static void erodeAll(Vec, Vec,
		       int, int,
		       int,
		       int&, int&,
		       int&, int&,
		       int&, int&,
		       int, float, float);  
  static void erodeConnected(int, int, int,
			     Vec, Vec, int,
			     int,
			     int&, int&,
			     int&, int&,
			     int&, int&,
			     int, float, float);

  static void dilateAllTags(Vec, Vec,
			    int,
			    int&, int&,
			    int&, int&,
			    int&, int&,
			    int, float, float);
  static void dilateAll(Vec, Vec, int,
			int,
			int&, int&,
			int&, int&,
			int&, int&,
			bool,
			int, float, float,
			bool showProgress = true);
  static void dilateConnected(int, int, int,
			      Vec, Vec, int,
			      int,
			      int&, int&,
			      int&, int&,
			      int&, int&,
			      bool,
			      int, float, float);

  static void openAll(Vec, Vec, int,
		      int, int,
		      int&, int&,
		      int&, int&,
		      int&, int&,
		      int, float, float);

  static void closeAll(Vec, Vec, int,
		       int, int,
		       int&, int&,
		       int&, int&,
		       int&, int&,
		       int, float, float);

  
  
  static void modifyOriginalVolume(Vec, Vec, int,
				   int&, int&,
				   int&, int&,
				   int&, int&);


  static void bakeCurves(uchar*,
			 int, int,
			 int, int,
			 int, int,
			 int,
			 int, float, float);
			 

  static float calcGrad(int, qint64, qint64, qint64,
			int, int, int,
			uchar*, ushort*);

  static bool checkClipped(Vec);

  static void setVisibilityMapDirtyBit(bool);
  static void genVisibilityMap(int, float, float);
  static MyBitArray* getVisibilityMap() { return &m_visibilityMap; }

  static void sortLabels(Vec, Vec,
			 int, float, float);
  
  static void getVisibleRegion(int, int, int,
			       int, int, int,
			       int, bool,
			       int, float, float,
			       MyBitArray&,
			       bool showProgress = true);

  static bool saveToROI(Vec, Vec,
			int,
			int&, int&,
			int&, int&,
			int&, int&,
			int, float, float);
  static bool roiOperation(Vec, Vec,
			   int,
			   int&, int&,
			   int&, int&,
			   int&, int&,
			   int, float, float);
  static void deleteROI();
  
 private :
  static int m_depth, m_width, m_height;
  static uchar *m_volData;
  static ushort *m_volDataUS;
  static ushort *m_maskDataUS;

  static VisibilityMapDirtyStruct m_vmDirty;
  static MyBitArray m_visibilityMap;
  
  static QList<Vec> m_cPos;
  static QList<Vec> m_cNorm;

  static QMap<QString, MyBitArray> m_roi;
  
  static void getConnectedRegion(int, int, int,
				 int, int, int,
				 int, int, int,
				 int, bool,
				 MyBitArray&,
				 int, float, float);
  static void getConnectedRegionFromBitmask(int, int, int,
					    int, int, int,
					    int, int, int,
					    MyBitArray&,
					    MyBitArray&);

  static void getRegionConnectedToROI(int, int, int,
				      int, int, int,
				      MyBitArray&,
				      MyBitArray&);

  static void shrinkwrapSlice(uchar*, int, int);

  static void dilateBitmask(int, bool,
			    qint64, qint64, qint64,
			    MyBitArray&);
  static void _dilatebitmask(int, bool,
			     qint64, qint64, qint64,
			     MyBitArray&,
			     bool showProgress = true);

  static void openCloseBitmask(int, int,
			       bool,
			       qint64, qint64, qint64,
			       MyBitArray&);

  static void parVisibleRegionGeneration(QList<QVariant>);

  
  static void getTransparentRegion(int, int, int,
				   int, int, int,
				   MyBitArray&,
				   int, float, float);
  static void parTransparentRegionGeneration(QList<QVariant>);  

  
  static void bakeC(int, int, int,
		    int, int, int,
		    int,
		    int, float, float,
		    uchar*);
  static void parBakeCurves(QList<QVariant>);


  static void resetT(int, int, int,
		     int, int, int,
		     int);
  static void parResetTag(QList<QVariant>);

  
  static void writeToMask(int, int, int,
			  int, int, int,
			  int,
			  int, float, float,
			  bool,
			  MyBitArray&);
  static void parWriteToMask(QList<QVariant>);

  
  static void convertToVDBandSmooth(int, int, int,
				    int, int, int,
				    MyBitArray&,
				    int);

  static QString getROIName();

  static void distDilate(float*, float*,
			 qint64, qint64, qint64,
			 int, int);
  static void parDistDilate(QList<QVariant>);

  static void padBitmask(MyBitArray&,
			 MyBitArray&,
			 qint64, qint64, qint64,
			 bool, int);
  static void unpadBitmask(MyBitArray&,
			   MyBitArray&,
			   qint64, qint64, qint64,
			   int);

  static VOXEL findSteepestDescent(float*,
				   int, int, int,
				   qint64, qint64, qint64);

};

#endif // DRISHTI_VOLUMEOPERATIONS_MEMORY_PROFILE_ONLY

#endif
