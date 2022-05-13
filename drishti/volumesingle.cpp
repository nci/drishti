#include "global.h"
#include "volumesingle.h"
#include "staticfunctions.h"
#include "rawvolume.h"
#include "enums.h"
#include "geometryobjects.h"
#include "prunehandler.h"
#include "mainwindowui.h"
#include "xmlheaderfunctions.h"

#include <QFileDialog>
#include <QInputDialog>

void VolumeSingle::closePvlFileManager() { m_pvlFileManager.closeQFile(); }
VolumeFileManager* VolumeSingle::pvlFileManager() { return &m_pvlFileManager; }
VolumeFileManager* VolumeSingle::gradFileManager() { return &m_gradFileManager; }
VolumeFileManager* VolumeSingle::lodFileManager() { return &m_lodFileManager; }

Vec VolumeSingle::getSubvolumeMin() { return m_dataMin; }
Vec VolumeSingle::getSubvolumeMax() { return m_dataMax; }

Vec VolumeSingle::getSubvolumeSize() { return m_subvolumeSize; }
Vec VolumeSingle::getSubvolumeTextureSize() { return m_subvolumeTextureSize; }
int VolumeSingle::getSubvolumeSubsamplingLevel() { return m_subvolumeSubsamplingLevel; }
uchar* VolumeSingle::getSubvolumeTexture() { return m_subvolumeTexture; }

Vec VolumeSingle::getDragSubvolumeTextureSize() { return m_dragSubvolumeTextureSize; }
int VolumeSingle::getDragSubvolumeSubsamplingLevel() { return m_dragSubvolumeSubsamplingLevel; }
uchar* VolumeSingle::getDragSubvolumeTexture() { return m_dragSubvolumeTexture; }

int* VolumeSingle::getSubvolume1dHistogram() { return m_subvolume1dHistogram; }
int* VolumeSingle::getSubvolume2dHistogram() { return m_subvolume2dHistogram; }

int* VolumeSingle::getDrag1dHistogram() { return m_drag1dHistogram; }
int* VolumeSingle::getDrag2dHistogram() { return m_drag2dHistogram; }


QList<QString> VolumeSingle::volumeFiles() { return m_volumeFiles; }

void VolumeSingle::setRepeatType(bool rt) { m_repeatType = rt; }

Vec
VolumeSingle::getFullVolumeSize()
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      Vec vsize = VolumeBase::getFullVolumeSize();
      return vsize;
    }

  Vec vsize = Vec(0,0,0);
  for(int i=0; i<m_volumeFiles.count(); i++)
    {
      int d, w, h;
      XmlHeaderFunctions::getDimensionsFromHeader(m_volumeFiles[i],
						  d, w, h);

      Vec vsz = Vec(h, w, d);
      vsize = StaticFunctions::maxVec(vsize, vsz);
    }
  return vsize;
}


VolumeSingle::VolumeSingle() :
  VolumeBase()
{
  m_repeatType = true;
  m_volnum = 0;
  m_dataMin = Vec(0,0,0);
  m_dataMax = Vec(0,0,0);
  
  m_volumeFiles.clear();

  m_dragSubvolumeTexture = 0;
  m_subvolumeTexture = 0;

  m_subvolume1dHistogram = new int[256];
  m_subvolume2dHistogram = new int[256*256];
  m_drag1dHistogram = new int[256];
  m_drag2dHistogram = new int[256*256];

  memset(m_subvolume1dHistogram, 0, 256*4);
  memset(m_subvolume2dHistogram, 0, 256*256*4);
  memset(m_drag1dHistogram, 0, 256*4);
  memset(m_drag2dHistogram, 0, 256*256*4);

  m_flhist1D = new float[256];
  m_flhist2D = new float[256*256];

  m_bitmask.clear();

  m_sliceTemp = 0;
  
  m_texColumns = 0;
  m_texRows = 0;
  m_texWidth = 0;
  m_texHeight = 0;

  m_nonZeroVoxels = 0;

  m_foffH = m_foffW = m_foffD = 0.5;
}

void
VolumeSingle::setOffsets(float od, float ow, float oh)
{
  m_foffD = od;
  m_foffW = ow;
  m_foffH = oh;
}

VolumeSingle::~VolumeSingle()
{
  if(m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;

  if(m_subvolumeTexture) delete [] m_subvolumeTexture;
  if(m_subvolume1dHistogram) delete [] m_subvolume1dHistogram;
  if(m_subvolume2dHistogram) delete [] m_subvolume2dHistogram;
  if(m_drag1dHistogram) delete [] m_drag1dHistogram;
  if(m_drag2dHistogram) delete [] m_drag2dHistogram;

  if(m_flhist1D) delete [] m_flhist1D;
  if(m_flhist2D) delete [] m_flhist2D;

  m_dragSubvolumeTexture = 0;
  m_subvolumeTexture = 0;
  m_subvolume1dHistogram = 0;
  m_subvolume2dHistogram = 0;
  m_drag1dHistogram = 0;
  m_drag2dHistogram = 0;

  m_flhist1D = 0;
  m_flhist2D = 0;

  m_volumeFiles.clear();

  m_bitmask.clear();

  if(m_sliceTemp) delete [] m_sliceTemp;
  m_sliceTemp = 0;
}

bool
VolumeSingle::loadVolume(QList<QString> vfiles, bool redo)
{
  Global::setLod(1);

  m_repeatType = true;
  m_volnum = 0;
  m_dataMin = Vec(0,0,0);
  m_dataMax = Vec(0,0,0);

  m_volumeFiles.clear();
  m_volumeFiles = vfiles;

  VolumeInformation vInfo = volInfo(0);
  VolumeInformation::setVolumeInformation(vInfo);
  
  bool ok = VolumeBase::loadVolume(m_volumeFiles[0].toLatin1().data(),
				   redo);

  setBasicInformation(m_volnum);

  return ok;
}

bool
VolumeSingle::loadDummyVolume(int nx, int ny, int nz)
{
  m_repeatType = true;
  m_volnum = 0;
  m_dataMin = Vec(0,0,0);
  m_dataMax = Vec(nx, ny, nz);

  m_volumeFiles.clear();

  VolumeInformation vInfo;
  VolumeInformation::setVolumeInformation(vInfo);

  VolumeBase::loadDummyVolume(nx, ny, nz);

  return true;
}

void
VolumeSingle::setBasicInformation(int volnum)
{
  //---------------------------------------------------------
  // --- set the information for pvl.nc file manager
  //---------------------------------------------------------
  int n_depth, n_width, n_height;
  int slabSize;
  XmlHeaderFunctions::getDimensionsFromHeader(m_volumeFiles[volnum],
					      n_depth, n_width, n_height);
  slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFiles[volnum]);

  int headerSize = XmlHeaderFunctions::getPvlHeadersizeFromHeader(m_volumeFiles[volnum]);
  QStringList pvlnames = XmlHeaderFunctions::getPvlNamesFromHeader(m_volumeFiles[volnum]);
  if (pvlnames.count() > 0)
    m_pvlFileManager.setFilenameList(pvlnames);
  m_pvlFileManager.setBaseFilename(m_volumeFiles[volnum]);
  m_pvlFileManager.setDepth(n_depth);
  m_pvlFileManager.setWidth(n_width);
  m_pvlFileManager.setHeight(n_height);
  m_pvlFileManager.setVoxelType(m_pvlVoxelType);
  m_pvlFileManager.setHeaderSize(headerSize); // default is 13 bytes
  m_pvlFileManager.setSlabSize(slabSize);

  RawVolume::setDepth(n_depth);
  RawVolume::setWidth(n_width);
  RawVolume::setHeight(n_height);
  RawVolume::setSlabSize(slabSize);

  QString vgfile = m_volumeFiles[volnum];
  vgfile.chop(6);
  vgfile += QString("grad");
  vgfile = StaticFunctions::replaceDirectory(Global::tempDir(),
					     vgfile);
  m_gradFileManager.setBaseFilename(vgfile);
  m_gradFileManager.setDepth(n_depth);
  m_gradFileManager.setWidth(n_width);
  m_gradFileManager.setHeight(n_height);
  m_gradFileManager.setVoxelType(m_pvlVoxelType);
  m_gradFileManager.setHeaderSize(13);
  m_gradFileManager.setSlabSize(slabSize);
  //---------------------------------------------------------
  //---------------------------------------------------------
}

int
VolumeSingle::timestepNumber(int n)
{
  int vn = n;

  if (m_repeatType) // cycle
    vn = n % m_volumeFiles.count();
  else // wave
    {
      vn = n % (2*m_volumeFiles.count()-1);
      if (vn >= m_volumeFiles.count())
	vn = (2*m_volumeFiles.count()-1) - vn;
    }

  return vn;
}

bool
VolumeSingle::setSubvolume(Vec boxMin, Vec boxMax,
			   int subsamplingLevel,
			   int volnum1,
			   bool force)
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      m_volnum = 0;
      m_dataMin = boxMin;
      m_dataMax = boxMax;
      m_subvolumeSize = m_dataMax - m_dataMin + Vec(1,1,1); 

      if (subsamplingLevel == 0)
	m_subvolumeSubsamplingLevel = 1;	  
      else
	m_subvolumeSubsamplingLevel = subsamplingLevel;

      return true;
    }

  int volnum = timestepNumber(volnum1);  

  boxMin = StaticFunctions::clampVec(Vec(0,0,0),
				     boxMin,
				     Vec(m_maxHeight-1, m_maxWidth-1, m_maxDepth-1));
  boxMax = StaticFunctions::clampVec(Vec(0,0,0),
				     boxMax,
				     Vec(m_maxHeight-1, m_maxWidth-1, m_maxDepth-1));

  if (force == false)
    {
      if (m_volnum == volnum &&
	  (m_dataMin-boxMin).squaredNorm() < 0.1 &&
	  (m_dataMax-boxMax).squaredNorm() < 0.1)
	// no change in dataMin and dataMax values
	  return false;
    }

  m_volnum = volnum;
  m_dataMin = boxMin;
  m_dataMax = boxMax;

  int cd, cw, ch;
  XmlHeaderFunctions::getDimensionsFromHeader(m_volumeFiles[m_volnum],
					      m_depth, m_width, m_height);
