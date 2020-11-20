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

#include <QTableWidget>
#include <QPushButton>

#include "savepvldialog.h"
#include "volumefilemanager.h"

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
		      "Drishti (*.pvl.nc) ;; MetaImage (*.mhd)");
  fdialog.setAcceptMode(QFileDialog::AcceptSave);

  if (!fdialog.exec() == QFileDialog::Accepted)
    return "";

  QString pvlFilename = fdialog.selectedFiles().value(0);
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
  
  QFile f(xmlfile.toLatin1().data());
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
			   0);
  progress.setMinimumDuration(0);

  //------------------------------------------------------
  int tsfcount = qMax(1, timeseriesFiles.count());
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
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
		    saveRawFile, rawflnm,
		    voxelType, pvlVoxelType, voxelUnit,
		    //dsz/svslz, wsz/svsl, hsz/svsl,
		    //dsz2, wsz2, hsz2,
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
  
  QMessageBox::information(0, "Save", "-----Done-----");
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
Raw2Pvl::saveIsosurface(VolumeData* volData,
			int dmin, int dmax,
			int wmin, int wmax,
			int hmin, int hmax,
			QStringList timeseriesFiles)
{
  QMessageBox::information(0, "Error", "This option is no longer available");
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

  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

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

    QProgressDialog progress("Saving MetaImage volume",
			     "Cancel",
			     0, 100,
			     0);
    progress.setMinimumDuration(0);
    
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


  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


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

  QProgressDialog progress("Saving "+rawflnm,
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  //------------------------------------------------------

  qint64 sliceSize = pvlbpv*wsz*hsz;
  for(qint64 dd=0; dd<dsz; dd++)
    {
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

