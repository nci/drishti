#include "global.h"
#include "staticfunctions.h"
#include "rawvolume.h"
#include "volumeinformation.h"
#include "xmlheaderfunctions.h"

#include <QFileDialog>
#include <QInputDialog>

QString RawVolume::m_rawFileName = "";
int RawVolume::m_depth=0;
int RawVolume::m_width=0;
int RawVolume::m_height=0;
int RawVolume::m_slabSize=0;
int RawVolume::m_voxelType=0;
VolumeFileManager RawVolume::m_rawFileManager;

void RawVolume::reset()
{
  m_rawFileName = "";
  m_depth = m_width = m_height = m_slabSize = 0;
}

void RawVolume::setDepth(int d) { m_depth = d; }
void RawVolume::setWidth(int w) { m_width = w; }
void RawVolume::setHeight(int h) { m_height = h; }
void RawVolume::setVoxelType(int vt) { m_voxelType = vt; }
void RawVolume::setSlabSize(int ss) { m_slabSize = ss; }

void
RawVolume::setRawFileName()
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      m_rawFileName = "dontask";
      return;
    }

//  if (!m_rawFileName.isEmpty())
//    return;

  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  QString pvlFile = pvlInfo.pvlFile;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume ||
      m_rawFileName == "dontask")
    {
      m_rawFileName = pvlFile;
      return;
    }

  QFileInfo pf(pvlFile);
  QFileInfo rf(pf.absolutePath(), pvlInfo.rawFile);


  int headerSize = XmlHeaderFunctions::getRawHeadersizeFromHeader(pvlFile);
  QStringList rawnames = XmlHeaderFunctions::getRawNamesFromHeader(pvlFile);
  if (rawnames.count() > 0 && rawnames[0] != "dontask")
    m_rawFileManager.setFilenameList(rawnames);
  m_voxelType = pvlInfo.voxelType;
  m_rawFileManager.setBaseFilename(rf.absoluteFilePath());
  m_rawFileManager.setDepth(m_depth);
  m_rawFileManager.setWidth(m_width);
  m_rawFileManager.setHeight(m_height);
  m_rawFileManager.setVoxelType(m_voxelType);
  m_rawFileManager.setHeaderSize(headerSize); // default is 13 bytes
  m_rawFileManager.setSlabSize(m_slabSize);

  if (pvlInfo.rawFile == "dontask" || rawnames.count() == 0)
    {
      QStringList pvlnames = XmlHeaderFunctions::getPvlNamesFromHeader(pvlFile);
      if (pvlnames.count() > 0)
	m_rawFileManager.setFilenameList(pvlnames);

      m_rawFileManager.setBaseFilename(pf.absoluteFilePath());
      if (Global::pvlVoxelType() == 0)
	{
	  m_rawFileManager.setVoxelType(VolumeInformation::_UChar);
	  m_voxelType = VolumeInformation::_UChar;
	}
      else
	{
	  m_rawFileManager.setVoxelType(VolumeInformation::_UShort);
	  m_voxelType = VolumeInformation::_UShort;
	}


      int headerSize = XmlHeaderFunctions::getPvlHeadersizeFromHeader(pvlFile);
      m_rawFileManager.setHeaderSize(headerSize); // default is 13 bytes

      m_rawFileName = pvlFile;
      return;
    }
  
  return;
}

QVariant
RawVolume::rawValue(Vec pos)
{
  if (m_rawFileName == "dontask")
    return QVariant(0);

  Vec voxelScaling = Global::voxelScaling();
  int i = pos.x/voxelScaling.x;
  int j = pos.y/voxelScaling.y;
  int k = pos.z/voxelScaling.z;

  return rawValue(i,j,k);
}

QVariant
RawVolume::tagValue(Vec pos)
{
  return QVariant(0);
}