//  m_offH = (m_maxHeight- m_height)/2;
//  m_offW = (m_maxWidth - m_width)/2;
//  m_offD = (m_maxDepth - m_depth)/2;

  m_offH = (m_maxHeight-m_height)*m_foffH;
  m_offW = (m_maxWidth - m_width)*m_foffW;
  m_offD = (m_maxDepth - m_depth)*m_foffD;
    
  if (m_volumeFiles.count() > 1)
    setBasicInformation(m_volnum);

  m_subvolumeSize = m_dataMax - m_dataMin + Vec(1,1,1); 

  if (subsamplingLevel == 0)
    {
      int bpv = 1;
      if (m_pvlVoxelType > 0) bpv = 2;
      m_subvolumeSubsamplingLevel = StaticFunctions::getSubsamplingLevel(Global::textureMemorySize(),
									 Global::textureSizeLimit(),
									 bpv,
									 boxMin, boxMax);
    }
  else
    m_subvolumeSubsamplingLevel = subsamplingLevel;

  //-------------
  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenz = m_subvolumeSize.z;
  int lenx2 = lenx/m_subvolumeSubsamplingLevel;
  int leny2 = leny/m_subvolumeSubsamplingLevel;
  int lenz2 = lenz/m_subvolumeSubsamplingLevel;
  m_subvolumeTextureSize = Vec(lenx2, leny2, lenz2);
  //-------------


  if (m_subvolumeTexture) delete [] m_subvolumeTexture;
  m_subvolumeTexture = 0;

  if (m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;
  m_dragSubvolumeTexture = 0;
  //m_subvolumeTextureSize = Vec(1,1,1);

  return true;
}

int
VolumeSingle::actualVolumeNumber(int vnum1)
{
  if (Global::volumeType() == Global::DummyVolume)
    return 0;

  int vnum;
  if (m_repeatType) // cycle
    vnum = vnum1 % m_volumeFiles.count();
  else // wave
    {
      vnum = vnum1 % (2*m_volumeFiles.count()-1);
      if (vnum >= m_volumeFiles.count())
	vnum = (2*m_volumeFiles.count()-1) - vnum;
    }

  return vnum;
}

VolumeInformation
VolumeSingle::volInfo(int vnum1)
{
  VolumeInformation pvlInfo;

  int vnum = actualVolumeNumber(vnum1);

//  int vnum;
//  if (m_repeatType) // cycle
//    vnum = vnum1 % m_volumeFiles.count();
//  else // wave
//    {
//      vnum = vnum1 % (2*m_volumeFiles.count()-1);
//      if (vnum >= m_volumeFiles.count())
//	vnum = (2*m_volumeFiles.count()-1) - vnum;
//    }

  if (Global::volumeType() == Global::DummyVolume)
    {
      pvlInfo.voxelSize = Vec(1,1,1);
      pvlInfo.dimensions = Vec(m_depth, m_width, m_height);
      return pvlInfo; // dummy info
    }

  //if (vnum < 0 || vnum >= m_volumeFiles.count())
  if (vnum < 0)
    {
      QMessageBox::information(0, "Volume Information",
			       QString("%1 greater than %2 volumes").\
			       arg(vnum).\
			       arg(m_volumeFiles.count()));
      VolumeInformation::setVolumeInformation(pvlInfo);
      return pvlInfo;
    }

  if (VolumeInformation::volInfo(m_volumeFiles[vnum].toLatin1().data(),
			       pvlInfo) == false)
    {
      QMessageBox::information(0, "Volume Information",
			       QString("Invalid netCDF file %1").\
			       arg(m_volumeFiles[vnum]));
    }
  else
    {
      Global::setRelativeVoxelScaling(pvlInfo.relativeVoxelScaling);
      VolumeInformation::setVolumeInformation(pvlInfo);
    }

  return pvlInfo;
}

void
VolumeSingle::getSurfaceArea(uchar *lut,
			     QList<Vec> clipPos,
			     QList<Vec> clipNormal,
			     QList<CropObject> crops,
			     QList<PathObject> paths)
{
  int minx = m_dataMin.x;
  int maxx = m_dataMax.x;

  int miny = m_dataMin.y;
  int maxy = m_dataMax.y;

  int minz = m_dataMin.z;
  int maxz = m_dataMax.z;

  createBitmask(minx, maxx,
		miny, maxy,
		minz, maxz,
		lut,
		clipPos,
		clipNormal,
		crops,
		paths);
  
  m_nonZeroVoxels = 0;
  getSurfaceBitmask(minx, maxx,
		    miny, maxy,
		    minz, maxz);

  VolumeInformation pvlInfo = volInfo(m_volnum);

  Vec voxelSize = pvlInfo.voxelSize;
  float voxvol = m_nonZeroVoxels*voxelSize.x*voxelSize.y*voxelSize.z;

  QString str;
  str = QString("Non-Zero Surface Voxels : %1\n").arg(m_nonZeroVoxels);
  str += QString("Surface Area : %1 %2\n").\
         arg(voxvol).\
         arg(pvlInfo.voxelUnitStringShort());

  QMessageBox::information(0, "Surface Area Calculation", str);
}

void
VolumeSingle::getSurfaceBitmask(int minx, int maxx,
				int miny, int maxy,
				int minz, int maxz)
{
  int bmx = maxx-minx+1;
  int bmy = maxy-miny+1;

  m_nonZeroVoxels = 0;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Generating Surface Voxels"));
  Global::progressBar()->show();

  QBitArray bitcopy = m_bitmask;
  m_bitmask.fill(false);

  int bidx = 0;
  for(int k=minz; k<=maxz; k++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(k-minz)/(float)(maxz-minz+1)));

      for(int y=miny; y<=maxy; y++)
	for(int x=minx; x<=maxx; x++)
	  {
	    if (bitcopy.testBit(bidx))
	      {
		int k0 = qMax(k-1, minz);
		int k1 = qMin(k+1, maxz);
		int y0 = qMax(y-1, miny);
		int y1 = qMin(y+1, maxy);
		int x0 = qMax(x-1, minx);
		int x1 = qMin(x+1, maxx);

		bool ok = true;
		for(int k2=k0; k2<=k1; k2++)
		  for(int y2=y0; y2<=y1; y2++)
		    for(int x2=x0; x2<=x1; x2++)
		      {
			int idx = (k2-minz)*bmy*bmx +
			          (y2-miny)*bmx +
			          (x2-minx);
			bool bc = bitcopy.testBit(idx);
			ok &= bc;
		      }
	    
		if (!ok)
		  {
		    m_bitmask.setBit(bidx);
		    
		    // count number of nonzero voxels
		    // that will give us surface area
		    m_nonZeroVoxels++;
		  }
	      }
	    
	    bidx ++;
	  }
    }

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
}

void
VolumeSingle::createBitmask(int minx, int maxx,
			    int miny, int maxy,
			    int minz, int maxz,
			    uchar* lut,
			    QList<Vec> clipPos,
			    QList<Vec> clipNormal,
			    QList<CropObject> crops,
			    QList<PathObject> paths)
{
  // -- check and generate gradient information
  if (m_pvlVoxelType == 0)
    checkGradients();

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Generating Non-Zero Voxels"));
  Global::progressBar()->show();

  Vec voxelScaling = Global::voxelScaling();

  int bmx = maxx-minx+1;
  int bmy = maxy-miny+1;
  int bmz = maxz-minz+1;

  m_bitmask.resize(bmx*bmy*bmz);
  m_bitmask.fill(false);  

  int nbytes = m_width*m_height;
  uchar *vg = new uchar [2*nbytes];
  
  m_nonZeroVoxels = 0;

  int bidx = 0;
  for(int k=minz; k<=maxz; k++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(k-minz)/(float)(maxz-minz+1)));

      uchar *vslice;
      vslice = m_pvlFileManager.getSlice(k);

      if (m_pvlVoxelType == 0)
	{
	  for(int t=0; t<nbytes; t++)
	    vg[2*t] = vslice[t];

	  vslice = m_gradFileManager.getSlice(k);

	  for(int t=0; t<nbytes; t++)
	    vg[2*t+1] = vslice[t];
	}
      else
	memcpy(vg, vslice, 2*nbytes); // ushort

      for(int y=miny; y<=maxy; y++)
	for(int x=minx; x<=maxx; x++)
	  {
	    Vec po = Vec(x, y, k);
	    po = VECPRODUCT(po, voxelScaling);

	    bool ok = StaticFunctions::getClip(po, clipPos, clipNormal);

	    if (ok)
	      {
		for(int ci=0; ci<crops.count(); ci++)
		  {
		    ok &= crops[ci].checkCropped(po);
		    if (ok == false)
		      break;
		  }
	      }
	    if (ok)
	      {
		for(int ci=0; ci<paths.count(); ci++)
		  {
		    ok &= paths[ci].checkCropped(po);
		    if (ok == false)
		      break;
		  }
	      }


	    // apply clipping
	    if (ok)
	      {
		ushort v,g;
		int idx = (y*m_height + x);
		if (m_pvlVoxelType == 0)
		{
		  v = vg[2*idx];
		  g = vg[2*idx+1];
		}
		else
		  {
		    v = ((ushort*)vg)[idx];
		    g = v%256;
		    v = v/256;
		  }	      
		bool op = (lut[4*(256*g + v)+3] > 0);	   
		if (op)
		  {
		    m_bitmask.setBit(bidx);
		    
		    // count number of nonzero voxels
		    // that will give us volume
		    m_nonZeroVoxels++;
		    
		  }
	      }

	    bidx++;
	  }
    }
  
  delete [] vg;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
}

void
VolumeSingle::findConnectedRegion(QList<Vec> pos,
				  int minx, int maxx,
				  int miny, int maxy,
				  int minz, int maxz,
				  int bmx, int bmy,
				  QList<Vec> clipPos,
				  QList<Vec> clipNormal)
{
  Vec voxelScaling = Global::voxelScaling();

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Identifying Connected Regions"));
  Global::progressBar()->show();

  QBitArray bitcopy = m_bitmask;
  m_bitmask.fill(false);

  QList<Vec> seeds1;
  QList<Vec> seeds2;
  seeds1.clear();
  seeds2.clear();

  for(int pi=0; pi<pos.count(); pi++)
    {
      Vec point = VECPRODUCT(pos[pi], voxelScaling);
      // apply clipping
      if (StaticFunctions::getClip(point, clipPos, clipNormal))
	{
	  int i = pos[pi].x;
	  int j = pos[pi].y;
	  int k = pos[pi].z;
	  int idx = (k-minz)*bmy*bmx + (j-miny)*bmx + (i-minx);
	  if (bitcopy.testBit(idx))
	    {
	      m_bitmask.setBit(idx);
	      seeds2 << pos[pi];
	    }
	}
    }

  int di = 0;

  seeds1 = seeds2;
  seeds2.clear();
  while (seeds1.count())
    {
      di = (++di)%100;
      Global::progressBar()->setValue(di);
      
      for(int si=0; si<seeds1.count(); si++)
	{
	  int i = seeds1[si].x;
	  int j = seeds1[si].y;
	  int k = seeds1[si].z;

	  int k0 = qMax(k-1, minz);
	  int k1 = qMin(k+1, maxz);
	  int y0 = qMax(j-1, miny);
	  int y1 = qMin(j+1, maxy);
	  int x0 = qMax(i-1, minx);
	  int x1 = qMin(i+1, maxx);
	      
	  for(int k2=k0; k2<=k1; k2++)
	    for(int y2=y0; y2<=y1; y2++)
	      for(int x2=x0; x2<=x1; x2++)
		{
		  Vec po = Vec(x2, y2, k2);
		  Vec point = VECPRODUCT(po, voxelScaling);
		  // apply clipping
		  if (StaticFunctions::getClip(point, clipPos, clipNormal))
		    {
		      int idx = (k2-minz)*bmy*bmx +
			        (y2-miny)*bmx +
			        (x2-minx);
		      if (bitcopy.testBit(idx) &&
			  !m_bitmask.testBit(idx))
			{
			  m_bitmask.setBit(idx);
			  seeds2 << po;
			}
		    }
	    }
	}

      seeds1 = seeds2;
      seeds2.clear();

    } // while (!done)

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
}

