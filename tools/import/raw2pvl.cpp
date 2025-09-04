#include "vdbvolume.h"

#include "global.h"
#include "staticfunctions.h"
#include "raw2pvl.h"
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>

#include <QtXml>
#include <QFile>

#include <QtConcurrentMap>
#include <QTableWidget>
#include <QPushButton>

#include "savepvldialog.h"
#include "volumefilemanager.h"
#include "propertyeditor.h"
#include "meshtools.h"


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
#include <openvdb/tools/GridTransformer.h>
#include <openvdb/tools/Filter.h>
#include <openvdb/Grid.h>


using namespace std;


#ifdef Q_OS_WIN
#include <float.h>
#define ISNAN(v) _isnan(v)
#else
#define ISNAN(v) isnan(v)
#endif

#define REMAPVOLUME()							\
  {									\
    for(uint j=0; j<width*height; j++)					\
      {									\
	float v = ptr[j];						\
	int idx;							\
	float frc;							\
	if (v <= rawMap[0] || ISNAN(v))					\
	  {								\
	    idx = 0;							\
	    frc = 0;							\
	  }								\
	else if (v >= rawMap[rawSize])					\
	  {								\
	    idx = rawSize-1;						\
	    frc = 1;							\
	  }								\
	else								\
	  {								\
	    for(uint m=0; m<rawSize; m++)				\
	      {								\
		if (v >= rawMap[m] &&					\
		    v <= rawMap[m+1])					\
		  {							\
		    idx = m;						\
		    frc = ((float)v-rawMap[m])/				\
		      (rawMap[m+1]-rawMap[m]);				\
		  }							\
	      }								\
	  }								\
									\
	int pv = pvlMap[idx] + frc*(pvlMap[idx+1]-pvlMap[idx]);		\
	pvl[j] = pv;							\
      }									\
  }