QVariant
RawVolume::rawValue(int h, int w, int d)
{
  setRawFileName();

  if (m_rawFileName == "dontask")
    return QVariant("NoRawFile");

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return QVariant("OutOfBounds");

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {      
      return QVariant("rgb");
    }

  uchar *va = m_rawFileManager.rawValue(d, w, h);
  if (!va)
    return QVariant("OutOfBounds");

  QVariant v;

  if (m_voxelType == VolumeInformation::_UChar ||
      m_voxelType == VolumeInformation::_UShort)
    {
      uint *a = (uint*)va;
      v = QVariant(*a);
    }
  else if (m_voxelType == VolumeInformation::_Char ||
	   m_voxelType == VolumeInformation::_Short ||
	   m_voxelType == VolumeInformation::_Int)
    {
      int *a = (int*)va;
      v = QVariant(*a);
    }
  else if (m_voxelType == VolumeInformation::_Float)
    {
      double *a = (double*)va;
      v = QVariant(*a);
    }
 
  return v;
}

QVariant
RawVolume::tagValue(int i, int j, int k)
{
  return QVariant(0);
}

QList<QVariant>
RawVolume::rawValues(QList<Vec> pos)
{
  QList<QVariant> raw;
  raw.clear();

  setRawFileName();
  if (m_rawFileName != "dontask")
    {
      // now start extracting the raw values
      for(int pi=0; pi<pos.size(); pi++)
	raw.append(rawValue(pos[pi]));
    }

  return raw;
}

QList<QVariant>
RawVolume::tagValues(QList<Vec> pos)
{
  QList<QVariant> tag;
  tag.clear();

  // now start extracting the tag values
  for(int pi=0; pi<pos.size(); pi++)
    tag.append(tagValue(pos[pi]));

  return tag;
}