void
VolumeSingle::saveVolume(uchar *lut,
			 QList<Vec> clipPos,
			 QList<Vec> clipNormal,
			 QList<CropObject> crops,
			 QList<PathObject> paths)
{
  QFileDialog fdialog(0,
		      "Save Volume",
		      Global::previousDirectory(),
		      "Processed (*.pvl.nc) ;; Opacity (*.raw)");

  fdialog.setAcceptMode(QFileDialog::AcceptSave);

  if (!fdialog.exec() == QFileDialog::Accepted)
    return;

  QString opFile = fdialog.selectedFiles().value(0);
  if (opFile.isEmpty())
    return;
  
  bool savePvl = true;
  if (fdialog.selectedNameFilter() == "Processed (*.pvl.nc)")
    {
      savePvl = true;
      if (!opFile.endsWith(".pvl.nc"))
	opFile += ".pvl.nc";
    }
  else if (fdialog.selectedNameFilter() == "Opacity (*.raw)")
    {
      savePvl = false;
      if (!opFile.endsWith(".raw"))
	opFile += ".raw";
    }


  //----------------------------
  // get prune volume
  int gridx,gridy,gridz,lod;
  lod = m_dragTextureInfo.z;
  gridx = m_subvolumeSize.x/lod;
  gridy = m_subvolumeSize.y/lod;
  gridz = m_subvolumeSize.z/lod;
  uchar *prune = new uchar[gridx*gridy*gridz];
  PruneHandler::getRaw(prune,
		       2, // channel Red (BGR)
		       m_dragTextureInfo,
		       m_subvolumeSize);

  bool saveTagged = false;
  bool saveTagValue = true;
  uchar *tagData = 0;
  int tagMin = 1;
  int tagMax = 5;

  QStringList items;
  items << "no" << "yes";
  QString yn = QInputDialog::getItem(0, "Save Tagged Volume",
				     "Save Tags ?",
				     items,
				     0,
				     false);
  if (yn == "yes") saveTagged = true;

  if (saveTagged)
    {
      yn = QInputDialog::getItem(0, "Save Tag Values in the volume",
				 "Yes : Save tag values instead of original voxel or opacity values.\nNo : Use tags as mask for storing original voxel or opacity values",
				 items,
				 1,
				 false);
      if (yn == "no") saveTagValue = false;

      tagData = new uchar[gridx*gridy*gridz];
      PruneHandler::getRaw(tagData,
			   0, // channel Blue (BGR)
			   m_dragTextureInfo,
			   m_subvolumeSize);
      

      bool ok;
      QString text;
      text = QInputDialog::getText(0,
				   "Tag Range",
				   "Tag Range (min to max inclusive)",
				   QLineEdit::Normal,
				   QString("%1 %2").arg(tagMin).arg(tagMax),
				   &ok);
      if (ok && !text.isEmpty())
	{
	  QStringList list = text.split(" ", QString::SkipEmptyParts);
	  if (list.count() > 0) tagMin = list[0].toInt();
	  if (list.count() > 1) tagMax = list[1].toInt();
	}
    }
  //----------------------------
  if (!savePvl)
    {
      if (m_pvlVoxelType == 0)
	checkGradients();
    }



  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Saving Volume"));
  Global::progressBar()->show();
  
  int minx = m_dataMin.x;
  int miny = m_dataMin.y;
  int minz = m_dataMin.z;
  int maxx = m_dataMax.x;
  int maxy = m_dataMax.y;
  int maxz = m_dataMax.z;
  int nx = maxx-minx+1;
  int ny = maxy-miny+1;
  int nz = maxz-minz+1;
  uchar vt = 0;

  //*** max 1Gb per slab
//  int opslabSize = (1024*1024*1024)/(ny*nx);
//  //------------------------------------------------------
//  if (opslabSize < nz)
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
//	opslabSize = nz+1;
//    }
//  //------------------------------------------------------
  int opslabSize = nz+1; // put all in a single file


  if (savePvl)
    {
      if (QFile::exists(opFile)) QFile::remove(opFile);	
      VolumeInformation pvlInfo = volInfo(m_volnum);
      float vx = pvlInfo.voxelSize.x;
      float vy = pvlInfo.voxelSize.y;
      float vz = pvlInfo.voxelSize.z;
      QList<float> rawMap;
      QList<int> pvlMap;
      for(int i=0; i<pvlInfo.mapping.count(); i++)
	{
	  float f = pvlInfo.mapping[i].x();
	  int b = pvlInfo.mapping[i].y();
	  rawMap << f;
	  pvlMap << b;
	}
      StaticFunctions::savePvlHeader(opFile,
				     false, "",
				     pvlInfo.voxelType,
				     m_pvlVoxelType,
				     pvlInfo.voxelUnit,
				     nz, ny, nx,
				     vx, vy, vz,
				     rawMap, pvlMap,
				     pvlInfo.description,
				     opslabSize);
    }
      

  VolumeFileManager opFileManager;
  opFileManager.setBaseFilename(opFile);
  opFileManager.setDepth(nz);
  opFileManager.setWidth(ny);
  opFileManager.setHeight(nx);
  opFileManager.setHeaderSize(13);
  if (savePvl && m_pvlVoxelType > 0)
    opFileManager.setVoxelType(m_pvlVoxelType);
  opFileManager.setSlabSize(opslabSize);
  opFileManager.createFile(true);

  int nbytes = m_width*m_height;
  uchar *vg = new uchar [2*nbytes];
  uchar *opacity = new uchar [2*ny*nx];

  Vec voxelScaling = Global::voxelScaling();
  for(int k=minz; k<=maxz; k++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(k-minz)/(float)nz));

      uchar *vslice;
      vslice = m_pvlFileManager.getSlice(k);

      memset(vg, 0, 2*nbytes);
      if (m_pvlVoxelType == 0)
	{
	  if (!savePvl) // we need grad for saving opacity
	    {
	      for(int t=0; t<nbytes; t++)
		vg[2*t] = vslice[t];
	      
	      vslice = m_gradFileManager.getSlice(k);
	      
	      for(int t=0; t<nbytes; t++)
		vg[2*t+1] = vslice[t];
	    }
	  else
	    memcpy(vg, vslice, nbytes); // uchar	    
	}
      else
	memcpy(vg, vslice, 2*nbytes); // ushort

      int tag=0;
      int idx=0;
      memset(opacity, 0, 2*ny*nx);
      for(int y=miny; y<=maxy; y++)
	for(int x=minx; x<=maxx; x++)
	  { 
	    Vec po = Vec(x, y, k);
	    bool ok = true;

	    // we don't want to scale it before pruning
	    float mop = 0;
	    {
	      Vec pp = po - m_dataMin;
	      int ppi = floor(pp.x)/lod;
	      int ppj = floor(pp.y)/lod;
	      int ppk = floor(pp.z)/lod;
	      ppi = qMin(gridx-1,ppi);
	      ppj = qMin(gridy-1,ppj);
	      ppk = qMin(gridz-1,ppk);
	      mop = prune[ppk*gridy*gridx + ppj*gridx + ppi];
	      ok = (mop > 0);
	      if (saveTagged)
		{
		  tag = tagData[ppk*gridy*gridx + ppj*gridx + ppi];
		  ok &= (tag >= tagMin && tag <= tagMax);		    
		}
	    }
	    mop /= 255.0;

	    po = VECPRODUCT(po, voxelScaling);

	    if (ok)
	      ok = StaticFunctions::getClip(po, clipPos, clipNormal);

	    if (ok)
	      {
		for(int ci=0; ci<crops.count(); ci++)
		  {
		    ok &= crops[ci].checkCropped(po);
		    if (ok == false)
		      break;
		  }
	      }
	    if (ok)
	      {
		for(int ci=0; ci<paths.count(); ci++)
		  {
		    ok &= paths[ci].checkCropped(po);
		    if (ok == false)
		      break;
		  }
	      }

	    // apply clipping
	    if (ok)
	      {
		int lx = (y*m_height + x);
		if (savePvl)
		  {
		    if (m_pvlVoxelType == 0)
		      {
			if (saveTagged && saveTagValue)
			  opacity[idx] = tag;
			else
			  opacity[idx] = vg[lx];
		      }
		    else
			if (saveTagged && saveTagValue)
			  ((ushort*)opacity)[idx] = tag;
			else
			  ((ushort*)opacity)[idx] = ((ushort*)vg)[lx];
		  }
		else
		  {
		    ushort v,g;
		    if (m_pvlVoxelType == 0)
		      {
			v = vg[2*lx];
			g = vg[2*lx+1];
		      }
		    else
		      {
			v = ((ushort*)vg)[lx];
			g = v%256;
			v = v/256;
		      }	      
		    if (saveTagged && saveTagValue)
		      opacity[idx] = tag;
		    else
		      opacity[idx] = mop*lut[4*(256*g + v)+3];
		  }
	      }
	    idx++;
	  }
      opFileManager.setSlice(k-minz, opacity);
    }
  delete [] vg;
  delete [] opacity;
  delete [] prune;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();

  if (savePvl)
    QMessageBox::information(0, "Save Volume",
			   QString("Volume saved to %1").arg(opFile));
  else
    QMessageBox::information(0, "Save Volume",
			     QString("Opacity volume saved to %1.* (001,002,....)").arg(opFile));
}

void
VolumeSingle::maskRawVolume(uchar *lut,
			    QList<Vec> clipPos,
			    QList<Vec> clipNormal,
			    QList<CropObject> crops,
			    QList<PathObject> paths)
{

  int minx = m_dataMin.x;
  int maxx = m_dataMax.x;
  int miny = m_dataMin.y;
  int maxy = m_dataMax.y;
  int minz = m_dataMin.z;
  int maxz = m_dataMax.z;

  createBitmask(minx, maxx,
		miny, maxy,
		minz, maxz,
		lut,
		clipPos, clipNormal,
		crops,
		paths);

  RawVolume::maskRawVolume(minx, maxx,
			   miny, maxy,
			   minz, maxz,
			   m_bitmask);
}