void
Raw2Pvl::applyMapping(uchar *raw, int voxelType,
		      QList<float> rawMap,
		      uchar *pvlslice, int pvlbpv,
		      QList<int> pvlMap,
		      int width, int height)
{
  int rawSize = rawMap.size()-1;

  if (rawMap.count() == pvlMap.count())
    {
      bool same = true;
      for(int i=0; i<rawMap.count(); i++)
	if (rawMap[i] != pvlMap[i])
	  same = false;

      if (same)
	{
	  memcpy(pvlslice, raw, width*height*pvlbpv);
	  return;
	}
    }


  if (pvlbpv == 1)
    {
      uchar *pvl = (uchar*)pvlslice;
      if (voxelType == _UChar)
	{
	  uchar *ptr = raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Char)
	{
	  char *ptr = (char*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _UShort)
	{
	  ushort *ptr = (ushort*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Short)
	{
	  short *ptr = (short*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Int)
	{
	  int *ptr = (int*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Float)
	{
	  float *ptr = (float*)raw;
	  REMAPVOLUME();
	}
    }
  else
    {
      ushort *pvl = (ushort*)pvlslice;
      if (voxelType == _UChar)
	{
	  uchar *ptr = raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Char)
	{
	  char *ptr = (char*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _UShort)
	{
	  ushort *ptr = (ushort*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Short)
	{
	  short *ptr = (short*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Int)
	{
	  int *ptr = (int*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Float)
	{
	  float *ptr = (float*)raw;
	  REMAPVOLUME();
	}
    }
}

//-----------------------------
QString
getPvlNcFilename()
{
  QFileDialog fdialog(0,
		      "Save processed volume",
		      Global::previousDirectory(),
		      "Drishti (*.pvl.nc) ;; MetaImage (*.mhd) ;; VDB (*.vdb)");
  fdialog.setAcceptMode(QFileDialog::AcceptSave);

  if (!fdialog.exec() == QFileDialog::Accepted)
    return "";

  QString pvlFilename = fdialog.selectedFiles().value(0);
  if (fdialog.selectedNameFilter() == "VDB (*.vdb)")
    {
      if (!pvlFilename.endsWith(".vdb"))
	pvlFilename += ".vdb";
      return pvlFilename;
    }
  if (fdialog.selectedNameFilter() == "MetaImage (*.mhd)")
    {
      if (!pvlFilename.endsWith(".mhd"))
	pvlFilename += ".mhd";
      return pvlFilename;
    }
  if (fdialog.selectedNameFilter() == "Drishti (*.pvl.nc)")
    {
      if (pvlFilename.endsWith(".pvl.nc.pvl.nc"))
	  pvlFilename.chop(7);
      if (!pvlFilename.endsWith(".pvl.nc"))
	pvlFilename += ".pvl.nc";

      return pvlFilename;
    }

  return "";
}

bool
checkParIsoGen()
{
  bool pariso = false;
  bool ok = false;
  QStringList type;
  type << "No (default) - Do one after another";
  type << "Yes - Try to cram as many as possible";  
  QString option = QInputDialog::getItem(0,
		   "Parallel Isosurface generation",
		   "Fire multiple isosurface generation threads ?\nFor large surfaces or NetCDF files you might be better off sequential",
		    type,
		    0,
		    false,
		    &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "No")
	{
	  pariso = false;
	  QMessageBox::information(0, "Isosurface generation", "Generating one after another");
	}
      else
	{
	  pariso = true;
	  QMessageBox::information(0, "Isosurface generation", "Will generate multiple surfaces in parallel");

//	  int maxThreads = QInputDialog::getInt(0, "Max Thread Count",
//						QString("Maximum threads (%1)\nthat can be used").\
//						arg(QThread::idealThreadCount()),
//						QThread::idealThreadCount(),
//						1,
//						QThread::idealThreadCount());
//	  QThreadPool::globalInstance()->setMaxThreadCount(maxThreads);
	}
    }
  else
    {
      pariso = false;
      QMessageBox::information(0, "Isosurface generation", "Generating one after another");
    }

  return pariso;
}


bool
saveSliceZeroAtTop()
{
  bool save0attop = true;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - (default)";  
  slevels << "No - save slice 0 as bottom slice";
  QString option = QInputDialog::getItem(0,
		   "Save Data",
		   "Save slice 0 as top slice ?",
		    slevels,
			  0,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "No")
	{
	  save0attop = false;
	  QMessageBox::information(0, "Save Data", "First slice is now bottom slice.");
	}
    }

  return save0attop;
}

bool
getSaveRawFile()
{
  bool saveRawFile = false;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - save raw file";
  slevels << "No";  
  QString option = QInputDialog::getItem(0,
		   "Save Processed Volume",
		   "Save RAW file along with preprocessed volume ?",
		    slevels,
			  1,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "Yes")
	saveRawFile = true;
    }
  else
    QMessageBox::information(0, "RAW Volume", "Will not save raw volume");

  return saveRawFile;
}

QString
getRawFilename(QString pvlFilename)
{
  QString rawfile = QFileDialog::getSaveFileName(0,
						 "Save processed volume",
						 QFileInfo(pvlFilename).absolutePath(),
						 "RAW Files (*.raw)");
//						 0,
//						 QFileDialog::DontUseNativeDialog);
  return rawfile;
}

int
getZSubsampling(int dsz, int wsz, int hsz)
{
  bool ok = false;
  QStringList slevels;

  slevels.clear();
  slevels << "No subsampling in Z";
  slevels << QString("2 [Z(%1) %2 %3]").arg(dsz/2).arg(wsz).arg(hsz);
  slevels << QString("3 [Z(%1) %2 %3]").arg(dsz/3).arg(wsz).arg(hsz);
  slevels << QString("4 [Z(%1) %2 %3]").arg(dsz/4).arg(wsz).arg(hsz);
  slevels << QString("5 [Z(%1) %2 %3]").arg(dsz/5).arg(wsz).arg(hsz);
  slevels << QString("6 [Z(%1) %2 %3]").arg(dsz/6).arg(wsz).arg(hsz);
  QString option = QInputDialog::getItem(0,
					 "Volume Size",
					 "Z subsampling",
					 slevels,
					 0,
					 false,
					 &ok);
  int svslz = 1;
  if (ok)
    {   
      QStringList op = option.split(' ');
      svslz = qMax(1, op[0].toInt());
    }
  return svslz;
}

int
getXYSubsampling(int svslz, int dsz, int wsz, int hsz)
{
  bool ok = false;
  QStringList slevels;

  slevels.clear();
  slevels << "No subsampling in XY";
  slevels << QString("2 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/2).arg(hsz/2);
  slevels << QString("3 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/3).arg(hsz/3);
  slevels << QString("4 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/4).arg(hsz/4);
  slevels << QString("5 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/5).arg(hsz/5);
  slevels << QString("6 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/6).arg(hsz/6);
  QString option = QInputDialog::getItem(0,
					 "Volume Size",
					 "XY subsampling",
					 slevels,
					 0,
					 false,
					 &ok);
  int svsl = 1;
  if (ok)
    {   
      QStringList op = option.split(' ');
      svsl = qMax(1, op[0].toInt());
    }
  return svsl;
}


#define AVERAGEFILTER(n)			\
  {						\
    for(int j=0; j<width; j++)			\
      for(int k=0; k<height; k++)		\
	{					\
	  float sum = 0;			\
	  for(int i=0; i<2*n+1; i++)		\
	    sum += weights[i]*pv[i][j*height+k]; \
	  p[j*height + k] = sum/wsum;		\
	}					\
  }

#define DILATEFILTER(n)					\
  {							\
    for(int j=0; j<width; j++)				\
      for(int k=0; k<height; k++)			\
	{						\
	  float avg = 0;				\
	  for(int i=0; i<2*n+1; i++)			\
	    avg = qMax(avg,(float)pv[i][j*height+k]);	\
	  p[j*height + k] = avg;			\
	}						\
  }

void
Raw2Pvl::applyMeanFilter(uchar **val, uchar *vg,
			 int voxelType,
			 int width, int height,
			 int spread, bool dilateFilter,
			 float *weights)
{
  float wsum = weights[2*spread+2];
 
  if (voxelType == _UChar)
    {
      uchar **pv = val;
      uchar *p  = vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Char)
    {
      char **pv = (char**)val;
      char *p  = (char*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _UShort)
    {
      ushort **pv = (ushort**)val;
      ushort *p  = (ushort*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Short)
    {
      short **pv = (short**)val;
      short *p  = (short*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Int)
    {
      int **pv = (int**)val;
      int *p  = (int*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Float)
    {
      float **pv = (float**)val;
      float *p  = (float*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
}


#define SLICEAVERAGEFILTER(n)					\
  {								\
    for(int i=0; i<height; i++)					\
      for(int j=0; j<width; j++)				\
	{							\
	  float pj = 0;						\
	  int jdx = 0;						\
	  for(int j1=j-n; j1<=j+n; j1++)			\
	    {							\
	      int idx = qBound(0, j1, width-1)*height+i;	\
	      pj += weights[jdx]*p[idx];			\
	      jdx ++;						\
	    }							\
	  pv[j*height+i] = pj/wsum;				\
	}							\
    								\
    for(int j=0; j<width; j++)					\
      for(int i=0; i<height; i++)				\
	{							\
	  float pj = 0;						\
	  int jdx = 0;						\
	  for(int i1=i-n; i1<=i+n; i1++)			\
	    {							\
	      int idx = j*height + qBound(0, i1, height-1);	\
	      pj += weights[jdx]*pv[idx];			\
	      jdx ++;						\
	    }							\
	  p[j*height+i] = pj/wsum;				\
	}							\
    								\
  }


#define SLICEDILATEFILTER(n)					\
  {								\
    for(int i=0; i<height; i++)					\
      for(int j=0; j<width; j++)				\
	{							\
	  float pj = 0;						\
	  int jst = qMax(0, j-n);				\
	  int jed = qMin(width-1, j+n);				\
	  for(int j1=jst; j1<=jed; j1++)			\
	    {							\
	      int idx = qBound(0, j1, width-1)*height+i;	\
	      pj = qMax((float)p[idx],pj);			\
	    }							\
	  pv[j*height+i] = pj;					\
	}							\
    								\
    for(int j=0; j<width; j++)					\
      for(int i=0; i<height; i++)				\
	{							\
	  float pi = 0;						\
	  int ist = qMax(0, i-n);				\
	  int ied = qMin(height-1, i+n);			\
	  for(int i1=ist; i1<=ied; i1++)			\
	    {							\
	      int idx = j*height+qBound(0, i1, height-1);	\
	      pi = qMax((float)pv[idx],pi);			\
	    }							\
	  p[j*height+i] = pi;					\
	}							\
  }

void
Raw2Pvl::applyMeanFilterToSlice(uchar *val, uchar *vg,
				int voxelType,
				int width, int height,
				int spread,
				bool dilateFilter,
				float *weights)
{
  float wsum = weights[2*spread+2];
 
  if (voxelType == _UChar)
    {
      uchar *p = val;
      uchar *pv  = vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Char)
    {
      char *p = (char*)val;
      char *pv = (char*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _UShort)
    {
      ushort *p = (ushort*)val;
      ushort *pv = (ushort*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Short)
    {
      short *p = (short*)val;
      short *pv = (short*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Int)
    {
      int *p = (int*)val;
      int *pv = (int*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Float)
    {
      float *p = (float*)val;
      float *pv = (float*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
}

void
Raw2Pvl::savePvlHeader(QString pvlFilename,
		       bool saveRawFile, QString rawfile,
		       int voxelType, int pvlVoxelType, int voxelUnit,
		       int d, int w, int h,
		       float vx, float vy, float vz,
		       QList<float> rawMap, QList<int> pvlMap,
		       QString description,
		       int slabSize)
{
  QString xmlfile = pvlFilename;

  QDomDocument doc("Drishti_Header");

  QDomElement topElement = doc.createElement("PvlDotNcFileHeader");
  doc.appendChild(topElement);

  {      
    QString vstr;
    if (saveRawFile)
      {
	// save relative path for the rawfile
	QFileInfo fileInfo(pvlFilename);
	QDir direc = fileInfo.absoluteDir();
	vstr = direc.relativeFilePath(rawfile);
      }
    else
      vstr = "";

    QDomElement de0 = doc.createElement("rawfile");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
      
  {      
    QString vstr;
    if (voxelType == Raw2Pvl::_UChar)      vstr = "unsigned char";
    else if (voxelType == Raw2Pvl::_Char)  vstr = "char";
    else if (voxelType == Raw2Pvl::_UShort)vstr = "unsigned short";
    else if (voxelType == Raw2Pvl::_Short) vstr = "short";
    else if (voxelType == Raw2Pvl::_Int)   vstr = "int";
    else if (voxelType == Raw2Pvl::_Float) vstr = "float";
    
    QDomElement de0 = doc.createElement("voxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }


  {      
    QString vstr;
    if (pvlVoxelType == Raw2Pvl::_UChar)      vstr = "unsigned char";
    else if (pvlVoxelType == Raw2Pvl::_Char)  vstr = "char";
    else if (pvlVoxelType == Raw2Pvl::_UShort)vstr = "unsigned short";
    else if (pvlVoxelType == Raw2Pvl::_Short) vstr = "short";
    else if (pvlVoxelType == Raw2Pvl::_Int)   vstr = "int";
    else if (pvlVoxelType == Raw2Pvl::_Float) vstr = "float";
    
    QDomElement de0 = doc.createElement("pvlvoxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }


  {      
    QDomElement de0 = doc.createElement("gridsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(d).arg(w).arg(h));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr;
    if (voxelUnit == Raw2Pvl::_Nounit)         vstr = "no units";
    else if (voxelUnit == Raw2Pvl::_Angstrom)  vstr = "angstrom";
    else if (voxelUnit == Raw2Pvl::_Nanometer) vstr = "nanometer";
    else if (voxelUnit == Raw2Pvl::_Micron)    vstr = "micron";
    else if (voxelUnit == Raw2Pvl::_Millimeter)vstr = "millimeter";
    else if (voxelUnit == Raw2Pvl::_Centimeter)vstr = "centimeter";
    else if (voxelUnit == Raw2Pvl::_Meter)     vstr = "meter";
    else if (voxelUnit == Raw2Pvl::_Kilometer) vstr = "kilometer";
    else if (voxelUnit == Raw2Pvl::_Parsec)    vstr = "parsec";
    else if (voxelUnit == Raw2Pvl::_Kiloparsec)vstr = "kiloparsec";
    
    QDomElement de0 = doc.createElement("voxelunit");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("voxelsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(vx).arg(vy).arg(vz));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {
    QString vstr = description.trimmed();
    QDomElement de0 = doc.createElement("description");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("slabsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(slabSize));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {      
    QString vstr;
    for(int i=0; i<rawMap.size(); i++)
      vstr += QString("%1 ").arg(rawMap[i]);
    
    QDomElement de0 = doc.createElement("rawmap");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr;
    for(int i=0; i<pvlMap.size(); i++)
      vstr += QString("%1 ").arg(pvlMap[i]);
    
    QDomElement de0 = doc.createElement("pvlmap");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  QFile f(xmlfile.toUtf8().data());
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
}

void
Raw2Pvl::savePvl(VolumeData* volData,
		 int dmin, int dmax,
		 int wmin, int wmax,
		 int hmin, int hmax,
		 QStringList timeseriesFiles)
{

  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }

  //------------------------------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int slabSize = (1024*1024*1024)/(bpv*wsz*hsz);
//  if (slabSize < dsz)
//    {  
//      QStringList items;
//      items << "no" << "yes";
//      QString yn = QInputDialog::getItem(0, "Split Volume",
//					 "Split volume larger than 1Gb into multiple files ?",
//					 items,
//					 0,
//					 false);
//      //*** max 1Gb per slab
//      if (yn != "yes") // put all in a single file
//	slabSize = dsz+1;
//    }
  slabSize = dsz+1;
  //------------------------------------------------------

  QString pvlFilename = getPvlNcFilename();
  if (pvlFilename.endsWith(".mhd"))
    {
      saveMHD(pvlFilename,
	      volData,
	      dmin, dmax,
	      wmin, wmax,
	      hmin, hmax);
      return;
    }

    if (pvlFilename.endsWith(".vdb"))
    {
      int tsfcount = qMax(1, timeseriesFiles.count());
      if (tsfcount == 1)
	{
	  saveVDB(-1, pvlFilename, volData);
	  QMessageBox::information(0, "Save VDB", "Volume save to "+pvlFilename);
	}
      else
	{
	  for (int tsf=0; tsf<tsfcount; tsf++)
	    {
	      QString pvlflnm = pvlFilename;
	      if (tsfcount > 1)
		{
		  QFileInfo ftpvl(pvlFilename);
		  QFileInfo ftraw(timeseriesFiles[tsf]);
		  pvlflnm = QFileInfo(ftpvl.absolutePath(),
				      ftraw.completeBaseName() + ".vdb").absoluteFilePath();
		  
		  volData->replaceFile(timeseriesFiles[tsf]);
		}
	      
	      if (!saveVDB(tsf, pvlflnm, volData))
		return;
		  
	    }
	  QMessageBox::information(0, "Save VDB", "Volumes saved to VDB files");
	}
      return;
    }

    
  if (pvlFilename.count() < 4)
    {
      QMessageBox::information(0, "pvl.nc", "No .pvl.nc filename chosen.");
      return;
    }

  bool save0AtTop = saveSliceZeroAtTop();;

  bool saveRawFile = getSaveRawFile();

  QString rawfile;
  if (saveRawFile) rawfile = getRawFilename(pvlFilename);
  if (rawfile.isEmpty())
    saveRawFile = false;

  int svslz = getZSubsampling(dsz, wsz, hsz);
  int svsl = getXYSubsampling(svslz, dsz, wsz, hsz);

  int dsz2 = dsz/svslz;
  int wsz2 = wsz/svsl;
  int hsz2 = hsz/svsl;
  int svsl3 = svslz*svsl*svsl;
  //------------------------------------------------------

  //------------------------------------------------------
  // get final volume size
  int final_dsz2 = dsz2;
  int final_wsz2 = wsz2;
  int final_hsz2 = hsz2;
  int pad_value = 0;
  int sfd = 0;
  int sfw = 0;
  int sfh = 0;
  int efd = 0;
  int efw = 0;
  int efh = 0;
  {
    bool ok;
    QString text;
    text = QInputDialog::getText(0,
				 "Final Volume Grid Size With Padding",
				 "Final Volume Grid Size With Padding",
				 QLineEdit::Normal,
				 QString("%1 %2 %3").\
				 arg(final_dsz2).\
				 arg(final_wsz2).\
				 arg(final_hsz2),
				 &ok);
    if (ok && !text.isEmpty())
      {
	QStringList list = text.split(" ", QString::SkipEmptyParts);
	if (list.count() == 3)
	  {
	    final_dsz2 = qMax(dsz2, list[0].toInt());
	    final_wsz2 = qMax(wsz2, list[1].toInt());
	    final_hsz2 = qMax(hsz2, list[2].toInt());

	    int td = final_dsz2 - dsz2;
	    int tw = final_wsz2 - wsz2;
	    int th = final_hsz2 - hsz2;

	    sfd = td/2;
	    efd = td - sfd;

	    sfw = tw/2;
	    efw = tw - sfw;

	    sfh = th/2;
	    efh = th - sfh;

	    if (td != 0 || tw != 0 || th != 0)
	      {
		QString text;
		text = QInputDialog::getText(0,
					     "Pad volume With Value",
					     "Pad Volume With Value",
					     QLineEdit::Normal,
					     "0",
					     &ok);
		if (ok && !text.isEmpty())
		  pad_value = text.toInt();
	      }
	  }
      }

    slabSize = final_dsz2+1;
  }
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  int vu = volData->voxelUnit();
  savePvlDialog.setVoxelUnit(vu);
  // scale the voxelsize according to subsampling used
  savePvlDialog.setVoxelSize(vx*svsl, vy*svsl, vz*svslz);
  savePvlDialog.setDescription(desc);
  savePvlDialog.exec();

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  bool invertData = savePvlDialog.invertData();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);


  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();

  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;

  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  bool subsample = (svsl > 1 || svslz > 1);

  //--------------------------
  int filterType = 0;
  if (subsample && spread > 0)
    {
      bool ok = true;
      
      QStringList items;
      items << "Tri-Linear Interpolation";
      items << "No Interpolation";
      QString item = QInputDialog::getItem(0,
					   QString("Subsampling Filter (%1)").arg(spread),
					   "FOR SEGMENTED DATA USE - NO INTERPOLATION",
					   items,
					   0,
					   false,
					   &ok);
      if (ok && !item.isEmpty())
	{
	  QStringList op = item.split(' ');
	  if (op[0] == "No")
	    {
	      filterType = 1;
	      spread = 0;
	    }
	}
    }
  //--------------------------
  
  int nbytes = rvwidth*rvheight*bpv;
  double *filtervol = new double[wsz2*hsz2];
  uchar *pvlslice = new uchar[pvlbpv*wsz2*hsz2];
  uchar *raw = new uchar[nbytes];
  uchar **val;
  if (spread > 0)
    {
      val = new uchar*[2*spread+1];
      for (int i=0; i<2*spread+1; i++)
	val[i] = new uchar[nbytes];
    }
  int rawSize = rawMap.size()-1;
  int width = wsz2;
  int height = hsz2;
  bool trim = (dmin != 0 ||
	       wmin != 0 ||
	       hmin != 0 ||
	       dsz2 != rvdepth ||
	       wsz2 != rvwidth ||
	       hsz2 != rvheight);

  uchar *final_val = new uchar[pvlbpv*final_wsz2*final_hsz2];

  VolumeFileManager rawFileManager;
  VolumeFileManager pvlFileManager;

  
  
  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());
  
  //------------------------------------------------------
  int tsfcount = qMax(1, timeseriesFiles.count());
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  break;
	}
	  
      QString pvlflnm = pvlFilename;
      QString rawflnm = rawfile;

      if (tsfcount > 1)
	{
	  QFileInfo ftpvl(pvlFilename);
	  QFileInfo ftraw(timeseriesFiles[tsf]);
	  pvlflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".pvl.nc").absoluteFilePath();

	  rawflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".raw").absoluteFilePath();

	  volData->replaceFile(timeseriesFiles[tsf]);
	}

      pvlFileManager.setBaseFilename(pvlflnm);
//      pvlFileManager.setDepth(dsz2);
//      pvlFileManager.setWidth(wsz2);
//      pvlFileManager.setHeight(hsz2);
      pvlFileManager.setDepth(final_dsz2);
      pvlFileManager.setWidth(final_wsz2);
      pvlFileManager.setHeight(final_hsz2);
      pvlFileManager.setVoxelType(pvlVoxelType);
      pvlFileManager.setHeaderSize(13);
      pvlFileManager.setSlabSize(slabSize);
      pvlFileManager.setSliceZeroAtTop(save0AtTop);
      pvlFileManager.createFile(true);
      
      if (saveRawFile)
	{
	  rawFileManager.setBaseFilename(rawflnm);
	  rawFileManager.setDepth(dsz2);
	  rawFileManager.setWidth(wsz2);
	  rawFileManager.setHeight(hsz2);
	  rawFileManager.setVoxelType(voxelType);
	  rawFileManager.setHeaderSize(13);
	  rawFileManager.setSlabSize(slabSize);
	  rawFileManager.setSliceZeroAtTop(save0AtTop);
	  if (rawFileManager.exists())
	    {
	      bool ok = false;
	      QStringList slevels;
	      slevels << "Yes - overwrite";
	      slevels << "No";  
	      QString option = QInputDialog::getItem(0,
						     "Save RAW Volume",
						     QString("%1 exists - Overwrite ?"). \
						     arg(rawFileManager.fileName()),
						     slevels,
						     0,
						     false,
						     &ok);
	      if (!ok)
		return;
	      
	      QStringList op = option.split(' ');
	      if (op[0] != "Yes")
		{
		  QMessageBox::information(0, "Save",
	        QString("Please choose a different name for the preprocessed volume - RAW file not overwritten"));
		  return;
		}
	    }
	  rawFileManager.createFile(true);
	}
      //------------------------------------------------------


      savePvlHeader(pvlflnm,
		    saveRawFile, rawflnm+".001",
		    voxelType, pvlVoxelType, voxelUnit,
		    final_dsz2, final_wsz2, final_hsz2,
		    vx, vy, vz,
		    rawMap, pvlMap,
		    description,
		    slabSize);


      // ------------------
      // add padding
      if (sfd > 0)
	{
	  memset(final_val, pad_value, pvlbpv*final_wsz2*final_hsz2);
	  for(int esl=0; esl<sfd; esl++)
	    pvlFileManager.setSlice(esl, final_val);
	}
      // ------------------
	

      // ------------------
      // calculate weights for Gaussian filter
      float weights[100];
      float wsum = 0.0;
      for(int i=-spread; i<=spread; i++)
	{
	  float wgt = qExp(-qAbs(i)/(2.0*spread*spread))/(M_PI*2*spread*spread);
	  wsum +=  wgt;
	  weights[i+spread] = wgt;
	}
      weights[2*spread+2] = wsum;
      // ------------------
      
      
      for(int dd=0; dd<dsz2; dd++)
	{

	  if (progress.wasCanceled())
	    {
	      progress.setValue(100);  
	      QMessageBox::information(0, "Save", "-----Aborted-----");
	      break;
	    }

	  int d0 = dmin + dd*svslz; 
	  int d1 = d0 + svslz-1;

	  if (spread == 0) // No Filter - Nearest Neighbour
	    {
	      d0 = dmin + dd*svslz;
	      d1 = d0;
	    }
	  
	  progress.setValue((int)(100*(float)dd/(float)dsz2));
	  qApp->processEvents();
	  
	  memset(filtervol, 0, 8*wsz2*hsz2);
	  for (int d=d0; d<=d1; d++)
	    {
	      if (spread > 0)
		{
		  if (d == d0)
		    {
		      volData->getDepthSlice(d, val[spread]);
		      applyMeanFilterToSlice(val[spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);

		      for(int i=-spread; i<0; i++)
			{
			  if (d+i >= 0)
			    volData->getDepthSlice(d+i, val[spread+i]);
			  else
			    volData->getDepthSlice(0, val[spread+i]);

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter, weights);
			}
		      
		      for(int i=1; i<=spread; i++)
			{
			  if (d+i < rvdepth)
			    volData->getDepthSlice(d+i, val[spread+i]);
			  else
			    volData->getDepthSlice(rvdepth-1, val[spread+i]);

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter, weights);
			}
		    }
		  else if (d < rvdepth-spread)
		    {
		      volData->getDepthSlice(d+spread, val[2*spread]);
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);
		    }		  
		  else
		    {
		      volData->getDepthSlice(rvdepth-1, val[2*spread]);
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);
		    }		  
		  // smoothed data is now in val[2*spread]
		  // copy that into raw
		  memcpy(raw, val[2*spread], rvwidth*rvheight*bpv);
		}
	      else // spread == 0
		volData->getDepthSlice(d, raw);
	      
	      if (spread > 0)
		{
		  applyMeanFilter(val, raw,
				  voxelType, rvwidth, rvheight,
				  spread, dilateFilter, weights);
		  
		  // now shift the planes
		  uchar *tmp = val[0];
		  for(int i=0; i<2*spread; i++)
		    val[i] = val[i+1];
		  val[2*spread] = tmp;
		}
	      
	      if (trim || subsample)
		{
		  int fi = 0;
		  for(int j=0; j<wsz2; j++)
		    {
		      int y0 = wmin+j*svsl;
		      int y1 = y0+svsl-1;
		      for(int i=0; i<hsz2; i++)
			{
			  int x0 = hmin+i*svsl;
			  int x1 = x0+svsl-1;
			  for(int y=y0; y<=y1; y++)
			    for(int x=x0; x<=x1; x++)
			      {
				if (spread > 0)
				  {
				    if (voxelType == _UChar)
				      { uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _Char)
				      { char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _UShort)
				      { ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _Short)
				      { short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _Int)
				      { int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _Float)
				      { float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				  }
				else // no filter
				  {
				    if (voxelType == _UChar)
				      { uchar *ptr = raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _Char)
				      { char *ptr = (char*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _UShort)
				      { ushort *ptr = (ushort*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _Short)
				      { short *ptr = (short*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _Int)
				      { int *ptr = (int*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _Float)
				      { float *ptr = (float*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				  }
			      }
			  fi++;
			}
		    }
		} // trim || subsample
	    }
	  
	  if (trim || subsample)
	    {
	      if (spread > 0)
		{
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    filtervol[fi] /= svsl3;
		}
	      
	      if (voxelType == _UChar)
		{
		  uchar *ptr = raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Char)
		{
		  char *ptr = (char*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Short)
		{
		  short *ptr = (short*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Int)
		{
		  int *ptr = (int*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Float)
		{
		  float *ptr = (float*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    } // trim || subsample
	  
	  if (saveRawFile)
	    rawFileManager.setSlice(dd, raw);
	  
	  applyMapping(raw, voxelType, rawMap,
		       pvlslice, pvlbpv, pvlMap,
		       width, height);

	  if (invertData)
	    {
	      if (pvlbpv == 1)
		{
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    pvlslice[fi] = 255-pvlslice[fi];
		}
	      else
		{
		  ushort *ptr = (ushort*)pvlslice;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = 65535-ptr[fi];
		}
	    }
	  
	  if (sfw == 0 && sfh == 0)
	    pvlFileManager.setSlice(sfd+dd, pvlslice);
	  else // add padding if required
	    {
	      memset(final_val, pad_value, pvlbpv*final_wsz2*final_hsz2);
	      if (pvlbpv == 1)
		{
		  for(int wi=0; wi<wsz2; wi++)
		    for(int hi=0; hi<hsz2; hi++)
		      final_val[(wi+sfw)*final_hsz2+(hi+sfh)] = pvlslice[wi*hsz2+hi];
		}
	      else
		{
		  for(int wi=0; wi<wsz2; wi++)
		    for(int hi=0; hi<hsz2; hi++)
		      ((ushort*)final_val)[(wi+sfw)*final_hsz2+(hi+sfh)] = ((ushort*)pvlslice)[wi*hsz2+hi];
		}
	      pvlFileManager.setSlice(sfd+dd, final_val);
	    }
	}
    }

  // -------------------------
  // add padding if required
  if (efd > 0)
    {
      memset(final_val, pad_value, pvlbpv*final_wsz2*final_hsz2);
      for(int esl=0; esl<efd; esl++)
	pvlFileManager.setSlice(dsz2+sfd+esl, final_val);
    }
  // -------------------------

  delete [] final_val;

  delete [] filtervol;
  delete [] pvlslice;
  delete [] raw;
  if (spread > 0)
    {
      for (int i=0; i<2*spread+1; i++)
	delete [] val[i];
      delete [] val;
    }
  
  progress.setValue(100);


  QMessageBox mb;
  mb.setWindowTitle("Save");
  mb.setText("-----Done-----");
  mb.setWindowFlags(Qt::Dialog|Qt::WindowStaysOnTopHint);
  mb.exec();
  
//QMessageBox::information(0, "Save", "-----Done-----");

}

void
saveSettings(int memGb,
	     int spareMb)
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".meshgenerator");

  QFile fin(settingsFile.absoluteFilePath());
  if (fin.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QTextStream out(&fin);
      out << "main memory :: " << memGb << "\n";
      out << "keep spare :: " << spareMb << "\n";
    }
}

bool
loadSettings(int &memGb,
	     int &spareMb)
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".meshgenerator");

  memGb = 1;
  spareMb = 500;

  bool ok = false;
  if (settingsFile.exists())
    {
      QFile fin(settingsFile.absoluteFilePath());
      if (fin.open(QIODevice::ReadOnly | QIODevice::Text))
	{
	  QTextStream in(&fin);
	  if (!in.atEnd())
	    {
	      QString line = in.readLine();
	      QStringList words = line.split("::");
	      memGb = words[1].toInt();

	      if (!in.atEnd())
		{
		  line = in.readLine();
		  words = line.split("::");
		  spareMb = words[1].toInt();
		}
	      
	      ok = true;
	    }
	}
    }

  return ok;
}

bool
checkSettings(int memGb,
	     int spareMb)
{
  bool ok = true;

  QStringList items;
  items << "Do not change memory settings";
  items << "Change memory settings";
  QString item = QInputDialog::getItem(0,
				       "Memory settings",
				       QString("Main memory : %1 GB\nKeep spare : %2 Mb").\
				       arg(memGb).arg(spareMb),
				       items,
				       0,
				       false,
				       &ok);
  if (ok && !item.isEmpty())
    {
      if (item == "Change memory settings")
	ok = false;
    }
  else if (!ok)
    ok = true;

  return ok;
}

void
getSettings(int &memGb,
	    int &spareMb)
{
  int mem = QInputDialog::getInt(0, "Main Memory Size in GB", "size (GB)", 1, 1, 1000);
  memGb = mem;

  mem = QInputDialog::getInt(0, "Keep Spare Memory (in MB)", "size (MB)", 1, 1, 1000);
  spareMb = mem;
}


void
Raw2Pvl::batchProcess(VolumeData* volData,
		      QStringList timeseriesFiles)
{
  QString pvlFilename = getPvlNcFilename();
  if (pvlFilename.count() < 4)
    {
      QMessageBox::information(0, "pvl.nc", "No .pvl.nc filename chosen.");
      return;
    }

  bool save0AtTop = saveSliceZeroAtTop();;

  bool saveRawFile = getSaveRawFile();

  QString rawfile;
  if (saveRawFile) rawfile = getRawFilename(pvlFilename);
  if (rawfile.isEmpty())
    saveRawFile = false;

  int svslz = getZSubsampling(1024, 1024, 1024);
  int svsl = getXYSubsampling(svslz, 1024, 1024, 1024);
  int svsl3 = svslz*svsl*svsl;
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  savePvlDialog.setVoxelUnit(Raw2Pvl::_Micron);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  savePvlDialog.exec();

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);

  bool subsample = (svsl > 1 || svslz > 1);
  bool trim = false;


  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  
  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());

  //------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);
  int dmin = 0;
  int wmin = 0;
  int hmin = 0;
  int dmax = rvdepth-1;
  int wmax = rvwidth-1;
  int hmax = rvheight-1;
  int dsz=rvdepth;
  int wsz=rvwidth;
  int hsz=rvheight;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();
      
  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;
  
  //*** max 1Gb per slab
  int slabSize;
  slabSize = (1024*1024*1024)/(bpv*wsz*hsz);
  
  int dsz2 = dsz/svslz;
  int wsz2 = wsz/svsl;
  int hsz2 = hsz/svsl;

  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();
  
  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;
      
  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  int nbytes = rvwidth*rvheight*bpv;
  double *filtervol = new double[wsz2*hsz2];
  uchar *pvlslice = new uchar[pvlbpv*wsz2*hsz2];
  uchar *raw = new uchar[nbytes];
  uchar **val;
  if (spread > 0)
    {
      val = new uchar*[2*spread+1];
      for (int i=0; i<2*spread+1; i++)
	val[i] = new uchar[nbytes];
    }
  int rawSize = rawMap.size()-1;
  int width = wsz2;
  int height = hsz2;
  //------------------------------

  
  //------------------------------------------------------
  int tsfcount = qMax(1, timeseriesFiles.count());
  bool vol4d = tsfcount > 0;
  for (int tsf=0; tsf<tsfcount; tsf++)
    {

      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  break;
	}

      QString pvlflnm = pvlFilename;
      QString rawflnm = rawfile;

      if (tsfcount > 1)
	{
	  QFileInfo ftpvl(pvlFilename);
	  QFileInfo ftraw(timeseriesFiles[tsf]);
	  pvlflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".pvl.nc").absoluteFilePath();

	  rawflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".raw").absoluteFilePath();

	  volData->replaceFile(timeseriesFiles[tsf]);
	  //QStringList flnms;
	  //flnms << timeseriesFiles[tsf];
	  //volData->setFile(flnms, (tsf>0));
	  //volData->setFile(flnms, vol4d);
	}

      VolumeFileManager rawFileManager;
      VolumeFileManager pvlFileManager;

      pvlFileManager.setBaseFilename(pvlflnm);
      pvlFileManager.setDepth(dsz2);
      pvlFileManager.setWidth(wsz2);
      pvlFileManager.setHeight(hsz2);
      pvlFileManager.setVoxelType(pvlVoxelType);
      pvlFileManager.setHeaderSize(13);
      pvlFileManager.setSlabSize(slabSize);
      pvlFileManager.setSliceZeroAtTop(save0AtTop);
      pvlFileManager.createFile(true);
      
      if (saveRawFile)
	{
	  rawFileManager.setBaseFilename(rawflnm);
	  rawFileManager.setDepth(dsz2);
	  rawFileManager.setWidth(wsz2);
	  rawFileManager.setHeight(hsz2);
	  rawFileManager.setVoxelType(voxelType);
	  rawFileManager.setHeaderSize(13);
	  rawFileManager.setSlabSize(slabSize);
	  rawFileManager.setSliceZeroAtTop(save0AtTop);
	  rawFileManager.createFile(true);
	}
      //------------------------------------------------------


      savePvlHeader(pvlflnm,
		    saveRawFile, rawflnm,
		    voxelType, pvlVoxelType, voxelUnit,
		    dsz/svslz, wsz/svsl, hsz/svsl,
		    vx, vy, vz,
		    rawMap, pvlMap,
		    description,
		    slabSize);

      progress.setLabelText(pvlflnm);
      
      // ------------------
      // calculate weights for Gaussian filter
      float weights[100];
      float wsum = 0.0;
      for(int i=-spread; i<=spread; i++)
	{
	  float wgt = qExp(-i/(2.0*spread*spread))/(M_PI*2*spread*spread);
	  wsum +=  wgt;
	  weights[i+spread] = wgt;
	}
      weights[2*spread+2] = wsum;
      // ------------------
      
      for(int dd=0; dd<dsz2; dd++)
	{

	  if (progress.wasCanceled())
	    {
	      progress.setValue(100);  
	      QMessageBox::information(0, "Save", "-----Aborted-----");
	      break;
	    }

	  int d0 = dmin + dd*svslz; 
	  int d1 = d0 + svslz-1;
	  
	  progress.setValue((int)(100*(float)dd/(float)dsz2));
	  qApp->processEvents();
	  
	  memset(filtervol, 0, 8*wsz2*hsz2);
	  for (int d=d0; d<=d1; d++)
	    {
	      if (spread > 0)
		{
		  if (d == d0)
		    {
		      volData->getDepthSlice(d, val[spread]);
		      applyMeanFilterToSlice(val[spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);

		      for(int i=-spread; i<0; i++)
			{
			  if (d+i >= 0)
			    volData->getDepthSlice(d+i, val[spread+i]);
			  else
			    volData->getDepthSlice(0, val[spread+i]);

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter, weights);
			}
		      
		      for(int i=1; i<=spread; i++)
			{
			  if (d+i < rvdepth)
			    volData->getDepthSlice(d+i, val[spread+i]);
			  else
			    volData->getDepthSlice(rvdepth-1, val[spread+i]);

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter, weights);
			}
		    }
		  else if (d < rvdepth-spread)
		    {
		      volData->getDepthSlice(d+spread, val[2*spread]);
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);
		    }
		  else
		    {
		      volData->getDepthSlice(rvdepth-1, val[2*spread]);
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);
		    }
		}
	      else
		volData->getDepthSlice(d, raw);
	      
	      if (spread > 0)
		{
		  applyMeanFilter(val, raw,
				  voxelType, rvwidth, rvheight,
				  spread, dilateFilter, weights);
		  
		  // now shift the planes
		  uchar *tmp = val[0];
		  for(int i=0; i<2*spread; i++)
		    val[i] = val[i+1];
		  val[2*spread] = tmp;
		}
	      
	      if (trim || subsample)
		{
		  int fi = 0;
		  for(int j=0; j<wsz2; j++)
		    {
		      int y0 = wmin+j*svsl;
		      int y1 = y0+svsl-1;
		      for(int i=0; i<hsz2; i++)
			{
			  int x0 = hmin+i*svsl;
			  int x1 = x0+svsl-1;
			  for(int y=y0; y<=y1; y++)
			    for(int x=x0; x<=x1; x++)
			      {
				if (voxelType == _UChar)
				  { uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Char)
				  { char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _UShort)
				  { ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Short)
				  { short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Int)
				  { int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Float)
				  { float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      }
			  fi++;
			}
		    }
		} // trim || subsample
	    }
	  
	  if (trim || subsample)
	    {
	      if (subsample)
		{
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    filtervol[fi] /= svsl3;
		}
	      
	      if (voxelType == _UChar)
		{
		  uchar *ptr = raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Char)
		{
		  char *ptr = (char*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Short)
		{
		  short *ptr = (short*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Int)
		{
		  int *ptr = (int*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Float)
		{
		  float *ptr = (float*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    } // trim || subsample
	  
	  if (saveRawFile)
	    rawFileManager.setSlice(dd, raw);
	  
	  applyMapping(raw, voxelType, rawMap,
		       pvlslice, pvlbpv, pvlMap,
		       width, height);
	  
	  pvlFileManager.setSlice(dd, pvlslice);
	} // end of dd loop

      progress.setLabelText(QString("Processed %1 of %2").arg(tsf).arg(tsfcount));
    }

  delete [] filtervol;
  delete [] pvlslice;
  delete [] raw;
  if (spread > 0)
    {
      for (int i=0; i<2*spread+1; i++)
	delete [] val[i];
      delete [] val;
    }

  progress.setValue(100);
  
  QMessageBox::information(0, "Batch Processing", "-----Done-----");
}

void
Raw2Pvl::saveMHD(QString mhdFilename,
		 VolumeData* volData,
		 int dmin, int dmax,
		 int wmin, int wmax,
		 int hmin, int hmax)
{
  bool saveByteData = false;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - (default)";  
  slevels << "No - save byte-mapped data";
  QString option = QInputDialog::getItem(0,
		   "Save Original Data",
		   "Save Original Data in MetaImage Format  ?",
		    slevels,
			  0,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "No")
	saveByteData = true;
    }
  

  //------------------------------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  int svslz = getZSubsampling(dsz, wsz, hsz);
  int svsl = getXYSubsampling(svslz, dsz, wsz, hsz);

  int dsz2 = dsz/svslz;
  int wsz2 = wsz/svsl;
  int hsz2 = hsz/svsl;
  int svsl3 = svslz*svsl*svsl;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  savePvlDialog.setVoxelUnit(Raw2Pvl::_Micron);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  savePvlDialog.exec();

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);
  //------------------------------------------------------


  QString zrawFilename = mhdFilename;
  zrawFilename.chop(3);
  zrawFilename += "raw";
  
  if (QFile::exists(zrawFilename))
    {
      QString zfl = QFileDialog::getSaveFileName(0,
						 "Save raw volume",
						 Global::previousDirectory(),
						 "File (*.raw)");
//						 0,
//						 QFileDialog::DontUseNativeDialog);

      if (zfl.isEmpty())
	{
	  QStringList items;
	  items << "No";
	  items << "Yes";
	  QString item = QInputDialog::getItem(0,
					       "Overwrite existing file ?",
					       QString("Overwrite %1 ").arg(zrawFilename),
					       items,
					       0,
					       false,
					       &ok);
	  if (item == "No" || !ok)
	    return;
	}
      else
	zrawFilename = zfl;
      
      if (!zrawFilename.endsWith(".raw"))
	zrawFilename += ".raw";
    }

  
  {
    QFile mhd;
    mhd.setFileName(mhdFilename);
    mhd.open(QFile::WriteOnly | QFile::Text);
    QTextStream out(&mhd);
    out << "ObjectType = Image\n";
    out << "NDims = 3\n";
    out << "BinaryData = True\n";
    out << "BinaryDataByteOrderMSB = False\n";
    out << "CompressedData = False\n";
    out << "TransformMatrix = 1 0 0 0 1 0 0 0 1\n";
    out << "Offset = 0 0 0\n";
    out << "CenterOfRotation = 0 0 0\n";
    out << QString("ElementSpacing = %1 %2 %3\n").arg(vz).arg(vy).arg(vx);
    out << QString("DimSize = %1 %2 %3\n").arg(hsz2).arg(wsz2).arg(dsz2);
    out << "HeaderSize = 0\n";
    out << "AnatomicalOrientation = ???\n";
    if (saveByteData)
      out << "ElementType = MET_UCHAR\n";
    else
      {
	if (voxelType == _UChar)      out << "ElementType = MET_UCHAR\n";
	else if (voxelType == _Char)  out << "ElementType = MET_CHAR\n";
	else if (voxelType == _UShort)out << "ElementType = MET_USHORT\n";
	else if (voxelType == _Short) out << "ElementType = MET_SHORT\n";
	else if (voxelType == _Int)   out << "ElementType = MET_INT\n";
	else if (voxelType == _Float) out << "ElementType = MET_FLOAT\n";
      }
    QString rflnm = QFileInfo(zrawFilename).fileName();
    out << QString("ElementDataFile = %1\n").arg(rflnm);
  }

  {
    QFile zraw;
    zraw.setFileName(zrawFilename);
    zraw.open(QFile::WriteOnly);

    int nbytes = rvwidth*rvheight*bpv;
    double *filtervol = new double[wsz2*hsz2];
    uchar *pvl = new uchar[wsz2*hsz2];
    uchar *raw = new uchar[nbytes];
    uchar **val;
    if (spread > 0)
      {
	val = new uchar*[2*spread+1];
	for (int i=0; i<2*spread+1; i++)
	  val[i] = new uchar[nbytes];
      }
    int width = wsz2;
    int height = hsz2;
    bool subsample = (svsl > 1 || svslz > 1);
    bool trim = (dmin != 0 ||
		 wmin != 0 ||
		 hmin != 0 ||
		 dsz2 != rvdepth ||
		 wsz2 != rvwidth ||
		 hsz2 != rvheight);

    QList<float> rawMap = volData->rawMap();
    QList<int> pvlMap = volData->pvlMap();
    int rawSize = rawMap.size()-1;

    
    QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
    QWidget *mainWidget = 0;
    for(QWidget *w : topLevelWidgets)
      {
	if (w->isWindow())
	  {
	    mainWidget = w;
	    break;
	  }
      }
  

    QProgressDialog progress("Saving MetaImage volume",
			     "Cancel",
			     0, 100,
			     mainWidget,
			     Qt::Dialog|Qt::WindowStaysOnTopHint);
    progress.setMinimumDuration(0);
    progress.resize(500, 100);
    progress.move(QCursor::pos());
    
    // ------------------
    // calculate weights for Gaussian filter
    float weights[100];
    float wsum = 0.0;
    for(int i=-spread; i<=spread; i++)
      {
	float wgt = qExp(-i/(2.0*spread*spread))/(M_PI*2*spread*spread);
	wsum +=  wgt;
	weights[i+spread] = wgt;
      }
    weights[2*spread+2] = wsum;
    // ------------------
      
    for(int dd=0; dd<dsz2; dd++)
      {

	if (progress.wasCanceled())
	  {
	    progress.setValue(100);  
	    QMessageBox::information(0, "Save", "-----Aborted-----");
	    break;
	  }

	int d0 = dmin + dd*svslz; 
	int d1 = d0 + svslz-1;
	  
	progress.setValue((int)(100*(float)dd/(float)dsz2));
	qApp->processEvents();
	  
	memset(filtervol, 0, 8*wsz2*hsz2);
	for (int d=d0; d<=d1; d++)
	  {
	    if (spread > 0)
	      {
		if (d == d0)
		  {
		    volData->getDepthSlice(d, val[spread]);
		    applyMeanFilterToSlice(val[spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter, weights);
		    
		    for(int i=-spread; i<0; i++)
		      {
			if (d+i >= 0)
			  volData->getDepthSlice(d+i, val[spread+i]);
			else
			  volData->getDepthSlice(0, val[spread+i]);
			
			applyMeanFilterToSlice(val[spread+i], raw,
					       voxelType, rvwidth, rvheight,
					       spread, dilateFilter, weights);
		      }
		    
		    for(int i=1; i<=spread; i++)
		      {
			if (d+i < rvdepth)
			  volData->getDepthSlice(d+i, val[spread+i]);
			else
			  volData->getDepthSlice(rvdepth-1, val[spread+i]);
			
			applyMeanFilterToSlice(val[spread+i], raw,
					       voxelType, rvwidth, rvheight,
					       spread, dilateFilter, weights);
		      }
		  }
		else if (d < rvdepth-spread)
		  {
		    volData->getDepthSlice(d+spread, val[2*spread]);
		    applyMeanFilterToSlice(val[2*spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter, weights);
		  }		  
		else
		  {
		    volData->getDepthSlice(rvdepth-1, val[2*spread]);
		    applyMeanFilterToSlice(val[2*spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter, weights);
		  }		  
	      }
	    else
	      volData->getDepthSlice(d, raw);
	    
	    if (spread > 0)
	      {
		applyMeanFilter(val, raw,
				voxelType, rvwidth, rvheight,
				spread, dilateFilter, weights);
		
		// now shift the planes
		uchar *tmp = val[0];
		for(int i=0; i<2*spread; i++)
		  val[i] = val[i+1];
		val[2*spread] = tmp;
	      }
	    
	    if (trim || subsample)
	      {
		int fi = 0;
		for(int j=0; j<wsz2; j++)
		  {
		    int y0 = wmin+j*svsl;
		    int y1 = y0+svsl-1;
		    for(int i=0; i<hsz2; i++)
		      {
			int x0 = hmin+i*svsl;
			int x1 = x0+svsl-1;
			for(int y=y0; y<=y1; y++)
			  for(int x=x0; x<=x1; x++)
			    {
			      if (voxelType == _UChar)
				{ uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Char)
				{ char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _UShort)
				{ ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Short)
				{ short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Int)
				{ int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Float)
				{ float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			    }
			fi++;
		      }
		  }
	      } // trim || subsample
	  }
	
	if (trim || subsample)
	  {
	    if (subsample)
	      {
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  filtervol[fi] /= svsl3;
	      }
	    
	    if (voxelType == _UChar)
	      {
		uchar *ptr = raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Char)
	      {
		char *ptr = (char*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    else if (voxelType == _Short)
	      {
		short *ptr = (short*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Int)
	      {
		int *ptr = (int*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Float)
	      {
		float *ptr = (float*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	  } // trim || subsample
	
	if (!saveByteData) // save original volume
	  zraw.write((char*)raw, wsz2*hsz2*bpv);
	else
	  {
	    if (voxelType == _UChar)
	      {
		uchar *ptr = raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _Char)
	      {
		char *ptr = (char*)raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _UShort)
	      {
		ushort *ptr = (ushort*)raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _Short)
	      {
		short *ptr = (short*)raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _Int)
	      {
		int *ptr = (int*)raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _Float)
	      {
		float *ptr = (float*)raw;
		REMAPVOLUME();
	      }
	  
	    zraw.write((char*)pvl, wsz2*hsz2);
	  }
      }
    progress.setValue(100);

    delete [] filtervol;
    delete [] raw;
    delete [] pvl;
    if (spread > 0)
      {
	for (int i=0; i<2*spread+1; i++)
	  delete [] val[i];
	delete [] val;
      }    
  }
  
  QMessageBox::information(0, "Save MetaImage Volume", "-----Done-----");
}
//================================
//================================
void
Raw2Pvl::mergeVolumes(VolumeData* volData,
		      int dmin, int dmax,
		      int wmin, int wmax,
		      int hmin, int hmax,
		      QStringList timeseriesFiles)
{
  //------------------------------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int slabSize = (1024*1024*1024)/(bpv*wsz*hsz);
  slabSize = dsz+1;
  //------------------------------------------------------

  QString pvlFilename = getPvlNcFilename();
  if (pvlFilename.endsWith(".mhd"))
    {
      saveMHD(pvlFilename,
	      volData,
	      dmin, dmax,
	      wmin, wmax,
	      hmin, hmax);
      return;
    }

  if (pvlFilename.count() < 4)
    {
      QMessageBox::information(0, "pvl.nc", "No .pvl.nc filename chosen.");
      return;
    }

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  int vu = volData->voxelUnit();
  savePvlDialog.setVoxelUnit(vu);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  savePvlDialog.exec();

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  bool invertData = savePvlDialog.invertData();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);


  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();

  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;

  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  
  int nbytes = rvwidth*rvheight*bpv;
  uchar *pvlslice = new uchar[pvlbpv*wsz*hsz];
  uchar *Mpvlslice = new uchar[pvlbpv*wsz*hsz];
  uchar *raw = new uchar[nbytes];
  int rawSize = rawMap.size()-1;
  int width = wsz;
  int height = hsz;

  VolumeFileManager rawFileManager;
  VolumeFileManager pvlFileManager;



  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  
  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());


  pvlFileManager.setBaseFilename(pvlFilename);
  pvlFileManager.setDepth(dsz);
  pvlFileManager.setWidth(wsz);
  pvlFileManager.setHeight(hsz);
  pvlFileManager.setVoxelType(pvlVoxelType);
  pvlFileManager.setHeaderSize(13);
  pvlFileManager.setSlabSize(slabSize);
  pvlFileManager.setSliceZeroAtTop(false);
  pvlFileManager.createFile(true); 


  savePvlHeader(pvlFilename,
		false, "",
		voxelType, pvlVoxelType, voxelUnit,
		dsz, wsz, hsz,
		vx, vy, vz,
		rawMap, pvlMap,
		description,
		slabSize);


  //------------------------------------------------------
  int tsfcount = qMax(1, timeseriesFiles.count());

  int tagF = 255/tsfcount;
  if (pvlbpv == 2)
    tagF = 65535/tsfcount;

  QList<int> tagValues;
  for(int i=0; i<timeseriesFiles.count(); i++)
    {
      tagValues << (i+1)*tagF;
    }
  
  //-----------------------
  // decide tag values
  QTableWidget *tw = new QTableWidget();
  tw->setRowCount(timeseriesFiles.count());
  tw->setColumnCount(2);
  QStringList item;
  item.clear();
  item << "Mask";
  item << "Value";
  tw->setHorizontalHeaderLabels(item);

  for (int i=0; i<timeseriesFiles.count(); i++)
    {
      QFileInfo fi(timeseriesFiles[i]);
      QTableWidgetItem *n0 = new QTableWidgetItem(fi.baseName());
      n0->setFlags(n0->flags() ^ Qt::ItemIsEditable);
      tw->setItem(i, 0, n0);

      QTableWidgetItem *n1 = new QTableWidgetItem(QString("%1").arg(tagValues[i]));
      tw->setItem(i, 1, n1);
    }

  QPushButton *ok = new QPushButton("OK");
  QDialog *dg = new QDialog();
  dg->setModal(true);
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(ok);
  layout->addWidget(tw);
  dg->setLayout(layout);
  QObject::connect(ok, SIGNAL(clicked()),
		   dg, SLOT(accept()));
  dg->exec();
  
  for (int i=0; i<timeseriesFiles.count(); i++)
    {
      tagValues[i] = tw->item(i, 1)->text().toInt();

    }
  delete tw;
  //-----------------------
  
  for(int dd=0; dd<dsz; dd++)
    {

      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  break;
	}

      progress.setValue((int)(100*(float)dd/(float)dsz));
      qApp->processEvents();
      
      memset(Mpvlslice, 0, pvlbpv*wsz*hsz);
      
      for (int tsf=0; tsf<tsfcount; tsf++)
	{
	  volData->replaceFile(timeseriesFiles[tsf]);
	  
	  memset(raw, 0, nbytes);
	  volData->getDepthSlice(dd, raw);
	  
	  applyMapping(raw, voxelType, rawMap,
		       pvlslice, pvlbpv, pvlMap,
		       width, height);

	  
	  //-----------------
	  //merge data
	  if (pvlbpv == 1)
	    {
	      for(int fi=0; fi<wsz*hsz; fi++)
		{
		  if (pvlslice[fi] > 10)
		    {
		      Mpvlslice[fi] = tagValues[tsf];
		    }
		}
	    }
	  else
	    {
	      ushort *Mptr = (ushort*)Mpvlslice;
	      ushort *ptr = (ushort*)pvlslice;
	      for(int fi=0; fi<wsz*hsz; fi++)
		{
		  if (ptr[fi] > 10)
		    {
		      Mptr[fi] = tagValues[tsf];
		    }
		}
	    }
	  //-----------------
	}
      pvlFileManager.setSlice(dd, Mpvlslice);
    }


  delete [] Mpvlslice;
  delete [] pvlslice;
  delete [] raw;
  
  progress.setValue(100);
  
  QMessageBox::information(0, "Save", "-----Done-----");
}

//================================
//================================
void
Raw2Pvl::quickRaw(VolumeData* volData,
		  QStringList fileNames)
{
  //------------------------------------------------------  
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  int dsz=rvdepth;
  int wsz=rvwidth;
  int hsz=rvheight;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int slabSize = (1024*1024*1024)/(bpv*wsz*hsz);
  slabSize = dsz+1;
  //------------------------------------------------------

  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();

  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;

  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  
  int nbytes = rvwidth*rvheight*bpv;
  uchar *pvlslice = new uchar[pvlbpv*wsz*hsz];
  uchar *raw = new uchar[nbytes];
  int rawSize = rawMap.size()-1;
  int width = wsz;
  int height = hsz;

  VolumeFileManager rawFileManager;
  QFileInfo fraw(fileNames[0]);
  QString rawflnm = QFileInfo(fraw.absolutePath(),
			      fraw.baseName() + ".raw").absoluteFilePath();
  
  QFile m_qfile;
  m_qfile.setFileName(rawflnm);
  m_qfile.open(QFile::WriteOnly);
  m_qfile.write((char*)&pvlVoxelType, 1);
  m_qfile.write((char*)&dsz, 4);
  m_qfile.write((char*)&wsz, 4);
  m_qfile.write((char*)&hsz, 4);


  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  
  QProgressDialog progress("Saving "+rawflnm,
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());


  //------------------------------------------------------

  qint64 sliceSize = pvlbpv*wsz*hsz;
  for(qint64 dd=0; dd<dsz; dd++)
    {
      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  break;
	}
      
      progress.setValue((int)(100*(float)dd/(float)dsz));
      qApp->processEvents();
      
      memset(raw, 0, nbytes);
      volData->getDepthSlice(dd, raw);
	  
      applyMapping(raw, voxelType, rawMap,
		   pvlslice, pvlbpv, pvlMap,
		   width, height);

	  
      m_qfile.seek(13 + (dsz-1-dd)*sliceSize);
      m_qfile.write((char*)pvlslice);
    }


  m_qfile.close();
		    
  delete [] pvlslice;
  delete [] raw;
  
  progress.setValue(100);
  
  //QMessageBox::information(0, "Save", "-----Done-----");
}



void
Raw2Pvl::getBackgroundValues(int &bType, float &bValue1, float &bValue2)
{
  bType = -2;  
  bValue1 = 0;
  bValue2 = 0;

  bool ok;
  QString mtext;
  mtext += "Background Value\n";
  mtext += " <Val - all voxels below Val will be treated as background\n";
  mtext += "        example : <100 - treat all voxels below 100 as background voxels\n";
  mtext += "                  that means only consider voxels above 100\n\n";
  mtext += " =Val - all voxels not equal to Val will be treated as background\n";
  mtext += "        example : =100 - treat all voxels equal to 100 as background voxels\n";
  mtext += "                  that means only consider voxels not equal to 100\n\n";
  mtext += " >Val - all voxels above Val will be treated as background\n";
  mtext += "        example : >100 - treat all voxels above 100 as background voxels\n";
  mtext += "                  that means only consider voxels below 100\n\n";
  mtext += " >Val1 <Val2 - all voxels between Val1 and Val2 will be treated as background\n";
  mtext += "        example : >100 <200 - treat all voxels above 100 and below 200 as background voxels\n";
  mtext += "                  that means only consider voxels outside of 100 and 200\n\n";
  mtext += " <Val1 >Val2 - all voxels below Val1 or above Val2 will be treated as background\n";
  mtext += "        example : <100 >200 - treat all voxels below 100 or above 200 as background voxels\n";
  mtext += "                  that means only consider voxels between and including 100 and 200\n\n";
  QString text = QInputDialog::getText(0,
				       "Background Value",
				       mtext,
				       QLineEdit::Normal,
				       "=0",
				       &ok);  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() == 2)
	{
	  if (list[0].left(1) == ">" && list[1].left(1) == "<")
	    {
	      bType = 2;
	      bValue1 = list[0].mid(1).toFloat();
	      bValue2 = list[1].mid(1).toFloat();
	    }
	  else if (list[0].left(1) == "<" && list[1].left(1) == ">")
	    {
	      bType = 3;
	      bValue1 = list[0].mid(1).toFloat();
	      bValue2 = list[1].mid(1).toFloat();
	    }
	}
      else if (list.count() == 1)
	{
	  if (list[0].left(1) == "<")
	    {
	      bType = -1;
	      bValue1 = list[0].mid(1).toInt();
	    }
	  if (list[0].left(2) == "!=")
	    {
	      bType = 0;
	      bValue1 = list[0].mid(2).toInt();
	    }
	  if (list[0].left(1) == ">")
	    {
	      bType = 1;
	      bValue1 = list[0].mid(1).toInt();
	    }
	}
    }
  if (bType == -2)
    {
      QMessageBox::information(0, "Background Value", QString("<Val,   !=Val,   >Val,   >Val1 <Val2,   <Val1 >Val2  expected.\nGot %1").arg(text));
      return;
    }
}

// Not storing uchar or ushort because Houdini/Omniverse cannot handle it without modifications
//using MyTree = openvdb::tree::Tree4<half, 5, 4, 3>::Type;
//using MyGrid = Grid<MyTree>;
int Raw2Pvl::m_vdb_bType;
float Raw2Pvl::m_vdb_bValue1;
float Raw2Pvl::m_vdb_bValue2;
float Raw2Pvl::m_vdb_resample;
bool
Raw2Pvl::saveVDB(int volIdx,
		 QString vdbFileName,
		 VolumeData* volData)
{
  int bType = -2;  
  float bValue1 = 0;
  float bValue2 = 0;
  
  if (volIdx <= 0)
    {
      getBackgroundValues(bType, bValue1, bValue2);
      
      if (bType == -2)
	return false;

      Raw2Pvl::m_vdb_bType   = bType;
      Raw2Pvl::m_vdb_bValue1 = bValue1;
      Raw2Pvl::m_vdb_bValue2 = bValue2;
    }
  else
    {
      bType   = Raw2Pvl::m_vdb_bType;
      bValue1 = Raw2Pvl::m_vdb_bValue1;
      bValue2 = Raw2Pvl::m_vdb_bValue2;
    }
  
  
  int dsz, wsz, hsz;
  volData->gridSize(dsz, wsz, hsz);
  
  float resample;
  if (volIdx <= 0)
    {
      bool ok;
      resample = QInputDialog::getDouble(0, "Resampling",
					 "Resample\nValues greater than 1.0 means downsampling.\nValues less than 1.0 means upsampling.",
					 1, 0.1, 10, 2, &ok, Qt::WindowFlags(), 0.1);
      Raw2Pvl::m_vdb_resample = resample;
    }
  else
    resample = Raw2Pvl::m_vdb_resample;


  uchar voxelType = volData->voxelType();  
  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int nbytes = wsz*hsz*bpv;
  uchar *raw = new uchar[nbytes];


  
  VdbVolume vdb;

  unsigned short *rawUS = (unsigned short*)raw;
  
  
  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  
  QProgressDialog progress("Saving "+vdbFileName,
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());

  for(int d = 0; d<dsz; d++)
    {
      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  return false;
	}
      
      progress.setValue((int)(100*(float)d/(float)dsz));
      qApp->processEvents();
      
      volData->getDepthSlice(d, raw);

      if (bpv == 1)
	vdb.addSliceToVDB(raw,
			  d, wsz, hsz,
			  bType, bValue1, bValue2);
      else
	vdb.addSliceToVDB((unsigned short*)raw,
			  d, wsz, hsz,
			  bType, bValue1, bValue2);
    }

  if (qAbs(resample-1.0)>0.001)
    vdb.resample(resample);

  
  progress.setLabelText("Writing to disk - " + vdbFileName); 
  progress.setValue(50);
  qApp->processEvents();

  vdb.save(vdbFileName);
  
  progress.setValue(100);

  return true;
}





void
Raw2Pvl::saveIsosurface(VolumeData* volData,
			int dmin, int dmax,
			int wmin, int wmax,
			int hmin, int hmax,
			QStringList timeseriesFiles)
{
  bool ok;
  QString meshFilename = QFileDialog::getSaveFileName(0,
						     "Export Mesh to file",
						      Global::previousDirectory(),
						      "Surface Mesh (*.ply *.obj *.stl) ;; Tetrahedral Mesh (*.msh)");
  if (meshFilename.isEmpty())
    {
      QMessageBox::information(0, "Error", "No OBJ filename specified");
      return;
    }
  
  
  if (!StaticFunctions::checkExtension(meshFilename, ".ply") &&
      !StaticFunctions::checkExtension(meshFilename, ".obj") &&
      !StaticFunctions::checkExtension(meshFilename, ".stl") &&
      !StaticFunctions::checkExtension(meshFilename, ".msh"))
    meshFilename += ".ply";

  
  bool tetMesh = false;
  if (StaticFunctions::checkExtension(meshFilename, ".msh"))
    tetMesh = true;    

  bool save0AtTop = saveSliceZeroAtTop();;
  
  int ivType = -2;
  int bType = -2;
  float isoValue, bValue1, bValue2;
  float adaptivity;
  float resample;
  int dataSmooth;
  int meshSmooth;
  int morphoType;
  int morphoRadius;
  QColor meshColor;
  bool applyVoxelScaling;
  if (!getValues(ivType, isoValue,
		 bType, bValue1, bValue2,
		 adaptivity, resample,
		 morphoType, morphoRadius,
		 dataSmooth, meshSmooth,
		 meshColor, applyVoxelScaling,
		 tetMesh))
    return;
  // return if the parameters are not correct



  // identify voxelType and set how many bytes per voxel to read
  uchar voxelType = volData->voxelType();  
  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;



  if (bType == 10)
    {
      if (voxelType != _Float)
	Raw2Pvl::saveIsosurfaceRange(volData,
				     dmin, dmax,
				     wmin, wmax,
				     hmin, hmax,
				     timeseriesFiles,
				     meshFilename,
				     save0AtTop,
				     bValue1, bValue2,
				     adaptivity, resample,
				     dataSmooth, meshSmooth,
				     morphoType, morphoRadius,
				     meshColor,
				     applyVoxelScaling);
      else
	QMessageBox::information(0, "Error",
				 "Isosurfaces not generated.\nIsosurface over range of values only works for non FLOAT datatypes");
      
      return;
    }
  

  int rvdepth, rvwidth, rvheight;
  volData->gridSize(rvdepth, rvwidth, rvheight);

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  
  uchar *raw = new uchar[rvwidth*rvheight*bpv];
  float *val = new float[wsz*hsz];

  bool trim = (dmin != 0 ||
	       wmin != 0 ||
	       hmin != 0 ||
	       dsz != rvdepth ||
	       wsz != rvwidth ||
	       hsz != rvheight);


  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }

  QProgressDialog progress("Exporting Mesh",
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());

  int tsfcount = qMax(1, timeseriesFiles.count());
  QChar fillChar = '0';
  int fieldWidth = 2;
  fieldWidth = tsfcount/10+2;
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
      VdbVolume vdb;

      QString meshflnm;
      if (tsfcount == 1)
	meshflnm = meshFilename;
      else
	meshflnm = meshFilename.chopped(4) +
	           QString("_%1").arg((int)tsf, fieldWidth, 10, fillChar) +
	           meshFilename.right(4);
      
      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  break;
	}
      
      if (tsfcount > 1)
	{
	  volData->replaceFile(timeseriesFiles[tsf]);
	}

      for(int d=dmin; d<=dmax; d++)
	{
	  
	  if (progress.wasCanceled())
	    {
	      progress.setValue(100);  
	      QMessageBox::information(0, "Save", "-----Aborted-----");
	      break;
	    }
	  	  
	  progress.setValue((int)(100*(float)d/(float)dsz));
	  qApp->processEvents();
	  
	  volData->getDepthSlice(d, raw);
	  int vi = 0;
	  for(int w=wmin; w<=wmax; w++)
	    {
	      for(int h=hmin; h<=hmax; h++)
		{
		  if (voxelType == _UChar)
		    {
		      uchar *ptr = raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _Char)
		    {
		      char *ptr = (char*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _UShort)
		    {
		      ushort *ptr = (ushort*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _Short)
		    {
		      short *ptr = (short*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _Int)
		    {
		      int *ptr = (int*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _Float)
		    {
		      float *ptr = (float*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  vi++;
		} // loop i
	    } // loop j

	  if (save0AtTop)
	    vdb.addSliceToVDB(val,
			      rvdepth-1-d, wsz, hsz,
			      bType, bValue1, bValue2);
	  else
	    vdb.addSliceToVDB(val,
			      d, wsz, hsz,
			      bType, bValue1, bValue2);
	} // loop dd - slices

      
      // resample is required
      if (qAbs(resample-1.0) > 0.001)
	{
	  progress.setLabelText("Downsampling Voxel Volume");
	  progress.setValue(80);  
	  qApp->processEvents();
	  vdb.resample(resample);
	}

      
      // convert to level set
      progress.setLabelText("Converting to levelset");
      progress.setValue(50);  
      qApp->processEvents();
      vdb.convertToLevelSet(isoValue, ivType);
      
      
      // Apply Morphological Operations
      if (morphoType > 0 && morphoRadius > 0)
	{
	  float offset = morphoRadius;
	  if (morphoType == 1)
	    {
	      progress.setLabelText("Applying Morphological Dilation");
	      vdb.offset(-offset); // dilate
	    }
	  if (morphoType == 2)
	    {
	      progress.setLabelText("Applying Morphological Erosion");
	      vdb.offset(offset); // erode
	    }
	  if (morphoType == 3)
	    {
	      progress.setLabelText("Applying Morphological Closing");
	      vdb.offset(-offset); // dilate
	      vdb.offset(offset); // erode
	    }
	  if (morphoType == 4)
	    {
	      progress.setLabelText("Applying Morphological Opening");
	      vdb.offset(offset); // erode
	      vdb.offset(-offset); // dilate
	    }

	  progress.setValue(60);  
	  qApp->processEvents();
	  
	}
            
      
      // smoothing if required  
      if (dataSmooth > 0)
	{
	  progress.setLabelText("Smoothing Voxel Volume");
	  progress.setValue(70);  
	  qApp->processEvents();
	  vdb.mean(0.1, dataSmooth); // width, iterations
	}


      
      Global::statusBar()->showMessage("Generating Mesh");
      qApp->processEvents();
      
      progress.setLabelText("Generating Isosurface Mesh");
      progress.setValue(90);  
      QVector<QVector3D> V;
      QVector<QVector3D> VN;
      QVector<int> T;
      if (!tetMesh)
	vdb.generateMesh(0, 0, adaptivity, V, VN, T);
      else
	vdb.generateMesh(0, 0, 0, V, VN, T);

      progress.setLabelText("Saving Mesh to "+QFileInfo(meshflnm).fileName());
      Global::statusBar()->showMessage("Saving Mesh to "+QFileInfo(meshflnm).fileName());
      qApp->processEvents();
      
      if (applyVoxelScaling) // take voxel size into account
	{
	  float vx, vy, vz;
	  volData->voxelSize(vx, vy, vz);
	  for(int i=0; i<V.count(); i++)
	    V[i] *= QVector3D(vx, vy, vz);
	}      
      
      if (meshSmooth > 0)  
	MeshTools::smoothMesh(V, VN, T, 5*meshSmooth);

      if (tetMesh)
	{
	  MeshTools::saveToTetrahedralMesh(meshflnm, V, T);
	}
      else if (meshflnm.right(3).toLower() == "obj")
	{
	  QVector<QVector3D> C;
	  C.resize(V.count());
	  C.fill(QVector3D(meshColor.red(),
			   meshColor.green(),
			   meshColor.blue()));			   
	  MeshTools::saveToOBJ(meshflnm, V, VN, C, T);
	}
      else if (meshflnm.right(3).toLower() == "ply")
	{
	  QVector<QVector3D> C;
	  C.resize(V.count());
	  C.fill(QVector3D(meshColor.red(),
			   meshColor.green(),
			   meshColor.blue()));			   
	  MeshTools::saveToPLY(meshflnm, V, VN, C, T);
	}
      else if (meshflnm.right(3).toLower() == "stl")
	MeshTools::saveToSTL(meshflnm, V, VN, T);
      
      Global::statusBar()->clearMessage();
    } // loop timeseries

  progress.setValue(100);  
  QMessageBox::information(0, "Export Mesh", "Save Done");
}



void
Raw2Pvl::parIsoGen(VolumeData* volData,
		   uchar voxelType,
		   int rvheight, int rvwidth, int rvdepth,
		   int dmin, int dmax,
		   int wmin, int wmax,
		   int hmin, int hmax,
		   bool save0AtTop,
		   int iso,
		   QString meshflnm,
		   float adaptivity,
		   bool applyVoxelScaling,
		   int dataSmooth, int meshSmooth,
		   int morphoType, float morphoRadius,
		   float resample,
		   bool showProgress)
{
  QProgressDialog progress;
  if (!showProgress)
    progress.close();
  else
    {
      progress.setLabelText("Isosurface generation");
      progress.setRange(0, 100);
      progress.setWindowFlags(Qt::Dialog|Qt::WindowStaysOnTopHint);
      progress.setMinimumDuration(0);
    }



  VdbVolume vdb;

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;
  
  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  
  uchar *raw = new uchar[rvwidth*rvheight*bpv];
  float *val = new float[wsz*hsz];

  
  //------------------------------------
  for(int d=dmin; d<=dmax; d++)
    {      
      if (showProgress)
	{
	  progress.setValue((int)(100*(float)(d-dmin)/(float)(dmax+1-dmin)));
	  qApp->processEvents();
	}
	  
      volData->getDepthSlice(d, raw);
      int vi = 0;
      for(int w=wmin; w<=wmax; w++)
	{
	  for(int h=hmin; h<=hmax; h++)
	    {
	      if (voxelType == _UChar)
		{
		  uchar *ptr = raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _Char)
		{
		  char *ptr = (char*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _Short)
		{
		  short *ptr = (short*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _Int)
		{
		  int *ptr = (int*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _Float)
		{
		  float *ptr = (float*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      vi++;
	    } // loop h
	} // loop w
      
      if (save0AtTop)
	vdb.addSliceToVDB(val,
			  rvdepth-1-d, wsz, hsz,
			  4, iso, 0);
      else
	vdb.addSliceToVDB(val,
			  d, wsz, hsz,
			  4, iso, 0);
      
    } // loop d - slices
  //------------------------------------


  //------------------------------------
  // create mesh filename
  QString iso_meshflnm;
  QChar fillChar = '0';
  iso_meshflnm = meshflnm.chopped(4) +
    QString("_%1").arg((int)iso, 5, 10, fillChar) +
    meshflnm.right(4);
  //------------------------------------
  

  if (showProgress)
    {
      progress.setLabelText("Downsampling Voxel Volume");
      progress.setValue(50);
      qApp->processEvents();
    }
  
  
  //------------------------------------
  // resample is required
  if (qAbs(resample-1.0) > 0.001)
    vdb.resample(resample);
  //------------------------------------
 
 
  if (showProgress)
    {
      progress.setLabelText("Converting to levelset");
      progress.setValue(60);
      qApp->processEvents();
    }
  

  //------------------------------------
  // convert to level set
  vdb.convertToLevelSet(iso, 0);
  //------------------------------------
	  

  if (showProgress)
    {
      progress.setLabelText("Applying Morphological Operation");
      progress.setValue(70);
      qApp->processEvents();
    }
  

  //------------------------------------
  // Apply Morphological Operations
  if (morphoType > 0 && morphoRadius > 0)
    {
      float offset = morphoRadius;
      if (morphoType == 1) vdb.offset(-offset); // dilate
      else if (morphoType == 2) vdb.offset(offset); // erode
      else if (morphoType == 3)
	{
	  vdb.offset(-offset); // dilate
	  vdb.offset(offset); // erode
	}
      else if (morphoType == 4)
	{
	  vdb.offset(offset); // erode
	  vdb.offset(-offset); // dilate
	}
    }
  //------------------------------------


  if (showProgress)
    {
      progress.setLabelText("Smoothing Voxel Volume");
      progress.setValue(80);
      qApp->processEvents();
    }
  
  
  //------------------------------------
  // smoothing if required  
  if (dataSmooth > 0)
    vdb.mean(0.1, dataSmooth); // width, iterations
  //------------------------------------


  
  //------------------------------------
  // color, smooth and save mesh
  QVector<QVector3D> V;
  QVector<QVector3D> VN;
  QVector<int> T;
  vdb.generateMesh(0, 0, adaptivity, V, VN, T);

  // don't generate file if no vertices found
  if (V.count() == 0)
    return;
  

  // take voxel size into account
  if (applyVoxelScaling)
    {
      float vx, vy, vz;
      volData->voxelSize(vx, vy, vz);
      for(int i=0; i<V.count(); i++)
	V[i] *= QVector3D(vx, vy, vz);
    }      

  // smoothing
  if (meshSmooth > 0)  
    MeshTools::smoothMesh(V, VN, T, 5*meshSmooth);

  // default color
  QColor meshColor = QColor(Qt::white);

 
  //------------------------------------
  // save mesh

  if (showProgress)
    {
      progress.setLabelText("Saving Mesh to "+QFileInfo(iso_meshflnm).fileName());
      progress.setValue(90);
      qApp->processEvents();
    }
  
  if (iso_meshflnm.right(3).toLower() == "obj")
    {
      QVector<QVector3D> C;
      C.resize(V.count());
      C.fill(QVector3D(meshColor.red(),
		       meshColor.green(),
		       meshColor.blue()));			   
      MeshTools::saveToOBJ(iso_meshflnm, V, VN, C, T, false);
    }
  else if (iso_meshflnm.right(3).toLower() == "ply")
    {
      QVector<QVector3D> C;
      C.resize(V.count());
      C.fill(QVector3D(meshColor.red(),
		       meshColor.green(),
		       meshColor.blue()));			   
      MeshTools::saveToPLY(iso_meshflnm, V, VN, C, T, false);
    }
  else if (iso_meshflnm.right(3).toLower() == "stl")
    MeshTools::saveToSTL(iso_meshflnm, V, VN, T, false);
  //------------------------------------


  if (showProgress)
    {
      progress.setValue(100);
      qApp->processEvents();
    }
  
}

void
Raw2Pvl::mapIsoGen(QList<QVariant> plist)
{
  VolumeData* volData = static_cast<VolumeData*>(plist[0].value<void*>());
  uchar voxelType = plist[1].toInt();
  int rvheight = plist[2].toInt();
  int rvwidth = plist[3].toInt();
  int rvdepth = plist[4].toInt();
  int dmin = plist[5].toInt();
  int dmax = plist[6].toInt();
  int wmin = plist[7].toInt();
  int wmax = plist[8].toInt();
  int hmin = plist[9].toInt();
  int hmax = plist[10].toInt();
  bool save0AtTop = plist[11].toBool();
  int iso = plist[12].toInt();
  QString meshflnm = plist[13].toString();
  float adaptivity = plist[14].toFloat();
  bool applyVoxelScaling = plist[15].toBool();
  int dataSmooth = plist[16].toInt();
  int meshSmooth = plist[17].toInt();
  int morphoType = plist[18].toInt();
  float morphoRadius = plist[19].toFloat();
  float resample = plist[20].toFloat();
  
  Raw2Pvl::parIsoGen(volData,
		     voxelType,
		     rvheight, rvwidth, rvdepth,
		     dmin, dmax,
		     wmin, wmax,
		     hmin, hmax,
		     save0AtTop,
		     iso,
		     meshflnm,
		     adaptivity,
		     applyVoxelScaling,
		     dataSmooth, meshSmooth,
		     morphoType, morphoRadius,
		     resample,
		     false);			     
}

void
Raw2Pvl::saveIsosurfaceRange(VolumeData* volData,
			     int dmin, int dmax,
			     int wmin, int wmax,
			     int hmin, int hmax,
			     QStringList timeseriesFiles,
			     QString meshFilename,
			     bool save0AtTop,
			     float bValue1, float bValue2,
			     float adaptivity, float resample,
			     int dataSmooth, int meshSmooth,
			     int morphoType, int morphoRadius,
			     QColor meshColor,
			     bool applyVoxelScaling)
{
  //--------------------
  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  //--------------------

  
  int rvdepth, rvwidth, rvheight;
  volData->gridSize(rvdepth, rvwidth, rvheight);

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  
  uchar voxelType = volData->voxelType();  
  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  uchar *raw = new uchar[rvwidth*rvheight*bpv];
  float *val = new float[wsz*hsz];

  bool trim = (dmin != 0 ||
	       wmin != 0 ||
	       hmin != 0 ||
	       dsz != rvdepth ||
	       wsz != rvwidth ||
	       hsz != rvheight);


  int tsfcount = qMax(1, timeseriesFiles.count());
  QChar fillChar = '0';
  int fieldWidth = 2;
  fieldWidth = tsfcount/10+2;
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
      QString meshflnm;
      if (tsfcount == 1)
	meshflnm = meshFilename;
      else
	meshflnm = meshFilename.chopped(4) +
	           QString("_%1").arg((int)tsf, fieldWidth, 10, fillChar) +
	           meshFilename.right(4);
      
      if (tsfcount > 1)
	{
	  volData->replaceFile(timeseriesFiles[tsf]);
	}


      if (checkParIsoGen())
	{
	  //-----------------------------------------------
	  // parallel isosurface generation
	  //-----------------------------------------------

	  // create parameter list to be sent to the parallel iso surface generation routine
	  QList<QList<QVariant>> param;
	  for(int iso=(int)bValue1; iso<=(int)bValue2; iso++)
	    {
	      QList<QVariant> plist;
	      plist << QVariant::fromValue(static_cast<void*>(volData));
	      plist << QVariant((int)voxelType);
	      plist << QVariant(rvheight);
	      plist << QVariant(rvwidth);
	      plist << QVariant(rvdepth);	  
	      plist << QVariant(dmin);
	      plist << QVariant(dmax);
	      plist << QVariant(wmin);
	      plist << QVariant(wmax);
	      plist << QVariant(hmin);
	      plist << QVariant(hmax);
	      plist << QVariant(save0AtTop);
	      plist << QVariant(iso);
	      plist << QVariant(meshflnm);
	      plist << QVariant(adaptivity);
	      plist << QVariant(applyVoxelScaling);
	      plist << QVariant(dataSmooth);
	      plist << QVariant(meshSmooth);
	      plist << QVariant(morphoType);
	      plist << QVariant(morphoRadius);
	      plist << QVariant(resample);
	      
	      param << plist;
	    }
	  
	  
	  // Create a progress dialog.
	  QProgressDialog dialog;
	  dialog.setLabelText(QString("Exporting mesh using %1 thread(s)...").arg(QThread::idealThreadCount()));
	  
	  // Create a QFutureWatcher and connect signals and slots.
	  QFutureWatcher<void> futureWatcher;
	  QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
	  QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
	  QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
	  QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);
	  
	  // Start generation of isosurface for all values within the range
	  futureWatcher.setFuture(QtConcurrent::map(param, Raw2Pvl::mapIsoGen));
	  
	  // Display the dialog and start the event loop.
	  dialog.exec();
	  
	  futureWatcher.waitForFinished();
	  //-----------------------------------------------
	}
      else
	{
	  //-----------------------------------------------
	  // sequential isosurface generation	  
	  //-----------------------------------------------

	  QProgressDialog progress("Exporting Mesh",
				   "Cancel",
				   0, 100,
				   mainWidget,
				   Qt::Dialog|Qt::WindowStaysOnTopHint);
	  progress.setMinimumDuration(0);
	  progress.resize(500, 100);
	  progress.move(QCursor::pos());
	  for(int iso=(int)bValue1; iso<=(int)bValue2; iso++)
	    {	     	  	  
	      progress.setValue((int)(100*(float)(iso-bValue1)/(float)(bValue2+1-bValue1)));
	      qApp->processEvents();

	      Raw2Pvl::parIsoGen(volData,
				 voxelType,
				 rvheight, rvwidth, rvdepth,
				 dmin, dmax,
				 wmin, wmax,
				 hmin, hmax,
				 save0AtTop,
				 iso,
				 meshflnm,
				 adaptivity,
				 applyVoxelScaling,
				 dataSmooth, meshSmooth,
				 morphoType, morphoRadius,
				 resample,
				 true);
	      if (progress.wasCanceled())
		{
		  progress.setValue(100);  
		  QMessageBox::information(0, "Save", "-----Aborted-----");
		  return;
		} 
	    }
	}
      
    } // loop timeseries
      
  QMessageBox::information(mainWidget, "Export Mesh", "Save Done");
}



bool
Raw2Pvl::getValues(int& ivType, float& isoValue,
		   int& bType, float& bValue1, float& bValue2,
		   float& adaptivity, float& resample,
		   int& morphoType, int& morphoRadius,
		   int& dataSmooth, int& meshSmooth,
		   QColor &color, bool& applyVoxelScaling,
		   bool tetMesh)
{
  isoValue = 0;
  adaptivity = 0.1;
  dataSmooth = 0;
  meshSmooth = 0;
  resample = 1.0;
  morphoType = 0;
  morphoRadius = 0;
  applyVoxelScaling = true;
  color = QColor(Qt::white);
  
  QString text("0");
  QString btext("<0");
  
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant("string");
  vlist << btext;
  plist["background value"] = vlist;
  
  vlist.clear();
  vlist << QVariant("string");
  vlist << text;
  plist["isosurface value"] = vlist;
  
  if (!tetMesh)
    {
      vlist.clear();
      vlist << QVariant("float");
      vlist << QVariant(adaptivity);
      vlist << QVariant(0.0);
      vlist << QVariant(1.0);
      vlist << QVariant(0.01); // singlestep
      vlist << QVariant(3); // decimals
      plist["adaptivity"] = vlist;
    }
  
  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(resample);
  vlist << QVariant(1.0);
  vlist << QVariant(10.0);
  vlist << QVariant(1); // singlestep
  vlist << QVariant(1); // decimals
  plist["downsample"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(dataSmooth);
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["smooth data"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(meshSmooth);
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["mesh smoothing"] = vlist;
  
  
  vlist.clear();
  vlist << QVariant("combobox");
  vlist << "0";
  vlist << "";
  vlist << "Dilate";
  vlist << "Erode";
  vlist << "Close";
  vlist << "Open";
  plist["morpho operator"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(morphoRadius);
  vlist << QVariant(0);
  vlist << QVariant(100);
  plist["morpho radius"] = vlist;

  if (!tetMesh)
    {
      vlist.clear();
      vlist << QVariant("color");
      vlist << color;
      plist["color"] = vlist;
    }

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(applyVoxelScaling);
  plist["apply voxel size"] = vlist;



  vlist.clear();
  QFile helpFile(":/mesh.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }	      
  plist["commandhelp"] = vlist;
  

  QStringList keys;
  keys << "background value";
  keys << "isosurface value";
  if (!tetMesh)
    keys << "adaptivity";
  keys << "downsample";
  keys << "smooth data";
  keys << "mesh smoothing";
  keys << "morpho operator";
  keys << "morpho radius";
  if (!tetMesh)
    keys << "color";
  keys << "apply voxel size";
  keys << "commandhelp";
  //keys << "message";

  
  propertyEditor.set("Mesh Generation Parameters", plist, keys);
  propertyEditor.resize(700, 400);

  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return false;
  
  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);

      if (pair.second)
	{
	  if (keys[ik] == "background value")
	    btext = pair.first.toString();
	  else if (keys[ik] == "isosurface value")
	    text = pair.first.toString();
	  else if (keys[ik] == "adaptivity")
	    adaptivity = pair.first.toFloat();
	  else if (keys[ik] == "downsample")
	    resample = pair.first.toFloat();
	  else if (keys[ik] == "smooth data")
	    dataSmooth = pair.first.toInt();
	  else if (keys[ik] == "mesh smoothing")
	    meshSmooth = pair.first.toInt();
	  else if (keys[ik] == "morpho operator")
	    morphoType = pair.first.toInt();
	  else if (keys[ik] == "morpho radius")
	    morphoRadius = pair.first.toInt();
	  else if (keys[ik] == "color")
	    color = pair.first.value<QColor>();
	  else if (keys[ik] == "apply voxel size")
	    applyVoxelScaling = pair.first.toBool();
	}
    }


  
  //=========================
  bType = -2;  
  bValue1 = 0;
  bValue2 = 0;

  if (!btext.isEmpty())
    {
      QStringList list = btext.split(" ", QString::SkipEmptyParts);
      if (list.count() == 2)
	{
	  if (list[0].left(1) == ">" && list[1].left(1) == "<")
	    {
	      bType = 2;
	      bValue1 = list[0].mid(1).toFloat();
	      bValue2 = list[1].mid(1).toFloat();
	    }
	  else if (list[0].left(1) == "<" && list[1].left(1) == ">")
	    {
	      bType = 3;
	      bValue1 = list[0].mid(1).toFloat();
	      bValue2 = list[1].mid(1).toFloat();
	    }
	}
      else if (list.count() == 1)
	{
	  if (list[0].left(1) == "<")
	    {
	      bType = -1;
	      bValue1 = list[0].mid(1).toInt();
	    }
	  if (list[0].left(2) == "!=")
	    {
	      bType = 4;
	      bValue1 = list[0].mid(2).toInt();
	      //QMessageBox::information(0, "", QString("%1").arg(bValue1));
	    }
	  if (list[0].left(1) == ">")
	    {
	      bType = 1;
	      bValue1 = list[0].mid(1).toInt();
	    }
	}
    }

  if (bType == -2)
    {
      QMessageBox::information(0, "Background Value", QString("<Val,   !=Val,   >Val,   >Val1 <Val2,   <Val1 >Val2  expected.\nGot %1").arg(btext));
      return false;
    }
  //=========================

  
  //=========================
  ivType = -2;
  if (!text.isEmpty())
    {
      QStringList list = text.split("-", QString::SkipEmptyParts);
      if (list.count() == 2)
	{
	  ivType = 2; // isvalue range
	  bType = 10;
	  isoValue = list[0].toFloat();
	  bValue1 = list[0].toFloat();
	  bValue2 = list[1].toFloat();
	  QMessageBox::information(0, "", QString("Isosurface Value Range : %1 to %2").arg(bValue1).arg(bValue2));
	}
      else
	{
	  QStringList list = text.split(" ", QString::SkipEmptyParts);
	  if (list.count() == 1)
	    {
	      if (list[0].left(1) == "<")
		{
		  ivType = 1;
		  isoValue = list[0].mid(1).toFloat();
		}
	      else
		{
		  ivType = -1;
		  isoValue = list[0].toFloat();	      
		}
	    }
	  else if (list.count() == 2)
	    {
	      if (list[0] == "<")
		{
		  ivType = 1;
		  isoValue = list[1].toFloat();
		}
	    }
	}
    }

  if (ivType == -2)
    {
      QMessageBox::information(0, "Isosurface Value", QString("isoValue or <isoValue expected.\nGot %1").arg(text));
      return false;
    }


  return true;
}