QMap<QString, QList<QVariant> >
RawVolume::rawValues(int radius,
		     QList<Vec> pos)
{
  setRawFileName();

  QList<QVariant> rawMin;
  QList<QVariant> raw;
  QList<QVariant> rawMax;

  QMap<QString, QList<QVariant> > valueMap;

  if (m_rawFileName == "dontask")
    {
      raw.append(QVariant("NoRawFile"));
      rawMin.append(QVariant("NoRawFile"));
      rawMax.append(QVariant("NoRawFile"));

      valueMap["raw"] = raw;
      valueMap["rawMin"] = rawMin;
      valueMap["rawMax"] = rawMax;

      return valueMap;
    }

  Vec voxelScaling = Global::voxelScaling();
  int radiush = radius/voxelScaling.x;
  int radiusw = radius/voxelScaling.y;
  int radiusd = radius/voxelScaling.z;

  QProgressDialog progress("Generating profile data",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  QHash<qint64, double> hashTable;

  // now start extracting the raw values
  for(int pi=0; pi<pos.size(); pi++)
    {
      progress.setValue((int)(100.0*(float)pi/(float)pos.size()));
      qApp->processEvents();

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
	  double minVal, maxVal;
	  for(int vi=imin; vi<=imax; vi++)
	  for(int vj=jmin; vj<=jmax; vj++)
	  for(int vk=kmin; vk<=kmax; vk++)
	    {
	      navg++;
	      
	      qint64 idx = vk*m_width*m_height +
		           vj*m_height + vi;
	      double val;
	      if (hashTable.contains(idx))
		{
		  val = hashTable[idx];
		}
	      else
		{
		  uchar *va = m_rawFileManager.rawValue(vk, vj, vi);

		  if (m_voxelType == VolumeInformation::_UChar ||
		      m_voxelType == VolumeInformation::_UShort)
		    {
		      uint *a = (uint*)va;
		      val = *a;
		    }
		  else if (m_voxelType == VolumeInformation::_Char ||
			   m_voxelType == VolumeInformation::_Short ||
			   m_voxelType == VolumeInformation::_Int)
		    {
		      int *a = (int*)va;
		      val = *a;
		    }
		  else if (m_voxelType == VolumeInformation::_Float)
		    {
		      double *a = (double*)va;
		      val = *a;
		    }
		  hashTable[idx] = val;
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
	  if (m_voxelType == VolumeInformation::_UChar ||
	      m_voxelType == VolumeInformation::_UShort)
	    {
	      v = QVariant((uint)avg);
	      vmin = QVariant((uint)minVal);
	      vmax = QVariant((uint)maxVal);
	    }
	  else if (m_voxelType == VolumeInformation::_Char ||
		   m_voxelType == VolumeInformation::_Short ||
		   m_voxelType == VolumeInformation::_Int)
	    {
	      v = QVariant((int)avg);
	      vmin = QVariant((int)minVal);
	      vmax = QVariant((int)maxVal);
	    }
	  else
	    {
	      v = QVariant((double)avg);
	      vmin = QVariant((double)minVal);
	      vmax = QVariant((double)maxVal);
	    }

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

void
RawVolume::maskRawVolume(int minx, int maxx,
			 int miny, int maxy,
			 int minz, int maxz,
			 QBitArray bitmask)
{
  setRawFileName();

  if (m_rawFileName == "dontask")
    {
      QMessageBox::information(0, "Save masked raw volume",
			       QString("Raw file to mask ( %1 ) not found").arg(m_rawFileName));
      
      return;
    }

  QString maskrawFile;
  maskrawFile= QFileDialog::getSaveFileName(0,
					"Save masked raw volume",
					Global::previousDirectory(),
					"RAW Files (*.raw)");

  if (maskrawFile.isEmpty())
    return;

  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  uchar vtype = pvlInfo.voxelType;
  int skipheaderbytes = pvlInfo.skipheaderbytes;
  Vec grid = pvlInfo.dimensions;
  int width = grid.y;
  int height = grid.z;

  int nx = maxx-minx+1;
  int ny = maxy-miny+1;
  int nz = maxz-minz+1;

  int bpv = 1;
  if (vtype == VolumeInformation::_UChar) bpv = 1;
  else if (vtype == VolumeInformation::_Char) bpv = 1;
  else if (vtype == VolumeInformation::_UShort) bpv = 2;
  else if (vtype == VolumeInformation::_Short) bpv = 2;
  else if (vtype == VolumeInformation::_Int) bpv = 4;
  else if (vtype == VolumeInformation::_Float) bpv = 4;

  int nbytes = width*height*bpv;
  uchar *raw = new uchar[nbytes];
  uchar *maskraw = new uchar[ny*nx*bpv];

  
  //---- max 1Gb per slab
  int slabSize;
  slabSize = (1024*1024*1024)/(ny*nx*bpv);
  //----
  VolumeFileManager maskRawFileManager;
  maskRawFileManager.setBaseFilename(maskrawFile);
  maskRawFileManager.setDepth(nz);
  maskRawFileManager.setWidth(ny);
  maskRawFileManager.setHeight(nx);
  maskRawFileManager.setVoxelType(vtype);  
  maskRawFileManager.setHeaderSize(13);
  maskRawFileManager.setSlabSize(slabSize);
  maskRawFileManager.createFile(true);

  float maskval = 0;
  maskval = (float) QInputDialog::getDouble(0, "Voxel value in transparent region",
					    "Voxel value", 0);

  QProgressDialog progress("Saving masked raw volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);
  
  int bidx = 0;
  for(int k=minz; k<=maxz; k++)
    {
      progress.setValue((int)(100.0*(float)(k-minz)/(float)nz));
      qApp->processEvents();

      uchar *slice = m_rawFileManager.getSlice(k);
      memcpy(raw, slice, nbytes);

      memset(maskraw, 0, ny*nx*bpv);
      if (fabs(maskval) > 0)
	{
	  if (vtype == VolumeInformation::_UChar)
	    {
	      uchar *mr = maskraw;
	      for(int i=0; i<ny*nx; i++) mr[i] = maskval;
	    }
	  else if (vtype == VolumeInformation::_Char)
	    {
	      char *mr = (char*)maskraw;
	      for(int i=0; i<ny*nx; i++) mr[i] = maskval;
	    }
	  else if (vtype == VolumeInformation::_UShort)
	    {
	      ushort *mr = (ushort*)maskraw;
	      for(int i=0; i<ny*nx; i++) mr[i] = maskval;
	    }
	  else if (vtype == VolumeInformation::_Short)
	    {
	      short *mr = (short*)maskraw;
	      for(int i=0; i<ny*nx; i++) mr[i] = maskval;
	    }
	  else if (vtype == VolumeInformation::_Int)
	    {
	      int *mr = (int*)maskraw;
	      for(int i=0; i<ny*nx; i++) mr[i] = maskval;
	    }
	  else if (vtype == VolumeInformation::_Float)
	    {
	      float *mr = (float*)maskraw;
	      for(int i=0; i<ny*nx; i++) mr[i] = maskval;
	    }
	}
	
      int idx=0;
      for(int y=miny; y<=maxy; y++)
	for(int x=minx; x<=maxx; x++)
	  {
	    if (bitmask.testBit(bidx))
	      {
		int ryx = y*height+x;

		if(bpv == 1)
		  *(maskraw + idx) = *(raw + ryx);
		else if(bpv == 2)
		  {
		    *(maskraw + 2*idx+0) = *(raw + 2*ryx);
		    *(maskraw + 2*idx+1) = *(raw + 2*ryx+1);
		  }
		else if(bpv == 4)
		  {
		    *(maskraw + 4*idx+0) = *(raw + 4*ryx);
		    *(maskraw + 4*idx+1) = *(raw + 4*ryx+1);
		    *(maskraw + 4*idx+2) = *(raw + 4*ryx+2);
		    *(maskraw + 4*idx+3) = *(raw + 4*ryx+3);
		  }
	      }
	    idx++;
	    bidx++;
	  }      
      maskRawFileManager.setSlice(k-minz, maskraw);
    }

  delete [] raw;
  delete [] maskraw;

  progress.setValue(100);

  QMessageBox::information(0, "Save masked raw volume",
			   QString("Saved masked raw volume to")+maskrawFile);
}

#define ReadVoxel(T)						\
{								\
  T pval;							\
  fin.read((char*)&pval, bpv);					\
  f[xv][yv][zv]=pval;						\
}


void
RawVolume::extractPath(QList<Vec> points,
		       QList<Vec> pathPoints,
		       QList<float> pathAngle,
		       int rads, int radt,
		       bool nearest,
		       Vec bmin, Vec bmax, QBitArray bitmask)
{
  QMessageBox::information(0, "Error", "Sorry !! Not implemented.\nPlease contact Ajay.Limaye@anu.edu.au if you would like to use this feature.");
  return;

  setRawFileName();

  if (m_rawFileName == "dontask")
    {
      QMessageBox::information(0, "Extract path raw volume",
			       QString("Raw file ( %1 ) not found").arg(m_rawFileName));     
      return;
    }

  QString rawFile;
  rawFile= QFileDialog::getSaveFileName(0,
					"Save path raw volume",
					Global::previousDirectory(),
					"RAW Files (*.raw)");

  if (rawFile.isEmpty())
    return;

  fstream fin((char *)m_rawFileName.toLatin1().data(),
	      ios::in|ios::binary);

  if (fin.fail())
    {
      QMessageBox::information(0, "",
		      QString("Cannot open : ")+m_rawFileName);
      return;
    }


  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  int vtype = pvlInfo.voxelType;
  int skipheaderbytes = pvlInfo.skipheaderbytes;
  Vec grid = pvlInfo.dimensions;
  int depth = grid.x;
  int width = grid.y;
  int height = grid.z;

  int bpv = 1;
  if (vtype == VolumeInformation::_UChar) bpv = 1;
  else if (vtype == VolumeInformation::_Char) bpv = 1;
  else if (vtype == VolumeInformation::_UShort) bpv = 2;
  else if (vtype == VolumeInformation::_Short) bpv = 2;
  else if (vtype == VolumeInformation::_Int) bpv = 4;
  else if (vtype == VolumeInformation::_Float) bpv = 4;

  int nZ = 0;
  int nY = 2*radt+1;
  int nX = 2*rads+1;

  fstream fout((char *)rawFile.toLatin1().data(),
	      ios::out|ios::binary);

  fout.write((char*)&vtype, 1);
  fout.write((char*)&nZ, sizeof(int));
  fout.write((char*)&nY, sizeof(int));
  fout.write((char*)&nX, sizeof(int));

  uchar *pathraw = new uchar[nY*nX*bpv];


  int minx = bmin.x;
  int miny = bmin.y;
  int minz = bmin.z;
  int maxx = bmax.x;
  int maxy = bmax.y;
  int maxz = bmax.z;
  int bnx = maxx-minx+1;
  int bny = maxy-miny+1;
  int bnz = maxz-minz+1;

  float maskval = 0;
  maskval = (float) QInputDialog::getDouble(0, "Voxel value in transparent region",
					    "Voxel value", 0);

  QProgressDialog progress("Extracting raw volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);
  

  int npaths = pathPoints.count();

  Vec ptang, pxaxis;
  ptang = Vec(0,0,1);
  pxaxis = Vec(1,0,0);
  for(int ip=0; ip<npaths-1; ip++)
    {
      progress.setValue((int)(100*(float)ip/(float)npaths));
      qApp->processEvents();
      
      Vec tang, xaxis, yaxis;

      if (ip== 0)
	tang = points[1]-points[0];
      else if (ip== npaths-1)
	tang = pathPoints[ip]-pathPoints[ip-1];
      else
	tang = pathPoints[ip+1]-pathPoints[ip-1];

      if (tang.norm() > 0)
	tang.normalize();
      else
	tang = Vec(0,0,1); // should really scold the user


      //----------------
      Vec axis;
      float angle;      
      StaticFunctions::getRotationBetweenVectors(ptang, tang, axis, angle);
      if (qAbs(angle) > 0.0 && qAbs(angle) < 3.1415)
	{
	  Quaternion q(axis, angle);	  
	  xaxis = q.rotate(pxaxis);
	}
      else
	xaxis = pxaxis;

      //apply offset rotation
      angle = pathAngle[ip];
      if (ip > 0) angle = pathAngle[ip]-pathAngle[ip-1];
      Quaternion q = Quaternion(tang, angle);
      xaxis = q.rotate(xaxis);

      yaxis = tang^xaxis;

      pxaxis = xaxis;
      ptang = tang;
      //----------------

      Vec v0 = pathPoints[ip];
      Vec v1 = pathPoints[ip+1];

      QList<Vec>voxels = StaticFunctions::voxelizeLine(v0, v1);
      
      int st = 0;
      if (ip > 0) st = 1;

      int vcount = voxels.count();
      for(int iv=st; iv<vcount; iv++)
	{
	  Vec pos = voxels[iv];
	  float angle = (pathAngle[ip+1]-pathAngle[ip])*((float)iv/(float)(vcount-1));
	  
	  Vec vxaxis = xaxis;
	  if (fabs(angle) > 0.0f)
	    {
	      Quaternion q = Quaternion(tang, angle);
	      vxaxis = q.rotate(xaxis);
	    }
	  Vec vyaxis = tang^vxaxis;

	  memset(pathraw, 0, bpv*nY*nX);
	  if (fabs(maskval) > 0)
	    {
	      if (vtype == VolumeInformation::_UChar)
		{
		  uchar *mr = pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_Char)
		{
		  char *mr = (char*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_UShort)
		{
		  ushort *mr = (ushort*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_Short)
		{
		  short *mr = (short*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_Int)
		{
		  int *mr = (int*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_Float)
		{
		  float *mr = (float*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	    }

	  int idx = 0;
	  for (int iy=-radt; iy<=radt; iy++)
	    for (int ix=-rads; ix<=rads; ix++)
	      {
		Vec vox = pos + iy*vyaxis + ix*vxaxis;
		int x = vox.x;
		int y = vox.y;
		int z = vox.z;
		
		bool ok = false;
		if (x >= minx && x <= maxx &&
		    y >= miny && y <= maxy &&
		    z >= minz && z <= maxz)
		  {
		    int bidx = (z-minz)*bny*bnx +
		      (y-miny)*bnx + (x-minx);
		    if (bitmask.testBit(bidx))
		      ok = true;
		  }
		    
		if (ok && nearest) // nearest neighbour interpolation
		  {
		    if (z>=0 && z<depth &&
			y>=0 && y<width &&
			x>=0 && x<height)
		      {
			fin.seekg(skipheaderbytes +
				  bpv*(z*width*height +
				       y*height +
				       x),
				  ios::beg);
			fin.read((char*)(pathraw+idx), bpv);
		      }
		  }
		else if (ok) // linear interpolation
		  {
		    int x1 = x+1;
		    int y1 = y+1;
		    int z1 = z+1;
		    
		    if (z>=0 && z<depth  &&  z1>=0 && z1<depth &&
			y>=0 && y<width  &&  y1>=0 && y1<width &&
			x>=0 && x<height && x1>=0 && x1<height)
		      {
			int xid[2], yid[2],zid[2];
			xid[0] = x; xid[1] = x1;
			yid[0] = y; yid[1] = y1;
			zid[0] = z; zid[1] = z1;
			float f[2][2][2];
			for(int zv=0; zv<2; zv++)
			  for(int yv=0; yv<2; yv++)
			    for(int xv=0; xv<2; xv++)
			      {
				fin.seekg(skipheaderbytes +
					  bpv*(zid[zv]*width*height +
					       yid[yv]*height +
					       xid[xv]),
					  ios::beg);
				
				if (vtype == VolumeInformation::_UChar)
				  ReadVoxel(uchar)
				else if (vtype == VolumeInformation::_Char)
				  ReadVoxel(char)
				else if (vtype == VolumeInformation::_UShort)
				  ReadVoxel(ushort)
				else if (vtype == VolumeInformation::_Short)
				  ReadVoxel(short)
				else if (vtype == VolumeInformation::_Int)
				  ReadVoxel(int)
				else if (vtype == VolumeInformation::_Float)
				  ReadVoxel(float)
				}

			// linear interpolation
			float dx = vox.x-x;
			float dy = vox.y-y;
			float dz = vox.z-z;
			float val = (         dx*dy*dz*f[1][1][1] +
				          dx*(1-dy)*dz*f[1][0][1] +
				          (1-dx)*dy*dz*f[0][1][1] +
				      (1-dx)*(1-dy)*dz*f[0][0][1] +
				  (1-dx)*(1-dy)*(1-dz)*f[0][0][0] +
				      dx*(1-dy)*(1-dz)*f[1][0][0] +
				      (1-dx)*dy*(1-dz)*f[0][1][0] +
					  dx*dy*(1-dz)*f[1][1][0] );
			
			if (vtype == VolumeInformation::_UChar)
			  ((uchar*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_Char)
			  ((char*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_UShort)
			  ((ushort*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_Short)
			  ((short*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_Int)
			  ((int*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_Float)
			  ((float*) pathraw)[idx] = val;
		      }
		  }

		if (nearest)
		  idx += bpv;		
		else
		  idx ++;
	      } // loop over x and y

	  nZ++;
	  fout.write((char*)pathraw, bpv*nY*nX);
	}
    }

  fout.seekg(1, ios::beg);
  fout.write((char*)&nZ, sizeof(int));

  fin.close();
  fout.close();

  progress.setValue(100);

  QMessageBox::information(0, "Save extracted raw volume",
			   QString("Saved extracted raw volume to")+rawFile);
}

void
RawVolume::extractPatch(QList<Vec> points,
			QList<Vec> pathPoints,
			QList<float> pathAngle,
			int rads, int radt,
			bool nearest,
			bool dohalf, int godeep,
			Vec bmin, Vec bmax, QBitArray bitmask)
{
  QMessageBox::information(0, "Error", "Sorry !! Not implemented.\nPlease contact Ajay.Limaye@anu.edu.au if you would like to use this feature.");
  return;

  setRawFileName();

  if (m_rawFileName == "dontask")
    {
      QMessageBox::information(0, "Extract path raw volume",
			       QString("Raw file ( %1 ) not found").arg(m_rawFileName));     
      return;
    }

  QString rawFile;
  rawFile= QFileDialog::getSaveFileName(0,
					"Save path raw volume",
					Global::previousDirectory(),
					"RAW Files (*.raw)");

  if (rawFile.isEmpty())
    return;

  fstream fin((char *)m_rawFileName.toLatin1().data(),
	      ios::in|ios::binary);

  if (fin.fail())
    {
      QMessageBox::information(0, "",
		      QString("Cannot open : ")+m_rawFileName);
      return;
    }


  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  int vtype = pvlInfo.voxelType;
  int skipheaderbytes = pvlInfo.skipheaderbytes;
  Vec grid = pvlInfo.dimensions;
  int depth = grid.x;
  int width = grid.y;
  int height = grid.z;

  int bpv = 1;
  if (vtype == VolumeInformation::_UChar) bpv = 1;
  else if (vtype == VolumeInformation::_Char) bpv = 1;
  else if (vtype == VolumeInformation::_UShort) bpv = 2;
  else if (vtype == VolumeInformation::_Short) bpv = 2;
  else if (vtype == VolumeInformation::_Int) bpv = 4;
  else if (vtype == VolumeInformation::_Float) bpv = 4;

  int nZ = 0;
  // perimeter of ellipse
  int nY = 3.14159265*(3*(rads+radt) -
		       sqrt((float)(3*rads+radt)*(3*radt+rads)));
  if (dohalf)
    nY /= 2; // take only half ellipse
  // go deep along the patch notmals
  int nX = godeep;

  fstream fout((char *)rawFile.toLatin1().data(),
	      ios::out|ios::binary);

  fout.write((char*)&vtype, 1);
  fout.write((char*)&nZ, sizeof(int));
  fout.write((char*)&nY, sizeof(int));
  fout.write((char*)&nX, sizeof(int));

  uchar *pathraw = new uchar[nY*nX*bpv];


  int minx = bmin.x;
  int miny = bmin.y;
  int minz = bmin.z;
  int maxx = bmax.x;
  int maxy = bmax.y;
  int maxz = bmax.z;
  int bnx = maxx-minx+1;
  int bny = maxy-miny+1;
  int bnz = maxz-minz+1;

  float maskval = 0;
  maskval = (float) QInputDialog::getDouble(0, "Voxel value in transparent region",
					    "Voxel value", 0);

  QProgressDialog progress("Extracting raw volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);
  

  int npaths = pathPoints.count();

  Vec ptang, pxaxis;
  ptang = Vec(0,0,1);
  pxaxis = Vec(1,0,0);
  for(int ip=0; ip<npaths-1; ip++)
    {
      progress.setValue((int)(100*(float)ip/(float)npaths));
      qApp->processEvents();
      
      Vec tang, xaxis, yaxis;

      if (ip== 0)
	tang = points[1]-points[0];
      else if (ip== npaths-1)
	tang = pathPoints[ip]-pathPoints[ip-1];
      else
	tang = pathPoints[ip+1]-pathPoints[ip-1];

      if (tang.norm() > 0)
	tang.normalize();
      else
	tang = Vec(0,0,1); // should really scold the user


      //----------------
      Vec axis;
      float angle;      
      StaticFunctions::getRotationBetweenVectors(ptang, tang, axis, angle);
      if (qAbs(angle) > 0.0 && qAbs(angle) < 3.1415)
	{
	  Quaternion q(axis, angle);	  
	  xaxis = q.rotate(pxaxis);
	}
      else
	xaxis = pxaxis;

      //apply offset rotation
      angle = pathAngle[ip];
      if (ip > 0) angle = pathAngle[ip]-pathAngle[ip-1];
      Quaternion q = Quaternion(tang, angle);
      xaxis = q.rotate(xaxis);

      yaxis = tang^xaxis;

      pxaxis = xaxis;
      ptang = tang;
      //----------------

      Vec v0 = pathPoints[ip];
      Vec v1 = pathPoints[ip+1];

      QList<Vec>voxels = StaticFunctions::voxelizeLine(v0, v1);
      
      int st = 0;
      if (ip > 0) st = 1;

      int vcount = voxels.count();
      for(int iv=st; iv<vcount; iv++)
	{
	  Vec pos = voxels[iv];
	  float angle = (pathAngle[ip+1]-pathAngle[ip])*((float)iv/(float)(vcount-1));
	  Quaternion q = Quaternion(tang, angle);
	  Vec vxaxis = q.rotate(xaxis);
	  Vec vyaxis = tang^vxaxis;

	  memset(pathraw, 0, bpv*nY*nX);
	  if (fabs(maskval) > 0)
	    {
	      if (vtype == VolumeInformation::_UChar)
		{
		  uchar *mr = pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_Char)
		{
		  char *mr = (char*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_UShort)
		{
		  ushort *mr = (ushort*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_Short)
		{
		  short *mr = (short*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_Int)
		{
		  int *mr = (int*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	      else if (vtype == VolumeInformation::_Float)
		{
		  float *mr = (float*)pathraw;
		  for(int i=0; i<nY*nX; i++) mr[i] = maskval;
		}
	    }


	  int idx = 0;
	  for (int iy=0; iy<nY; iy++)
	    {
	      float a = (float)iy/(float)(nY-1);
	      if (dohalf)
		a*=3.14159265f;
	      else
		a*=6.2831853f;
	      float s = rads*cos(a);
	      float t = radt*sin(a);
	      Vec p0 = pos + s*xaxis + t*yaxis;
	      Vec dir = (2*s/(rads*rads))*xaxis +
		        (2*t/(radt*radt))*yaxis;
	      dir.normalize();
	      for (int ix=0; ix<nX; ix++)
	      {
		Vec vox = p0 - ix*dir;
		int x = vox.x;
		int y = vox.y;
		int z = vox.z;

		bool ok = false;
		if (x >= minx && x <= maxx &&
		    y >= miny && y <= maxy &&
		    z >= minz && z <= maxz)
		  {
		    int bidx = (z-minz)*bny*bnx +
		      (y-miny)*bnx + (x-minx);
		    if (bitmask.testBit(bidx))
		      ok = true;
		  }
		    
		if (ok && nearest) // nearest neighbour interpolation
		  {
		    if (z>=0 && z<depth &&
			y>=0 && y<width &&
			x>=0 && x<height)
		      {
			fin.seekg(skipheaderbytes +
				  bpv*(z*width*height +
				       y*height +
				       x),
				  ios::beg);
			fin.read((char*)(pathraw+idx), bpv);
		      }
		  }
		else if (ok) // linear interpolation
		  {
		    int x1 = x+1;
		    int y1 = y+1;
		    int z1 = z+1;
		    
		    if (z>=0 && z<depth  &&  z1>=0 && z1<depth &&
			y>=0 && y<width  &&  y1>=0 && y1<width &&
			x>=0 && x<height && x1>=0 && x1<height)
		      {
			int xid[2], yid[2],zid[2];
			xid[0] = x; xid[1] = x1;
			yid[0] = y; yid[1] = y1;
			zid[0] = z; zid[1] = z1;
			float f[2][2][2];
			for(int zv=0; zv<2; zv++)
			  for(int yv=0; yv<2; yv++)
			    for(int xv=0; xv<2; xv++)
			      {
				fin.seekg(skipheaderbytes +
					  bpv*(zid[zv]*width*height +
					       yid[yv]*height +
					       xid[xv]),
					  ios::beg);
				
				if (vtype == VolumeInformation::_UChar)
				  ReadVoxel(uchar)
				else if (vtype == VolumeInformation::_Char)
				  ReadVoxel(char)
				else if (vtype == VolumeInformation::_UShort)
				  ReadVoxel(ushort)
				else if (vtype == VolumeInformation::_Short)
				  ReadVoxel(short)
				else if (vtype == VolumeInformation::_Int)
				  ReadVoxel(int)
				else if (vtype == VolumeInformation::_Float)
				  ReadVoxel(float)
				}

			// linear interpolation
			float dx = vox.x-x;
			float dy = vox.y-y;
			float dz = vox.z-z;
			float val = (         dx*dy*dz*f[1][1][1] +
				          dx*(1-dy)*dz*f[1][0][1] +
				          (1-dx)*dy*dz*f[0][1][1] +
				      (1-dx)*(1-dy)*dz*f[0][0][1] +
				  (1-dx)*(1-dy)*(1-dz)*f[0][0][0] +
				      dx*(1-dy)*(1-dz)*f[1][0][0] +
				      (1-dx)*dy*(1-dz)*f[0][1][0] +
					  dx*dy*(1-dz)*f[1][1][0] );
			
			if (vtype == VolumeInformation::_UChar)
			  ((uchar*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_Char)
			  ((char*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_UShort)
			  ((ushort*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_Short)
			  ((short*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_Int)
			  ((int*) pathraw)[idx] = val;
			else if (vtype == VolumeInformation::_Float)
			  ((float*) pathraw)[idx] = val;
		      }
		  }

		if (nearest)
		  idx += bpv;		
		else
		  idx ++;

	      } // loop over x
	    } // loop over y
	  nZ++;
	  fout.write((char*)pathraw, bpv*nY*nX);
	}
    }

  fout.seekg(1, ios::beg);
  fout.write((char*)&nZ, sizeof(int));

  fin.close();
  fout.close();

  progress.setValue(100);

  QMessageBox::information(0, "Save extracted raw volume",
			   QString("Saved extracted raw volume to")+rawFile);
}