QBitArray
VolumeSingle::getBitmask(uchar *lut,
			 QList<Vec> clipPos,
			 QList<Vec> clipNormal,
			 QList<CropObject> crops,
			 QList<PathObject> paths)
{

  int minx = m_dataMin.x;
  int maxx = m_dataMax.x;
  int miny = m_dataMin.y;
  int maxy = m_dataMax.y;
  int minz = m_dataMin.z;
  int maxz = m_dataMax.z;

  createBitmask(minx, maxx,
		miny, maxy,
		minz, maxz,
		lut,
		clipPos, clipNormal,
		crops,
		paths);

  return m_bitmask;
}




void
VolumeSingle::getColumnsAndRows(int &ncols, int &nrows)
{
  ncols = m_texColumns;
  nrows = m_texRows;
}

void
VolumeSingle::getSliceTextureSize(int& texX, int& texY)
{
  texX = m_texWidth;
  texY = m_texHeight;
}

Vec VolumeSingle::getDragTextureInfo() { return m_dragTextureInfo; }

void
VolumeSingle::getDragTextureSize(int& dtexX, int& dtexY)
{
  dtexX = m_dragTexWidth;
  dtexY = m_dragTexHeight;
}

void
VolumeSingle::setMaxDimensions(int maxH, int maxW, int maxD)
{
  //-------------------------
  // used for centering smaller volumes within larger volume
  m_maxHeight = maxH;
  m_maxWidth = maxW;
  m_maxDepth = maxD;
  //-------------------------

  if (m_sliceTemp)
    delete [] m_sliceTemp;
  
  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;
  m_sliceTemp = new uchar [bpv*m_maxWidth*m_maxHeight];
}

void
VolumeSingle::forMultipleVolumes(int svsl,
				 Vec dts, int dtexw, int dtexh,
				 int texw, int texh,
				 int ncols, int nrows)
{
  m_subvolumeSubsamplingLevel = svsl;

  m_dragTextureInfo = dts;
  m_dragTexWidth = dtexw;
  m_dragTexHeight = dtexh;

  m_texWidth = texw;
  m_texHeight = texh;
  m_texColumns = ncols;
  m_texRows = nrows;

  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;
}

QList<Vec>
VolumeSingle::getSliceTextureSizeSlabs()
{
  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;

  if(m_subvolumeSubsamplingLevel > 1)
    saveSubsampledVolume();
  
  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Generating slab limits"));
  Global::progressBar()->show();


  int nrows, ncols;
  QList<Vec> slabinfo = Global::getSlabs(m_subvolumeSubsamplingLevel,
					 m_dataMin, m_dataMax,
					 nrows, ncols);

  if (slabinfo.count() > 1)
    {
      m_dragTextureInfo = slabinfo[0];
      int dlenx2 = int(m_subvolumeSize.x)/int(m_dragTextureInfo.z);
      int dleny2 = int(m_subvolumeSize.y)/int(m_dragTextureInfo.z);
      m_dragTexWidth = int(m_dragTextureInfo.x)*dlenx2;
      m_dragTexHeight = int(m_dragTextureInfo.y)*dleny2;
    }
  else
    {
      m_dragTextureInfo = Vec(ncols, nrows, m_subvolumeSubsamplingLevel);
      int dlenx2 = int(m_subvolumeSize.x)/m_subvolumeSubsamplingLevel;
      int dleny2 = int(m_subvolumeSize.y)/m_subvolumeSubsamplingLevel;
      m_dragTexWidth = ncols*dlenx2;
      m_dragTexHeight = nrows*dleny2;
    }

  Global::progressBar()->setValue(10);

  m_texColumns = ncols;
  m_texRows = nrows;

  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenx2 = lenx/m_subvolumeSubsamplingLevel;
  int leny2 = leny/m_subvolumeSubsamplingLevel;

  Global::progressBar()->setValue(50);

  m_texWidth = ncols*lenx2;
  m_texHeight = nrows*leny2;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();

  return slabinfo;
}

void
VolumeSingle::saveSubsampledVolume()
{
  if(m_subvolumeSubsamplingLevel <= 1)
    return;

  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;

  int lenx2 = m_height/m_subvolumeSubsamplingLevel;
  int leny2 = m_width/m_subvolumeSubsamplingLevel;
  int lenz2 = m_depth/m_subvolumeSubsamplingLevel;

  //*** number of slices in each tmp file
  //*** max 1Gb per slab
//  int lodslabSize = (1024*1024*1024)/(bpv*lenx2*leny2);
//  if (lodslabSize < lenz2)
//    {  
//      QStringList items;
//      items << "yes" << "no";
//      QString yn = QInputDialog::getItem(0, "Split Subsampled Volume",
//					 "Split subsampled volume larger than 1Gb into multiple files ?",
//					 items,
//					 0,
//					 false);
//      //*** max 1Gb per slab
//      if (yn != "yes") // put all in a single file
//	lodslabSize = lenz2+1;
//    }

  int lodslabSize = lenz2+1;

  QString lodflnm = m_volumeFiles[m_volnum]+QString(".lod%1").arg(m_subvolumeSubsamplingLevel);
  lodflnm = StaticFunctions::replaceDirectory(Global::tempDir(),
					      lodflnm);

  m_lodFileManager.setBaseFilename(lodflnm);
  m_lodFileManager.setDepth(lenz2);
  m_lodFileManager.setWidth(leny2);
  m_lodFileManager.setHeight(lenx2);
  m_lodFileManager.setVoxelType(m_pvlVoxelType);
  m_lodFileManager.setHeaderSize(13);
  m_lodFileManager.setSlabSize(lodslabSize);
  if (m_lodFileManager.exists())
    return;
  m_lodFileManager.createFile(true);

  float *tmp = new float[leny2*lenx2];

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Generating subsampled volume"));
  Global::progressBar()->show();

  int svsl3 = pow((float)m_subvolumeSubsamplingLevel, (float)3);

  for(int kslc=0; kslc<lenz2; kslc++)
    {
      int kmin = kslc*m_subvolumeSubsamplingLevel;
      //int kmax = kmin + m_subvolumeSubsamplingLevel-1;

      Global::progressBar()->setValue((int)(100.0*(float)kslc/(float)lenz2));

      memset(tmp, 0, 4*leny2*lenx2);
      //for(int k=kmin; k<=kmax; k++)
      int k = kmin;
	{
	  uchar *vslice = m_pvlFileManager.getSlice(k);
	  memcpy(m_sliceTemp, vslice, bpv*m_width*m_height);

	  int ji=0;
	  for(int j=0; j<leny2; j++)
	    { 
	      int y = j*m_subvolumeSubsamplingLevel;
	      //int hiy = y+m_subvolumeSubsamplingLevel-1;
	      for(int i=0; i<lenx2; i++) 
		{ 
		  int x = i*m_subvolumeSubsamplingLevel; 
		  if (bpv == 1)
		    tmp[ji] = m_sliceTemp[y*m_height+x];
		  else
		    tmp[ji] = ((ushort*)m_sliceTemp)[y*m_height+x];
		  ji++;

		  //---------------------------------
		  // tri-linear interpolation
		  //int x = i*m_subvolumeSubsamplingLevel; 
		  //int hix = x+m_subvolumeSubsamplingLevel-1;
		  //float sumv = 0; 
		  //if (bpv == 1)
		  //  {
		  //    for(int jy=y; jy<=hiy; jy++) 
		  //      for(int ix=x; ix<=hix; ix++) 
		  //        {
		  //          int idx = jy*m_height+ix;
		  //          sumv += m_sliceTemp[idx];
		  //        }
		  //  }
		  //else
		  //  {
		  //    for(int jy=y; jy<=hiy; jy++) 
		  //      for(int ix=x; ix<=hix; ix++) 
		  //        {
		  //          int idx = jy*m_height+ix;
		  //          sumv += ((ushort*)m_sliceTemp)[idx];
		  //        }
		  //  }
		  //tmp[ji] += sumv; 
		  //ji++;
		  //---------------------------------
		}
	    }
	}

      if (bpv == 1)
	{
	  for(int j=0; j<leny2*lenx2; j++)
	    m_sliceTemp[j] = tmp[j];
	}
      else
	{
	  for(int j=0; j<leny2*lenx2; j++)
	    ((ushort*)m_sliceTemp)[j] = tmp[j];
	}
      //---------------------------------
      // tri-linear interpolation
      //if (bpv == 1)
      //  {
      //    for(int j=0; j<leny2*lenx2; j++)
      //      m_sliceTemp[j] = tmp[j]/svsl3;
      //  }
      //else
      //  {
      //    for(int j=0; j<leny2*lenx2; j++)
      //      ((ushort*)m_sliceTemp)[j] = tmp[j]/svsl3;
      //  }
      ////---------------------------------

      m_lodFileManager.setSlice(kslc, m_sliceTemp);
    }  

  delete [] tmp;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
}

//----------------------------
// for array texture
//----------------------------
uchar*
VolumeSingle::getSubvolume()
{
  MainWindowUI::mainWindowUI()->statusBar->showMessage("Loading data for Hires mode");
  Global::progressBar()->show();

  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;

  int minx = m_dataMin.x;
  int miny = m_dataMin.y;
  int minz = m_dataMin.z;
  
  int maxx = m_dataMax.x;
  int maxy = m_dataMax.y;
  int maxz = m_dataMax.z;

  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenz = m_subvolumeSize.z;

  qint64 lenx2 = m_subvolumeTextureSize.x;
  qint64 leny2 = m_subvolumeTextureSize.y;
  qint64 lenz2 = m_subvolumeTextureSize.z;

  //-------- for dragTexure ---------------
  int dtlod = m_dragTextureInfo.z;
  int dtlenx2 = lenx/dtlod;
  int dtleny2 = leny/dtlod;
  int dtlenz2 = lenz/dtlod;
  float stp = (float)dtlod/(float)m_subvolumeSubsamplingLevel;
  uchar *tmp = new uchar[bpv*dtlenx2*dtleny2];
  //---------------------------------------

  m_dragSubvolumeSubsamplingLevel = dtlod;
  m_dragSubvolumeTextureSize = Vec(dtlenx2, dtleny2, dtlenz2);
  
  if (m_subvolumeTexture) delete [] m_subvolumeTexture;
  if (m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;

  m_subvolumeTexture = new uchar[bpv*lenx2*leny2*lenz2];
  m_dragSubvolumeTexture = new uchar[bpv*dtlenx2*dtleny2*dtlenz2];

  
  uchar *g0, *g1, *g2, *g3;
  g0 = new uchar [bpv*m_maxWidth*m_maxHeight];
  g1 = new uchar [bpv*m_maxWidth*m_maxHeight];
  g2 = new uchar [bpv*m_maxWidth*m_maxHeight];
  g3 = new uchar [bpv*m_maxWidth*m_maxHeight];
    
  //---------------------------------------------------------
  if (m_subvolumeSubsamplingLevel > 1)
    {
      int kmin = minz/m_subvolumeSubsamplingLevel;
      int kmax = maxz/m_subvolumeSubsamplingLevel;

      int leni2 = m_height/m_subvolumeSubsamplingLevel;
      int lenj2 = m_width/m_subvolumeSubsamplingLevel;
      int lenk2 = m_depth/m_subvolumeSubsamplingLevel;

      int imin = minx/m_subvolumeSubsamplingLevel;
      int jmin = miny/m_subvolumeSubsamplingLevel;

      int kbytes = bpv*leni2*lenj2;

      int kslc = 0;

      int offD = m_offD/m_subvolumeSubsamplingLevel;
      int offW = m_offW/m_subvolumeSubsamplingLevel;
      int offH = m_offH/m_subvolumeSubsamplingLevel;

      int maxHsl = m_maxHeight/m_subvolumeSubsamplingLevel;
      int maxWsl = m_maxWidth/m_subvolumeSubsamplingLevel;

      Vec relDataPos = Global::relDataPos();
      if (relDataPos.x < -0.5) offH = 0;
      if (relDataPos.y < -0.5) offW = 0;
      if (relDataPos.z < -0.5) offD = 0;

      if (relDataPos.x > 0.5) offH = (m_maxHeight-m_height)/m_subvolumeSubsamplingLevel;;
      if (relDataPos.y > 0.5) offW = (m_maxWidth-m_width)/m_subvolumeSubsamplingLevel;;
      if (relDataPos.z > 0.5) offD = (m_maxDepth-m_depth)/m_subvolumeSubsamplingLevel;;
      
      uchar *sliceTemp1 = new uchar [bpv*maxWsl*maxHsl];

      for(int k0=kmin; k0<=kmax; k0++)
	{
	  Global::progressBar()->setValue((int)(100.0*(float)(kslc)/(float)(lenz2)));
	  if (kslc%100==0) qApp->processEvents();

	  int k = k0 - offD; // shift slice by given depth offset

	  if (k >= 0 && k<lenk2)
	    {
	      uchar *vslice = m_lodFileManager.getSlice(k);
	      memcpy(m_sliceTemp, vslice, kbytes);
	    }
	  else
	    {
	      memset(m_sliceTemp, 0, kbytes);
	    }

	  //---
	  memset(sliceTemp1, 0, bpv*maxWsl*maxHsl);
	  for(int j=0; j<lenj2; j++)
	    memcpy(sliceTemp1 + bpv*((j+offW)*maxHsl + offH),
		   m_sliceTemp + bpv*j*leni2,
		   bpv*leni2);
	  
	  for(int j=0; j<leny2; j++)
	    memcpy(m_sliceTemp + bpv*j*lenx2,
		   sliceTemp1 + bpv*((j+jmin)*maxHsl + imin),
		   bpv*lenx2);

	  // copy into array texture
	  memcpy(m_subvolumeTexture + bpv*kslc*lenx2*leny2,
		 m_sliceTemp,
		 bpv*lenx2*leny2);
	  //---


	  memcpy(g2, m_sliceTemp, bpv*lenx2*leny2);
	  memset(tmp, 0, bpv*dtlenx2*dtleny2);

	  //-------- form dragSubvolumeTexure ---------------
	  if (k >= (int)(m_dataMin.z/m_subvolumeSubsamplingLevel) &&
	      k <= (int)(m_dataMax.z/m_subvolumeSubsamplingLevel))
	    {
	      int ji=0;
	      if (bpv == 1)
		{
		  for(int j=0; j<dtleny2; j++)
		    { 
		      int y = j*stp;
		      for(int i=0; i<dtlenx2; i++) 
			{ 
			  int x = i*stp; 
			  tmp[ji] = g2[y*lenx2+x];
			  ji++;
			}
		    }
		}
	      else
		{
		  for(int j=0; j<dtleny2; j++)
		    { 
		      int y = j*stp;
		    for(int i=0; i<dtlenx2; i++) 
		      { 
			int x = i*stp; 
			((ushort*)tmp)[ji] = ((ushort*)g2)[y*lenx2+x];
			ji++;
		      }
		  }
		}

	      // copy into array texture
	      int dtkslc = qBound(0, (int)(kslc/stp), dtlenz2-1);
	      memcpy(m_dragSubvolumeTexture + bpv*dtkslc*dtlenx2*dtleny2,
		     tmp,
		     bpv*dtlenx2*dtleny2);
	    }
	  //---------------------------------------

	  if (bpv == 1)
	    {
	      for(int j=0; j<lenx2*leny2; j++)
		m_flhist1D[g2[j]]++;

	      if (kslc >= 2)
		{
		  for(int j=1; j<leny2-1; j++)
		    for(int i=1; i<lenx2-1; i++)
		      {
			int gx = g1[j*lenx2+(i+1)] - g1[j*lenx2+(i-1)];
			int gy = g1[(j+1)*lenx2+i] - g1[(j-1)*lenx2+i];
			int gz = g2[j*lenx2+i] - g0[j*lenx2+i];
			int gsum = sqrtf(gx*gx+gy*gy+gz*gz);
			gsum = qBound(0, gsum, 255);
			int v = g1[j*lenx2+i];
			m_flhist2D[gsum*256 + v]++;
		      }
		}
	    }
	  else
	    {
	      for(int j=0; j<lenx2*leny2; j++)
		m_flhist1D[((ushort*)g2)[j]/256]++;

	      if (kslc >= 2)
		{
		  for(int j=1; j<leny2-1; j++)
		    for(int i=1; i<lenx2-1; i++)
		      {
			int gx = ((ushort*)g1)[j*lenx2+(i+1)] -
			         ((ushort*)g1)[j*lenx2+(i-1)];
			int gy = ((ushort*)g1)[(j+1)*lenx2+i] -
			         ((ushort*)g1)[(j-1)*lenx2+i];
			int gz = ((ushort*)g2)[j*lenx2+i] -
			         ((ushort*)g0)[j*lenx2+i];
			int gsum = sqrtf(gx*gx+gy*gy+gz*gz);
			gsum = qBound(0, gsum/256, 255);
			int v = ((ushort*)g1)[j*lenx2+i]/256;
			m_flhist2D[gsum*256 + v]++;
		      }
		}
	    }

	  
	  uchar *gt = g0;
	  g0 = g1;
	  g1 = g2;
	  g2 = gt;

	  kslc ++;
	}

      delete [] g0;
      delete [] g1;
      delete [] g2;
      delete [] g3;
      delete [] tmp;
      delete [] sliceTemp1;

      Global::progressBar()->setValue(100);
      return m_subvolumeTexture;
    }
  //---------------------------------------------------------


  //---------------------------------------------------------
  //m_subvolumeSubsamplingLevel == 1
  //---------------------------------------------------------
  int nbytes = bpv*m_width*m_height;
  int kslc = 0;

  int offD = m_offD;
  int offW = m_offW;
  int offH = m_offH;

  Vec relDataPos = Global::relDataPos();
  if (relDataPos.x < -0.5) offH = 0;
  if (relDataPos.y < -0.5) offW = 0;
  if (relDataPos.z < -0.5) offD = 0;
  
  if (relDataPos.x > 0.5) offH = m_maxHeight- m_height;
  if (relDataPos.y > 0.5) offW = m_maxWidth - m_width;
  if (relDataPos.z > 0.5) offD = m_maxDepth - m_depth;


  bool quick;
  quick = (offH == 0) && (offW==0) && (offD==0);
  quick = quick && (lenx2==m_height);
  quick = quick && (leny2==m_width);
  quick = quick && (lenz2==m_depth);
  quick = quick && (m_maxHeight==m_height);
  quick = quick && (m_maxWidth==m_width);
  quick = quick && (m_maxDepth==m_depth);

  
  uchar *sliceTemp1 = new uchar [bpv*m_maxWidth*m_maxHeight];

  //-------------------------------------------------------
  for(int k0=minz; k0<=maxz; k0++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(kslc)/(float)(lenz2)));
      if (kslc%100==0) qApp->processEvents();


      int k = k0 - offD; // shift slice by given depth offset
      
      if (k >= 0 && k < m_depth)
	{
	  uchar *vslice = m_pvlFileManager.getSlice(k);
	  memcpy(m_sliceTemp, vslice, nbytes);
	}
      else
	{
	  memset(m_sliceTemp, 0, nbytes);
	}

      if (quick)
	{
	  // copy into array texture
	  memcpy(m_subvolumeTexture + kslc*bpv*lenx2*leny2,
		 m_sliceTemp,
		 bpv*lenx2*leny2);
	}
      else
	{
	  //---
	  memset(sliceTemp1, 0, bpv*m_maxWidth*m_maxHeight);
	  for(int j=0; j<m_width; j++)
	    memcpy(sliceTemp1 + bpv*((j+offW)*m_maxHeight + offH),
		   m_sliceTemp + bpv*j*m_height,
		   bpv*m_height);
	  
	  for(int j=0; j<leny2; j++)
	    memcpy(m_sliceTemp + bpv*j*lenx2,
		   sliceTemp1 + bpv*((j+miny)*m_maxHeight + minx),
		   bpv*lenx2);
	  
	  // copy into array texture
	  memcpy(m_subvolumeTexture + kslc*bpv*lenx2*leny2,
		 m_sliceTemp,
		 bpv*lenx2*leny2);
	  //---
	}
      
      memcpy(g2, m_sliceTemp, bpv*lenx2*leny2);

      //-------- form dragTexure ---------------
      if (k >= (int)m_dataMin.z && k <= (int)m_dataMax.z)
	{
	  int ji=0;
	  if (bpv == 1)
	    {
	      for(int j=0; j<dtleny2; j++)
		{ 
		  int y = j*stp;
		  for(int i=0; i<dtlenx2; i++) 
		    { 
		      int x = i*stp; 
		      tmp[ji] = g2[y*lenx2+x];
		      ji++;
		    }
		}
	    }
	  else
	    {
	      for(int j=0; j<dtleny2; j++)
		{ 
		  int y = j*stp;
		  for(int i=0; i<dtlenx2; i++) 
		    { 
		      int x = i*stp; 
		      ((ushort*)tmp)[ji] = ((ushort*)g2)[y*lenx2+x];
		      ji++;
		    }
		}
	    }
	  
	  // copy into array texture
	  int dtkslc = qBound(0, (int)(kslc/stp), dtlenz2-1);
	  memcpy(m_dragSubvolumeTexture + bpv*dtkslc*dtlenx2*dtleny2,
		 tmp,
		 bpv*dtlenx2*dtleny2);
	}
      //---------------------------------------

      if (bpv == 1)
	{
	  for(int j=0; j<lenx2*leny2; j++)
	    m_flhist1D[g2[j]]++;
	  
	  if (kslc >= 2)
	    {
	      for(int j=1; j<leny2-1; j++)
		for(int i=1; i<lenx2-1; i++)
		  {
		    int gx = g1[j*lenx2+(i+1)] - g1[j*lenx2+(i-1)];
		    int gy = g1[(j+1)*lenx2+i] - g1[(j-1)*lenx2+i];
		    int gz = g2[j*lenx2+i] - g0[j*lenx2+i];
		    int gsum = sqrtf(gx*gx+gy*gy+gz*gz);
		    gsum = qBound(0, gsum, 255);
		    int v = g1[j*lenx2+i];
		    m_flhist2D[gsum*256 + v]++;
		  }	  
	    }
	}
      else
	{
	  for(int j=0; j<lenx2*leny2; j++)
	    m_flhist1D[((ushort*)g2)[j]/256]++;
	  
	  for(int j=0; j<lenx2*leny2; j++)
	    m_flhist2D[((ushort*)g2)[j]]++;
	}
 
      uchar *gt = g0;
      g0 = g1;
      g1 = g2;
      g2 = gt;

      kslc ++;
    }

  delete [] g0;
  delete [] g1;
  delete [] g2;
  delete [] g3;
  delete [] tmp;
  delete [] sliceTemp1;

  Global::progressBar()->setValue(100);
  MainWindowUI::mainWindowUI()->statusBar->showMessage("Ready");

  return m_subvolumeTexture;

}


void
VolumeSingle::startHistogramCalculation()
{
  memset(m_flhist1D, 0, 256*4);
  memset(m_flhist2D, 0, 256*256*4);
}

void
VolumeSingle::endHistogramCalculation()
{
  memset(m_subvolume1dHistogram, 0, 256*4);
  memset(m_subvolume2dHistogram, 0, 256*256*4);
  
  if (m_pvlVoxelType == 0)
    StaticFunctions::generateHistograms(m_flhist1D, m_flhist2D,
					m_subvolume1dHistogram,
					m_subvolume2dHistogram);
  else // just copy
    {
      for(int i=0; i<256; i++)
	m_subvolume1dHistogram[i] = m_flhist1D[i];
      for(int i=0; i<256*256; i++)
	m_subvolume2dHistogram[i] = m_flhist2D[i];
    }

  calculateGradientsForDragTexture();
}

void
VolumeSingle::calculateGradientsForDragTexture()
{
  MainWindowUI::mainWindowUI()->statusBar->showMessage("Histogram calculation");
  Global::progressBar()->show();
  qApp->processEvents();

  memset(m_flhist1D, 0, 256*4);
  memset(m_flhist2D, 0, 256*256*4);

  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;

  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenz = m_subvolumeSize.z;
  int dtlod = m_dragTextureInfo.z;
  int dtlenx2 = lenx/dtlod;
  int dtleny2 = leny/dtlod;
  int dtlenz2 = lenz/dtlod;

  for(int i=0;i<dtlenz2;i++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)i/(float)dtlenz2));
      if (i%10==0) qApp->processEvents();

      for(int j=0;j<dtleny2;j++)
	for(int k=0;k<dtlenx2;k++)
	  {
	    int i0,j0,k0;
	    
	    int v = m_dragSubvolumeTexture[i*dtleny2*dtlenx2 + j*dtlenx2 + k];
	    m_flhist1D[v]++;
	    
	    float dval;
	    float ax = 1.0;
	    float ay = -1.0;
	    
	    i0 = qBound(0, i + (int)ax, dtlenz2-1);
	    j0 = qBound(0, j + (int)ay, dtleny2-1);
	    k0 = qBound(0, k + (int)ay, dtlenx2-1);
	    if (bpv == 1)
	      dval = m_dragSubvolumeTexture[i0*dtleny2*dtlenx2 + j0*dtlenx2 + k0];
	    else
	      dval = ((ushort*)m_dragSubvolumeTexture)[i0*dtleny2*dtlenx2 + j0*dtlenx2 + k0];
	    Vec gxyy = Vec(ax, ay, ay) * dval;

	    
	    i0 = qBound(0, i + (int)ay, dtlenz2-1);
	    j0 = qBound(0, j + (int)ay, dtleny2-1);
	    k0 = qBound(0, k + (int)ax, dtlenx2-1);
	    if (bpv == 1)
	      dval = m_dragSubvolumeTexture[i0*dtleny2*dtlenx2 + j0*dtlenx2 + k0];
	    else
	      dval = ((ushort*)m_dragSubvolumeTexture)[i0*dtleny2*dtlenx2 + j0*dtlenx2 + k0];
	    Vec gyyx = Vec(ay, ay, ax) * dval;

	    
	    i0 = qBound(0, i + (int)ay, dtlenz2-1);
	    j0 = qBound(0, j + (int)ax, dtleny2-1);
	    k0 = qBound(0, k + (int)ay, dtlenx2-1);
	    if (bpv == 1)
	      dval = m_dragSubvolumeTexture[i0*dtleny2*dtlenx2 + j0*dtlenx2 + k0];
	    else
	      dval = ((ushort*)m_dragSubvolumeTexture)[i0*dtleny2*dtlenx2 + j0*dtlenx2 + k0];
	    Vec gyxy = Vec(ay, ax, ay) * dval;

	    
	    i0 = qBound(0, i + (int)ax, dtlenz2-1);
	    j0 = qBound(0, j + (int)ax, dtleny2-1);
	    k0 = qBound(0, k + (int)ax, dtlenx2-1);
	    if (bpv == 1)
	      dval = m_dragSubvolumeTexture[i0*dtleny2*dtlenx2 + j0*dtlenx2 + k0];
	    else
	      dval = ((ushort*)m_dragSubvolumeTexture)[i0*dtleny2*dtlenx2 + j0*dtlenx2 + k0];
	    Vec gxxx = Vec(ax, ax, ax) * dval;

	    
	    Vec grad = (gxyy+gyyx+gyxy+gxxx)/2.0;
	    int g = qBound(0.0, grad.norm(), 255.0);
	    
	    m_flhist2D[(int)g*256 + v]++;
	  }
    }

  //-----
  // for drag histogram calculation
  memset(m_drag1dHistogram, 0, 256*4);
  memset(m_drag2dHistogram, 0, 256*256*4);

  if (m_pvlVoxelType == 0)
    StaticFunctions::generateHistograms(m_flhist1D, m_flhist2D,
					m_drag1dHistogram,
					m_drag2dHistogram);
  else // just copy
    {
      for(int i=0; i<256; i++)
	m_drag1dHistogram[i] = m_flhist1D[i];
      for(int i=0; i<256*256; i++)
	m_drag2dHistogram[i] = m_flhist2D[i];
    }
  //-----

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  MainWindowUI::mainWindowUI()->statusBar->showMessage("");
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
}

void
VolumeSingle::checkGradients()
{
  //---- check with grad file manager -------------
  int n_depth, n_width, n_height;
  int slabSize;
  XmlHeaderFunctions::getDimensionsFromHeader(m_volumeFiles[m_volnum],
					      n_depth, n_width, n_height);
  slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFiles[m_volnum]);

  QString vgfile = m_volumeFiles[m_volnum];
  vgfile.chop(6);
  vgfile += QString("grad");
  vgfile = StaticFunctions::replaceDirectory(Global::tempDir(),
					     vgfile);
  m_gradFileManager.setBaseFilename(vgfile);
  m_gradFileManager.setDepth(n_depth);
  m_gradFileManager.setWidth(n_width);
  m_gradFileManager.setHeight(n_height);
  m_gradFileManager.setVoxelType(m_pvlVoxelType);
  m_gradFileManager.setHeaderSize(13);
  m_gradFileManager.setSlabSize(slabSize);
  if (! m_gradFileManager.exists())
    createGradVolume();
}


void
VolumeSingle::createGradVolume()
{
  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;

  //---- create grad file manager -------------
  int n_depth, n_width, n_height;
  int slabSize;
  XmlHeaderFunctions::getDimensionsFromHeader(m_volumeFiles[m_volnum],
					      n_depth, n_width, n_height);
  slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFiles[m_volnum]);


  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Allocating space for gradient values on disk"));

  QString vgfile = m_volumeFiles[m_volnum];
  vgfile.chop(6);
  vgfile += QString("grad");
  vgfile = StaticFunctions::replaceDirectory(Global::tempDir(),
					     vgfile);
  m_gradFileManager.setBaseFilename(vgfile);
  m_gradFileManager.setDepth(n_depth);
  m_gradFileManager.setWidth(n_width);
  m_gradFileManager.setHeight(n_height);
  m_gradFileManager.setVoxelType(m_pvlVoxelType);
  m_gradFileManager.setHeaderSize(13);
  m_gradFileManager.setSlabSize(slabSize);
  m_gradFileManager.createFile(true);
  //-------------------------------------------

  int minx = 0;
  int miny = 0;
  int minz = 0;
  int maxx = m_height;
  int maxy = m_width;
  int maxz = m_depth;

  int lenx = m_height;
  int leny = m_width;
  int lenz = m_depth;
  
  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Saving gradient magnitude volume"));
  Global::progressBar()->show();
  qApp->processEvents();


  int nbytes = bpv*m_width*m_height;
  uchar *tmp, *g0, *g1, *g2;
  tmp = new uchar [nbytes];
  g0  = new uchar [nbytes];
  g1  = new uchar [nbytes];
  g2  = new uchar [nbytes];

  memset(g0, 0, nbytes);
  memset(g1, 0, nbytes);
  memset(g2, 0, nbytes);

  for(int kslc=0; kslc<m_depth; kslc++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)kslc/(float)m_depth));
      if (kslc%100==0) qApp->processEvents();

      uchar *vslice;
      vslice = m_pvlFileManager.getSlice(kslc);

      memcpy(g2, vslice, nbytes);

      memset(tmp, 0, nbytes);

      if (kslc >= 2)
	{
	  if (bpv == 1)
	    {
	      for(int j=1; j<m_width-1; j++)
		for(int i=1; i<m_height-1; i++)
		  {
		    int gx = g1[j*m_height+(i+1)] - g1[j*m_height+(i-1)];
		    int gy = g1[(j+1)*m_height+i] - g1[(j-1)*m_height+i];
		    int gz = g2[j*m_height+i] - g0[j*m_height+i];
		    int gsum = sqrtf(gx*gx+gy*gy+gz*gz);
		    gsum = qBound(0, gsum, 255);
		    tmp[j*m_height+i] = gsum;
		  }
	    }
	  else
	    {
	      for(int j=1; j<m_width-1; j++)
		for(int i=1; i<m_height-1; i++)
		  {
		    int gx = ((ushort*)g1)[j*m_height+(i+1)] -
		             ((ushort*)g1)[j*m_height+(i-1)];
		    int gy = ((ushort*)g1)[(j+1)*m_height+i] -
		             ((ushort*)g1)[(j-1)*m_height+i];
		    int gz = ((ushort*)g2)[j*m_height+i] -
		             ((ushort*)g0)[j*m_height+i];
		    int gsum = sqrtf(gx*gx+gy*gy+gz*gz);
		    //gsum = qBound(0, gsum, 255);
		    ((ushort*)tmp)[j*m_height+i] = gsum;
		  }
	    }
	}

      if (kslc==0 || kslc==m_depth-1) // first or last slice
	m_gradFileManager.setSlice(kslc, tmp);
      else
	m_gradFileManager.setSlice(kslc-1, tmp);

      uchar *gt = g0;
      g0 = g1;
      g1 = g2;
      g2 = gt;
    }

  delete [] tmp;
  delete [] g0;
  delete [] g1;
  delete [] g2;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
}

void
VolumeSingle::saveSliceImage(Vec pos,
			     Vec normal, Vec xaxis, Vec yaxis,
			     float scalex, float scaley,
			     int step)
{
  int wd = 2*scalex/step;
  int ht = 2*scaley/step;

  Vec ptop = pos - scaley*yaxis - scalex*xaxis;

  uchar *slice = new uchar[4*wd*ht];
  memset(slice, 0, 4*wd*ht);


  QProgressDialog progress("Extracting slice image",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);
  
  int blksz = 5;
  for(int ih=ht-1; ih>=0; ih-=blksz)
    {
      progress.setValue((int)(100.0*(float)(ht-ih)/(float)ht));
      if (ih%100==0) qApp->processEvents();
      
      for(int iw=0; iw<wd; iw+=blksz)
	{

	  for(int ihh=ih; ihh>=qMax(0, ih-blksz); ihh--)
	    for(int iww=iw; iww<qMin(iw+blksz,wd); iww++)
	      {
		Vec po = ptop + ihh*yaxis*step + iww*xaxis*step;
		
		uchar v = 0;
		if (m_pvlVoxelType == 0)
		  memcpy(&v, m_pvlFileManager.interpolatedRawValue(po.z, po.y, po.x), 1);
		else
		  {
		    ushort *va = (ushort*)(m_pvlFileManager.interpolatedRawValue(po.z, po.y, po.x));
		    v = (*va)/256;
		  }
		
		int idx = ihh*wd + iww;
		slice[4*idx+0] = v;
		slice[4*idx+1] = v;
		slice[4*idx+2] = v;
		slice[4*idx+3] = 255;
	      }
	}
    }
  progress.setValue(100);

  QImage img = QImage(slice, wd, ht, QImage::Format_ARGB32);

  QString flnm = QFileDialog::getSaveFileName(0,
					      "Save image slice",
					      Global::previousDirectory(),
					      "Image Files (*.png *.tif *.bmp *.jpg)");

  if (flnm.isEmpty())
    return;

  img.save(flnm);

  QMessageBox::information(0, "", "done");

  delete [] slice;
}


void
VolumeSingle::resliceVolume(Vec pos,
			    Vec normal, Vec xaxis, Vec yaxis,
			    float scalex, float scaley,
			    int step1, int step2)
{
  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;

  //--- drop perpendiculars onto normal from all 8 vertices of the subvolume 
  Vec box[8];
  box[0] = Vec(m_dataMin.x, m_dataMin.y, m_dataMin.z);
  box[1] = Vec(m_dataMin.x, m_dataMin.y, m_dataMax.z);
  box[2] = Vec(m_dataMin.x, m_dataMax.y, m_dataMax.z);
  box[3] = Vec(m_dataMin.x, m_dataMax.y, m_dataMin.z);
  box[4] = Vec(m_dataMax.x, m_dataMin.y, m_dataMin.z);
  box[5] = Vec(m_dataMax.x, m_dataMin.y, m_dataMax.z);
  box[6] = Vec(m_dataMax.x, m_dataMax.y, m_dataMax.z);
  box[7] = Vec(m_dataMax.x, m_dataMax.y, m_dataMin.z);
  float boxdist[8];

  // get number of slices
  for (int i=0; i<8; i++) boxdist[i] = (box[i] - pos)*normal;
  float dmin = boxdist[0];
  for (int i=1; i<8; i++) dmin = qMin(dmin, boxdist[i]);
  float dmax = boxdist[0];
  for (int i=1; i<8; i++) dmax = qMax(dmax, boxdist[i]);
  //------------------------

  // get width
  for (int i=0; i<8; i++) boxdist[i] = (box[i] - pos)*xaxis;
  float wmin = boxdist[0];
  for (int i=1; i<8; i++) wmin = qMin(wmin, boxdist[i]);
  float wmax = boxdist[0];
  for (int i=1; i<8; i++) wmax = qMax(wmax, boxdist[i]);
  //------------------------

  // get height
  for (int i=0; i<8; i++) boxdist[i] = (box[i] - pos)*yaxis;
  float hmin = boxdist[0];
  for (int i=1; i<8; i++) hmin = qMin(hmin, boxdist[i]);
  float hmax = boxdist[0];
  for (int i=1; i<8; i++) hmax = qMax(hmax, boxdist[i]);
  //------------------------


  int nslices = (dmax-dmin)/step2;
  int wd = (wmax-wmin)/step1;
  int ht = (hmax-hmin)/step1;
  Vec sliceZero = pos + dmin*normal + wmin*xaxis + hmin*yaxis;

  int blksz = 10;
  uchar *slice = new uchar[bpv*blksz*wd*ht];
  memset(slice, 0, bpv*blksz*wd*ht);


  QString flnm = QFileDialog::getSaveFileName(0,
					      "Save resliced volume to",
					      Global::previousDirectory(),
					      "RAW Files (*.raw)");

  if (flnm.isEmpty())
    return;

  
  int slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFiles[0]);

  VolumeFileManager resliceFileManager;
  resliceFileManager.setBaseFilename(flnm);
  resliceFileManager.setDepth(nslices);
  resliceFileManager.setWidth(ht);
  resliceFileManager.setHeight(wd);
  resliceFileManager.setVoxelType(m_pvlVoxelType);
  resliceFileManager.setHeaderSize(13); // default is 13 bytes
  resliceFileManager.setSlabSize(slabSize);
  resliceFileManager.createFile(true);

  QProgressDialog progress("Reslicing volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);
  
  m_pvlFileManager.startBlockInterpolation();

  for(int iv=0; iv<nslices; iv+=blksz)
    {
      progress.setLabelText(QString("Reslicing volume (%1 of %2)").arg(iv).arg(nslices));
      float frc = (float)iv/(float)nslices;
      memset(slice, 0, bpv*blksz*wd*ht);
      for(int ih=0; ih<ht; ih+=blksz)
	{
	  progress.setValue((int)(100.0*(frc + (1-frc)*(float)ih/(float)ht)));
	  if (ih%10==0) qApp->processEvents();
      
	  for(int iw=0; iw<wd; iw+=blksz)
	    {
	      int ivend = qMin(iv+blksz, nslices);
	      int ihend = qMin(ih+blksz,ht);
	      int iwend = qMin(iw+blksz,wd);
	      for(int ivv=iv; ivv<ivend; ivv++)
	      for(int ihh=ih; ihh<ihend; ihh++)
	      for(int iww=iw; iww<iwend; iww++)
		{
		  Vec po = (sliceZero +
		            ivv*normal*step2 +
		            ihh*yaxis*step1 +
		            iww*xaxis*step1);
		      
		  int idx = (ivv-iv)*wd*ht + ihh*wd + iww;
		  if (m_pvlVoxelType == 0)
		    {
		      uchar *v = m_pvlFileManager.blockInterpolatedRawValue(po.z, po.y, po.x);
		      slice[idx] = *v;
		    }
		  else
		    {
		      ushort *v = (ushort*)(m_pvlFileManager.blockInterpolatedRawValue(po.z, po.y, po.x));
		      ((ushort*)slice)[idx] = *v;
		    }
		}
	    }
	}

      for(int ivv=iv; ivv<qMin(iv+blksz, nslices); ivv++)
	resliceFileManager.setSlice(ivv, slice+bpv*(ivv-iv)*wd*ht);
    }

  m_pvlFileManager.endBlockInterpolation();

  progress.setValue(100);

  QMessageBox::information(0, "", "done");

  delete [] slice;
}

QList<QVariant>
VolumeSingle::rawValues(QList<Vec> pos)
{
  QList<QVariant> raw;

  Vec voxelScaling = Global::voxelScaling();
  // now start extracting the raw values  
  for(int pi=0; pi<pos.size(); pi++)
    {
      Vec u = VECDIVIDE(pos[pi], voxelScaling);
      int h = u.x;
      int w = u.y;
      int d = u.z;

      if (d < 0 || d >= m_depth ||
	  w < 0 || w >= m_width ||
	  h < 0 || h >= m_height)
	raw << QVariant("OutOfBounds");
      else
	{
	  uchar *va = m_pvlFileManager.rawValue(d, w, h);
	  if (!va)
	    raw << QVariant("OutOfBounds");
	  else
	    {
	      uint *a = (uint*)va;
	      raw << QVariant((uint)*a);
	    }
	}
    }

  return raw;
}

QMap<QString, QList<QVariant> >
VolumeSingle::rawValues(int radius,
			QList<Vec> pos)
{
  Vec voxelScaling = Global::voxelScaling();
  int radiush = radius/voxelScaling.x;
  int radiusw = radius/voxelScaling.y;
  int radiusd = radius/voxelScaling.z;


  QList<QVariant> rawMin;
  QList<QVariant> raw;
  QList<QVariant> rawMax;

  QMap<QString, QList<QVariant> > valueMap;

  QProgressDialog progress("Generating profile data",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  QHash<qint64, ushort> hashTable;

  // now start extracting the raw values
  for(int pi=0; pi<pos.size(); pi++)
    {
      progress.setValue((int)(100.0*(float)pi/(float)pos.size()));
      if (pi%10==0) qApp->processEvents();

      Vec u = VECDIVIDE(pos[pi], voxelScaling);
      int h = u.x;
      int w = u.y;
      int d = u.z;

      if (d < 0 || d >= m_depth ||
	  w < 0 || w >= m_width ||
	  h < 0 || h >= m_height)
	{
	  raw << QVariant("OutOfBounds");
	  rawMin << QVariant("OutOfBounds");
	  rawMax << QVariant("OutOfBounds");
	}
      else
	{
	  int imin, imax, jmin, jmax, kmin, kmax;
	  imin = h-radiush; imin = qMax(0, imin);
	  imax = h+radiush; imax = qMin(imax, m_height-1);
	  jmin = w-radiusw; jmin = qMax(0, jmin);
	  jmax = w+radiusw; jmax = qMin(jmax, m_width-1);
	  kmin = d-radiusd; kmin = qMax(0, kmin);
	  kmax = d+radiusd; kmax = qMin(kmax, m_depth-1);

	  int navg = 0;
	  double avg = 0;
	  ushort minVal, maxVal;
	  for(int vi=imin; vi<=imax; vi++)
	  for(int vj=jmin; vj<=jmax; vj++)
	  for(int vk=kmin; vk<=kmax; vk++)
	    {
	      navg++;
	      
	      qint64 idx = vk*m_width*m_height +
		           vj*m_height + vi;
	      ushort val;
	      if (hashTable.contains(idx))
		{
		  val = hashTable[idx];
		}
	      else
		{
		  uchar *a = m_pvlFileManager.rawValue(vk, vj, vi);
		  hashTable[idx] = (ushort)*a;
		  val = (ushort)*a;
		}

	      avg += val;

	      if (navg > 1)
		{
		  minVal = qMin(minVal, val);
		  maxVal = qMax(maxVal, val);
		}
	      else
		{
		  minVal = maxVal = val;
		}
	    }

	  // take average
	  avg /= navg;

	  // convert to QVariant
	  QVariant v, vmin, vmax;
	  v = QVariant((uint)avg);
	  vmin = QVariant((uint)minVal);
	  vmax = QVariant((uint)maxVal);

	  raw.append(v);
	  rawMin.append(vmin);
	  rawMax.append(vmax);
	}
    }

  progress.setValue(100);
  qApp->processEvents();


  valueMap["raw"] = raw;
  valueMap["rawMin"] = rawMin;
  valueMap["rawMax"] = rawMax;
  
  return valueMap;
}

QList<float>
VolumeSingle::getThicknessProfile(int searchType,
				  uchar *lut,
				  QList<Vec> voxel,
				  QList<Vec> normal)
{
  QList<float> thickness;
  QList<Vec> tcrd;

//-----------------------------
//  do not use gradient information
//  use only voxel intensity information
//  // -- check and generate gradient information
//  checkGradients();
//-----------------------------

  Vec voxelScaling = Global::voxelScaling();

  for(int p=0; p<voxel.count(); p++)
    {
      int nlen = normal[p].norm();
      int nl = 4*nlen;

      bool vtflag[2];
      Vec vt[2];

      for(int side=0; side<2; side++)
	{
	  vtflag[side] = false;
	  Vec u = VECDIVIDE(voxel[p], voxelScaling);
	  Vec uv = normal[p].unit();
	  if (side == 0)
	    {
	      // seed point assumed to be very close to the surface
	      // take a few steps back before starting the search
	      // just in case the point is slightly within the surface
	      // search in the direction of the normal
	      u = u - 5*uv;
	    }
	  else
	    {
	      if (searchType == 1)
		{
		  // start from far inside the surface to find a point
		  // on opposite side of the surface
		  // search in the direction opposite to the normal
		  u = u + nlen*uv;
		  uv = -uv;
		}
	      else
		{
		  // start from already searched point on the surface
		  // search in the direction of the normal
		  u = vt[0];
		}
	    }
	  uv = uv * 0.5;

	  for(int i=0; i<nl; i++)
	    {
	      ushort v, g;
	      int d, w, h;
	      d = u.z;
	      w = u.y;
	      h = u.x;

	      v = (ushort)*m_pvlFileManager.rawValue(d, w, h);
	      g = 0;

//-----------------------------
//	      do not use gradient information
//            use only voxel intensity information
//	      g = (ushort)*m_gradFileManager.rawValue(d, w, h);
//-----------------------------

	      if (m_pvlVoxelType > 0) // modify v & g
		{
		  g = v%256;
		  v = v/256;
		}
	      
	      if (side == 0 || searchType == 1)
		{
		  if (lut[4*(256*g + v) + 3] > 0)
		    {
		      vt[side] = u;
		      vtflag[side] = true;
		      break;
		    }
		}
	      else
		{
		  if (lut[4*(256*g + v) + 3] == 0)
		    {
		      vt[side] = u;
		      vtflag[side] = true;
		      break;
		    }
		}
	      
	      u = u + uv;
	    }
	}

      if (vtflag[0] && vtflag[1])
	{
	  tcrd << vt[0];
	  tcrd << vt[1];
	  thickness << (vt[0]-vt[1]).norm();
	}
      else
	thickness.append(0);
    }

  //--------------------------------------
  // save endpoints in a path group
  QString pflnm;
  pflnm = QFileDialog::getSaveFileName(0,
				      "Save end-points as path group ?",
				      Global::previousDirectory(),
				      "Files (*.path)");
  if (! pflnm.isEmpty())
    {
      QFile pfile(pflnm);
      pfile.open(QIODevice::WriteOnly | QIODevice::Text);
      QTextStream out(&pfile);
      for(int i=0; i<tcrd.count(); i+=2)
	{
	  out << "2\n";
	  out << tcrd[i].x << " ";
	  out << tcrd[i].y << " ";
	  out << tcrd[i].z << " ";
	  out << "\n";
	  out << tcrd[i+1].x << " ";
	  out << tcrd[i+1].y << " ";
	  out << tcrd[i+1].z << " ";
	  out << "\n";
	}
    }
  //--------------------------------------

  return thickness;
}

QList<Vec>
VolumeSingle::stickToSurface(uchar *lut,
			     int rad,
			     QList< QPair<Vec,Vec> > pn)
{
//-----------------------------
//  do not use gradient information
//  use only voxel intensity information
//  // -- check and generate gradient information
//  checkGradients();
//-----------------------------

  Vec voxelScaling = Global::voxelScaling();

  QList<Vec> pts;

  for(int p=0; p<pn.count(); p++)
    {
      Vec voxel = pn[p].first;
      Vec normal = pn[p].second;

      Vec u = VECDIVIDE(voxel, voxelScaling);
      int d, w, h;
      d = u.z;
      w = u.y;
      h = u.x;  
      ushort v, g;
      v = (ushort)*m_pvlFileManager.rawValue(d, w, h);
      g = 0;

      if (m_pvlVoxelType > 0) // modify v & g
	{
	  g = v%256;
	  v = v/256;
	}
	      
      bool inside = false;
      if (lut[4*(256*g + v) + 3] > 0)
	inside = true;

      bool vtflag[2];
      Vec vt[2];

      for(int side=0; side<2; side++)
	{
	  vtflag[side] = false;

	  u = VECDIVIDE(voxel, voxelScaling);

	  Vec uv = 0.5*normal;
	  if (side == 0)
	    uv = -uv;

	  for(int i=0; i<2*rad; i++)
	    {
	      d = u.z;
	      w = u.y;
	      h = u.x;  
	      v = (ushort)*m_pvlFileManager.rawValue(d, w, h);

//-----------------------------
//	      do not use gradient information
//            use only voxel intensity information
//	      g = *m_gradFileManager.rawValue(d, w, h);
//-----------------------------
	  
	      if (m_pvlVoxelType > 0) // modify v & g
		{
		  g = v%256;
		  v = v/256;
		}
	      
	      if (inside)
		{
		  if (lut[4*(256*g + v) + 3] == 0)
		    {
		      vt[side] = u;
		      vtflag[side] = true;
		      break;
		    }
		}
	      else
		{
		  if (lut[4*(256*g + v) + 3] > 0)
		    {
		      vt[side] = u;
		      vtflag[side] = true;
		      break;
		    }
		}
	      
	      u = u + uv;
	    }
	}

      float len0 = 10000000.0f;
      float len1 = 10000000.0f;
      if (vtflag[0])
	len0 = (voxel-vt[0]).squaredNorm();
      if (vtflag[1])
	len1 = (voxel-vt[1]).squaredNorm();

      if (vtflag[0] || vtflag[1])
	{
	  if (len0 < len1)
	    pts.append(vt[0]);
	  else
	    pts.append(vt[1]);
	}
      else
	pts.append(voxel);
    }

  return pts;
}

void
VolumeSingle::countIsolatedRegions(uchar *lut,
				   QList<Vec> clipPos,
				   QList<Vec> clipNormal,
				   QList<CropObject> crops,
				   QList<PathObject> paths)
{
  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Counting Isolated Regions"));

  MainWindowUI::mainWindowUI()->statusBar->show();
  MainWindowUI::mainWindowUI()->statusBar->showMessage("Counting Isolated Regions");
  
  int minx = m_dataMin.x;
  int maxx = m_dataMax.x;
  int miny = m_dataMin.y;
  int maxy = m_dataMax.y;
  int minz = m_dataMin.z;
  int maxz = m_dataMax.z;
  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenz = m_subvolumeSize.z;

  createBitmask(minx, maxx,
		miny, maxy,
		minz, maxz,
		lut,
		clipPos, clipNormal,
		crops,
		paths);

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Counting Isolated Regions"));

  MainWindowUI::mainWindowUI()->statusBar->show();
  MainWindowUI::mainWindowUI()->statusBar->showMessage("Counting Isolated Regions");
    
  Global::progressBar()->show();
  
  
  QBitArray bitcopy = m_bitmask;
  int ncells = 0;
  bool done = false;
  while (!done)
    {
      bool gotseed = false;
      Vec pos;
      int bidx = 0;
      for(int z=minz; z<=maxz; z++)
	for(int y=miny; y<=maxy; y++)
	  for(int x=minx; x<=maxx; x++)
	    {
	      if (bitcopy.testBit(bidx))
		{
		  bitcopy.clearBit(bidx);
		  gotseed = true;
		  pos = Vec(x,y,z);
		  z = maxz+1;
		  y = maxy+1;
		  x = maxx+1;
		  break;
		}
	      bidx ++;
	    }


      if (!gotseed)
	{
	  done = true;
	  break;
	}

      ncells ++;
      MainWindowUI::mainWindowUI()->statusBar->showMessage(QString("Isolated Regions : %1").arg(ncells));
      
      QStack<Vec> posStack;
      posStack.push(pos);
      int pb = 0;
      while(!posStack.isEmpty())
	{
	  Global::progressBar()->setValue(ncells%99);
	  if (pb%10==0) qApp->processEvents();
	  //qApp->processEvents();

	  pb = pb%1000;

	  pos = posStack.pop();
	  int x = pos.x;
	  int y = pos.y;
	  int z = pos.z;

	  int z0 = qMax(z-1, minz);
	  int z1 = qMin(z+1, maxz);
	  int y0 = qMax(y-1, miny);
	  int y1 = qMin(y+1, maxy);
	  int x0 = qMax(x-1, minx);
	  int x1 = qMin(x+1, maxx);
	      
	  for(int z2=z0; z2<=z1; z2++)
	    for(int y2=y0; y2<=y1; y2++)
	      for(int x2=x0; x2<=x1; x2++)
		{
		  int idx = z2*leny*lenx + y2*lenx + x2;
		  if (bitcopy.testBit(idx))
		    {
		      bitcopy.clearBit(idx);
		      Vec po = Vec(x2, y2, z2);
		      posStack.push(po);
		    }
		}
	} // while (!posStack.isEmpty())
    } // while (!done)
  
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();

  QMessageBox::information(0, "", QString("Number of cells : %1").arg(ncells));

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->	\
    setWindowTitle(QString("Drishti"));
}
