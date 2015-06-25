#include "volumemask.h"
#include "global.h"
#include <QDomDocument>

VolumeMask::VolumeMask()
{
  m_maskfile.clear();
  m_maskslice = 0;
  m_depth = m_width = m_height = 0;
  m_bitmask.clear();
}

VolumeMask::~VolumeMask()
{
  reset();
}

void
VolumeMask::reset()
{
  if (!m_maskfile.isEmpty())
    m_maskFileManager.saveMemFile();    

  m_maskFileManager.reset();

  m_maskfile.clear();
  if (m_maskslice) delete [] m_maskslice;
  m_maskslice = 0;
  m_depth = m_width = m_height = 0;

  m_bitmask.clear();
}

void
VolumeMask::saveIntermediateResults()
{
  m_maskFileManager.saveMemFile();
}

void
VolumeMask::setFile(QString mfile, bool inMem)
{
  reset();
  m_maskfile = mfile;
  //m_maskFileManager.setBaseFilename(mfile);
  QStringList tflnms;
  tflnms << mfile;
  m_maskFileManager.setFilenameList(tflnms);
  m_maskFileManager.setMemMapped(inMem);
}

void
VolumeMask::setGridSize(int d, int w, int h, int slabsize)
{
  m_depth = d;
  m_width = w;
  m_height= h;  
  
  m_bitmask.resize(m_depth*m_width*m_height);

  m_maskFileManager.setDepth(m_depth);
  m_maskFileManager.setWidth(m_width);
  m_maskFileManager.setHeight(m_height);
  m_maskFileManager.setHeaderSize(13);
  //m_maskFileManager.setSlabSize(slabsize);
  // do not split data across multiple files
  m_maskFileManager.setSlabSize(m_depth+1);

  if (m_maskFileManager.exists())
    m_maskFileManager.loadMemFile();
  else
    checkMaskFile();
}

void
VolumeMask::checkMaskFile()
{
  // create mask file if not present
  if (!m_maskFileManager.exists())
    {
      m_maskFileManager.createFile(true, true);

      QDomDocument doc("Drishti_Header");
      
      QDomElement topElement = doc.createElement("PvlDotNcFileHeader");
      doc.appendChild(topElement);
      
      {      
	QDomElement de0 = doc.createElement("rawfile");
	QDomText tn0;
	tn0 = doc.createTextNode("");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("pvlnames");
	QDomText tn0;
	QFileInfo fileInfo(m_maskfile);
	QDir direc = fileInfo.absoluteDir();
	QString vstr = direc.relativeFilePath(m_maskfile);
	tn0 = doc.createTextNode(vstr);
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("description");
	QDomText tn0;
	tn0 = doc.createTextNode("");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("voxeltype");
	QDomText tn0;
	tn0 = doc.createTextNode("unsigned char");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("voxelunit");
	QDomText tn0;
	tn0 = doc.createTextNode("no units");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("voxelsize");
	QDomText tn0;
	tn0 = doc.createTextNode("1 1 1");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("gridsize");
	QDomText tn0;
	tn0 = doc.createTextNode(QString("%1 %2 %3").arg(m_depth).arg(m_width).arg(m_height));
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("slabsize");
	QDomText tn0;
	tn0 = doc.createTextNode(QString("%1").arg(m_depth+1));
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("rawmap");
	QDomText tn0;
	tn0 = doc.createTextNode("0 255");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("pvlmap");
	QDomText tn0;
	tn0 = doc.createTextNode("0 255");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      
      QString pvlfile = m_maskfile;
      //pvlfile.chop(4);
      pvlfile += ".pvl.nc";
      QFile pf(pvlfile.toLatin1().data());
      if (pf.open(QIODevice::WriteOnly))
	{
	  QTextStream out(&pf);
	  doc.save(out, 2);
	  pf.close();
	}
      
    }
} 

void
VolumeMask::setMaskDepthSlice(int slc, uchar* tagData)
{
  checkMaskFile();
  m_maskFileManager.setSliceMem(slc, tagData);
}

uchar*
VolumeMask::getMaskDepthSliceImage(int slc)
{
  checkMaskFile();

  if (m_maskslice) delete [] m_maskslice;

  int nbytes = m_width*m_height;
  m_maskslice = new uchar[nbytes];

  uchar *mslice = m_maskFileManager.getSliceMem(slc);
  memcpy(m_maskslice, mslice, nbytes);

  return m_maskslice;
}

uchar*
VolumeMask::getMaskWidthSliceImage(int slc)
{
  checkMaskFile();

  if (m_maskslice) delete [] m_maskslice;

  int nbytes = m_depth*m_height;
  m_maskslice = new uchar[nbytes];

  uchar *mslice = m_maskFileManager.getWidthSliceMem(slc);
  memcpy(m_maskslice, mslice, nbytes);

  return m_maskslice;
}

uchar*
VolumeMask::getMaskHeightSliceImage(int slc)
{
  checkMaskFile();

  if (m_maskslice) delete [] m_maskslice;

  int nbytes = m_depth*m_width;
  m_maskslice = new uchar[nbytes];

  uchar *mslice = m_maskFileManager.getHeightSliceMem(slc);
  memcpy(m_maskslice, mslice, nbytes);

  return m_maskslice;
}

uchar
VolumeMask::maskValue(int d, int w, int h)
{
  checkMaskFile();

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return 0;
  
  uchar tmp = 0;
  uchar *mslice = m_maskFileManager.rawValueMem(d, w, h);
  if (mslice)
    tmp = mslice[0];

  return tmp;
}

void
VolumeMask::tagDSlice(int d, QBitArray bitmask, uchar *usermask)
{
  checkMaskFile();

  int nbytes = m_width*m_height;
  uchar *mask = new uchar[nbytes];
  int tag = Global::tag();

  uchar *mslice = m_maskFileManager.getSliceMem(d);      
  memcpy(mask, mslice, nbytes);

  qint64 idx = 0;
  for(int w=0; w<m_width; w++)
    for(int h=0; h<m_height; h++)
      {
	qint64 midx = d*m_width*m_height + w*m_height + h;
	if (bitmask.testBit(midx))
	  {
	    if (usermask[4*idx] > 0)
	      mask[idx] = tag;
	  }
	idx ++;
      }
  
  m_maskFileManager.setSliceMem(d, mask);
  delete [] mask;
}

void
VolumeMask::tagWSlice(int w, QBitArray bitmask, uchar *usermask)
{
  checkMaskFile();

  int nbytes = m_width*m_height;
  uchar *mask = new uchar[nbytes];
  int tag = Global::tag();
  qint64 bidx = 0;

  for(int d=0; d<m_depth; d++)
    {
      uchar *mslice = m_maskFileManager.getSliceMem(d);      
      memcpy(mask, mslice, nbytes);
      for(int h=0; h<m_height; h++)
	{
	  qint64 midx = d*m_width*m_height + w*m_height + h;
	  if (bitmask.testBit(midx))
	    {
	      qint64 bidx = (d*m_height + h);
	      qint64 idx = (w*m_height + h);
	      if (usermask[4*bidx] > 0)
		mask[idx] = tag;
	    }
	}
  
      m_maskFileManager.setSliceMem(d, mask);
    }

  delete [] mask;
}

void
VolumeMask::tagHSlice(int h, QBitArray bitmask, uchar *usermask)
{
  checkMaskFile();

  int nbytes = m_width*m_height;
  uchar *mask = new uchar[nbytes];
  int tag = Global::tag();
  qint64 bidx = 0;

  for(int d=0; d<m_depth; d++)
    {
      uchar *mslice = m_maskFileManager.getSliceMem(d);      
      memcpy(mask, mslice, nbytes);
      for(int w=0; w<m_width; w++)
	{
	  qint64 midx = d*m_width*m_height + w*m_height + h;
	  if (bitmask.testBit(midx))
	    {
	      qint64 bidx = (d*m_width + w);
	      qint64 idx = (w*m_height + h);
	      if (usermask[4*bidx] > 0)
		mask[idx] = tag;
	    }
	}
  
      m_maskFileManager.setSliceMem(d, mask);
    }

  delete [] mask;
}


void
VolumeMask::tagDSlice(int d, uchar *tags)
{
  checkMaskFile();
  int nbytes = m_width*m_height;
  uchar *mask = new uchar[nbytes];
  memcpy(mask, tags, nbytes);
  m_maskFileManager.setSliceMem(d, mask);
  delete [] mask;
}
void
VolumeMask::tagWSlice(int w, uchar *tags)
{
  checkMaskFile();
  m_maskFileManager.setWidthSliceMem(w, tags);
}
void
VolumeMask::tagHSlice(int h, uchar *tags)
{
  checkMaskFile();
  m_maskFileManager.setHeightSliceMem(h, tags);
}


void
VolumeMask::tagUsingBitmask(QList<int> pos,
			    QBitArray bitmask)
{
  checkMaskFile();

  if (pos.count())
    findConnectedRegion(pos);

//  QProgressDialog progress("Updating Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  int nbytes = m_width*m_height;
  uchar *mask = new uchar[nbytes];
  int tag = Global::tag();

  qint64 bidx = 0;
  for(int d=0; d<m_depth; d++)
    {
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();
      
      uchar *mslice = m_maskFileManager.getSliceMem(d);      
      memcpy(mask, mslice, nbytes);

      for(int w=0; w<m_width; w++)
	for(int h=0; h<m_height; h++)
	  {
	    if (bitmask.testBit(bidx))
	      {
		qint64 idx = (w*m_height + h);
		//if (tag == 0 || mask[idx] == 0)
		mask[idx] = tag;
	      }

	    bidx++;
	  }

      m_maskFileManager.setSliceMem(d, mask);
    }
  
  delete [] mask;

  emit progressReset();
}

void
VolumeMask::dilate(QBitArray vbitmask)
{
  createBitmask();

  //int thickness = Global::spread();
  int thickness = 1;
  while (thickness > 0)
    {
      dilateBitmask();
      thickness --;
    }

  // m_bitmask now contains region dilated by thickness

//  QProgressDialog progress("Updating Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  int nbytes = m_width*m_height;
  unsigned char *mask = new unsigned char[nbytes];

  qint64 bidx = 0;
  for(int d=0; d<m_depth; d++)
    { 
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();
      
      uchar *mslice = m_maskFileManager.getSliceMem(d);      
      memcpy(mask, mslice, nbytes);

      for(int w=0; w<m_width; w++)
	for(int h=0; h<m_height; h++)
	  {
	    if ( vbitmask.testBit(bidx) &&
		m_bitmask.testBit(bidx))
	      {
		qint64 idx = (w*m_height + h);
		mask[idx] = Global::tag();
	      }
	    bidx++;
	  }
 
      m_maskFileManager.setSliceMem(d, mask);
    }

  delete [] mask;

  emit progressReset();
}


void
VolumeMask::dilate(int mind, int maxd,
		   int minw, int maxw,
		   int minh, int maxh,
		   QBitArray vbitmask)
{
  createBitmask();

  //int thickness = Global::spread();
  int thickness = 1;
  while (thickness > 0)
    {
      dilateBitmask(mind, maxd,
		    minw, maxw,
		    minh, maxh);
      thickness --;
    }

  // m_bitmask now contains region dilated by thickness

//  QProgressDialog progress("Updating Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  int nbytes = m_width*m_height;
  unsigned char *mask = new unsigned char[nbytes];

  for(int d=mind; d<=maxd; d++)
    { 
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();
      
      uchar *mslice = m_maskFileManager.getSliceMem(d);      
      memcpy(mask, mslice, nbytes);

      for(int w=minw; w<=maxw; w++)
	for(int h=minh; h<=maxh; h++)
	  {
	    qint64 bidx = (d*m_width*m_height +
			   w*m_height + h);
	    if ( vbitmask.testBit(bidx) &&
		m_bitmask.testBit(bidx))
	      {
		qint64 idx = (w*m_height + h);
		mask[idx] = Global::tag();
	      }
	  }

      m_maskFileManager.setSliceMem(d, mask);
    }

  delete [] mask;

  emit progressReset();
}


void
VolumeMask::erode(QBitArray vbitmask)
{
  createBitmask();

  //int thickness = Global::spread();
  int thickness = 1;
  while (thickness > 0)
    {
      erodeBitmask();
      thickness --;
    }

  // m_bitmask now contains region eroded by thickness

//  QProgressDialog progress("Updating Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  int nbytes = m_width*m_height;
  unsigned char *mask = new unsigned char[nbytes];

  qint64 bidx = 0;
  for(int d=0; d<m_depth; d++)
    { 
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();
      
      uchar *mslice = m_maskFileManager.getSliceMem(d);      
      memcpy(mask, mslice, nbytes);

      for(int w=0; w<m_width; w++)
	for(int h=0; h<m_height; h++)
	  {	    
	    if (  vbitmask.testBit(bidx) &&
		!m_bitmask.testBit(bidx))
	      {
		// reset mask to 0 if the
		// mask value is same as supplied tag value
		qint64 idx = (w*m_height + h);
		if ( mask[idx] == Global::tag())
		  mask[idx] = 0;
	      }

	    bidx++;
	  }

      m_maskFileManager.setSliceMem(d, mask);
    }

  delete [] mask;

  emit progressReset();
}

void
VolumeMask::erode(int mind, int maxd,
		  int minw, int maxw,
		  int minh, int maxh,
		  QBitArray vbitmask)
{
  createBitmask();

  //int thickness = Global::spread();
  int thickness = 1;
  while (thickness > 0)
    {
      erodeBitmask(mind, maxd,
		   minw, maxw,
		   minh, maxh);
      thickness --;
    }

  // m_bitmask now contains region eroded by thickness

//  QProgressDialog progress("Updating Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  int nbytes = m_width*m_height;
  unsigned char *mask = new unsigned char[nbytes];

  for(int d=mind; d<=maxd; d++)
    { 
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();
      
      uchar *mslice = m_maskFileManager.getSliceMem(d);      
      memcpy(mask, mslice, nbytes);

      for(int w=minw; w<=maxw; w++)
	for(int h=minh; h<=maxh; h++)
	  {
	    qint64 bidx = (d*m_width*m_height +
			w*m_height + h);	    
	    if (  vbitmask.testBit(bidx) &&
		!m_bitmask.testBit(bidx))
	      {
		// reset mask to 0 if the
		// mask value is same as supplied tag value
		qint64 idx = (w*m_height + h);
		if ( mask[idx] == Global::tag())
		  mask[idx] = 0;
	      }
	  }

      m_maskFileManager.setSliceMem(d, mask);
    }

  delete [] mask;

  emit progressReset();
}

void
VolumeMask::createBitmask()
{
  checkMaskFile();

  m_bitmask.fill(false);

//  QProgressDialog progress("Identifying Tagged Region",
//			   "Cancel",
//			   0, 100,
//			   0);

  int nbytes = m_width*m_height;
  unsigned char *mask = new unsigned char[nbytes];

  qint64 bidx = 0;
  for(int d=0; d<m_depth; d++)
    {
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();
      
      uchar *mslice = m_maskFileManager.getSliceMem(d);      
      memcpy(mask, mslice, nbytes);

      for(int w=0; w<m_width; w++)
	for(int h=0; h<m_height; h++)
	  {
	    qint64 idx = (w*m_height + h);
	    if (mask[idx] == Global::tag())
	      m_bitmask.setBit(bidx);

	    bidx++;
	  }
    }

  delete [] mask;

  emit progressReset();
}

void
VolumeMask::dilateBitmask()
{
//  QProgressDialog progress("Dilating Tag Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  QBitArray bitcopy = m_bitmask;
  m_bitmask.fill(false);

  qint64 bidx = 0;
  for(int d=0; d<m_depth; d++)
    {
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();

      for(int w=0; w<m_width; w++)
	for(int h=0; h<m_height; h++)
	  {
	    if (bitcopy.testBit(bidx))
	      {
		int d0 = qMax(d-1, 0);
		int d1 = qMin(d+1, m_depth-1);
		int w0 = qMax(w-1, 0);
		int w1 = qMin(w+1, m_width-1);
		int h0 = qMax(h-1, 0);
		int h1 = qMin(h+1, m_height-1);

		for(int d2=d0; d2<=d1; d2++)
		  for(int w2=w0; w2<=w1; w2++)
		    for(int h2=h0; h2<=h1; h2++)
		      {
			qint64 idx = d2*m_width*m_height +
			             w2*m_height + h2;
			m_bitmask.setBit(idx);
		      }
	      }
	    bidx ++;	   
	  }
    }

  emit progressReset();
}

void
VolumeMask::dilateBitmask(int mind, int maxd,
			  int minw, int maxw,
			  int minh, int maxh)
{
//  QProgressDialog progress("Dilating Tag Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  QBitArray bitcopy = m_bitmask;
  m_bitmask.fill(false);

  for(int d=mind; d<=maxd; d++)
    {
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();

      for(int w=minw; w<=maxw; w++)
	for(int h=minh; h<=maxh; h++)
	  {
	    qint64 bidx = (d*m_width*m_height +
			   w*m_height + h);
	    if (bitcopy.testBit(bidx))
	      {
		int d0 = qMax(d-1, mind);
		int d1 = qMin(d+1, maxd);
		int w0 = qMax(w-1, minw);
		int w1 = qMin(w+1, maxw);
		int h0 = qMax(h-1, minh);
		int h1 = qMin(h+1, maxh);

		for(int d2=d0; d2<=d1; d2++)
		  for(int w2=w0; w2<=w1; w2++)
		    for(int h2=h0; h2<=h1; h2++)
		      {
			qint64 idx = d2*m_width*m_height +
			             w2*m_height + h2;
			m_bitmask.setBit(idx);
		      }
	      }
	    bidx ++;
	    
	  }
    }

  emit progressReset();
}

void
VolumeMask::erodeBitmask()
{
//  QProgressDialog progress("Eroding Tag Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  QBitArray bitcopy = m_bitmask;

  qint64 bidx = 0;
  for(int d=0; d<m_depth; d++)
    {
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();

      for(int w=0; w<m_width; w++)
	for(int h=0; h<m_height; h++)
	  {
	    if (bitcopy.testBit(bidx))
	      {
		int d0 = qMax(d-1, 0);
		int d1 = qMin(d+1, m_depth-1);
		int w0 = qMax(w-1, 0);
		int w1 = qMin(w+1, m_width-1);
		int h0 = qMax(h-1, 0);
		int h1 = qMin(h+1, m_height-1);

		bool ok = true;
		for(int d2=d0; d2<=d1; d2++)
		  for(int w2=w0; w2<=w1; w2++)
		    for(int h2=h0; h2<=h1; h2++)
		      {
			qint64 idx = d2*m_width*m_height +
			             w2*m_height + h2;
			ok &= bitcopy.testBit(idx);
		      }

		if (!ok) // surface voxel
		  m_bitmask.setBit(bidx, false); // set bit to 0
	      }
	    bidx ++;
	    
	  }
    }

  emit progressReset();
}

void
VolumeMask::erodeBitmask(int mind, int maxd,
			 int minw, int maxw,
			 int minh, int maxh)
{
//  QProgressDialog progress("Eroding Tag Mask",
//			   "Cancel",
//			   0, 100,
//			   0);

  QBitArray bitcopy = m_bitmask;

  for(int d=mind; d<=maxd; d++)
    {
      emit progressChanged((int)(100.0*(float)d/(float)m_depth));
      qApp->processEvents();

      for(int w=minw; w<=maxw; w++)
	for(int h=minh; h<=maxh; h++)
	  {
	    qint64 bidx = (d*m_width*m_height +
			   w*m_height + h);
	    if (bitcopy.testBit(bidx))
	      {
		int d0 = qMax(d-1, mind);
		int d1 = qMin(d+1, maxd);
		int w0 = qMax(w-1, minw);
		int w1 = qMin(w+1, maxw);
		int h0 = qMax(h-1, minh);
		int h1 = qMin(h+1, maxh);

		bool ok = true;
		for(int d2=d0; d2<=d1; d2++)
		  for(int w2=w0; w2<=w1; w2++)
		    for(int h2=h0; h2<=h1; h2++)
		      {
			qint64 idx = d2*m_width*m_height +
			             w2*m_height + h2;
			ok &= bitcopy.testBit(idx);
		      }

		if (!ok) // surface voxel
		  m_bitmask.setBit(bidx, false); // set bit to 0
	      }
	    
	  }
    }

  emit progressReset();
}

void
VolumeMask::findConnectedRegion(QList<int> pos)
{
  QBitArray bitcopy = m_bitmask;
  m_bitmask.fill(false);


  QStack<int> stack;

  // put the seeds in
  for(int pi=0; pi<pos.size()/3; pi++)
    {
      int d = pos[3*pi];
      int w = pos[3*pi+1];
      int h = pos[3*pi+2];
      qint64 idx = d*m_width*m_height + w*m_height + h;
      if (bitcopy.testBit(idx))
	{
	  uchar mask;
	  uchar *mslice = m_maskFileManager.rawValueMem(d, w, h);
	  if (mslice)
	    mask = mslice[0];
	  
	  if (mask != Global::tag())
	    {
	      m_bitmask.setBit(idx);
	      stack.push(d);
	      stack.push(w);
	      stack.push(h);
	    }
	}
    }

  uchar tag = Global::tag();
  while(!stack.isEmpty())
    {
      int h = stack.pop();
      int w = stack.pop();
      int d = stack.pop();
      qint64 idx = d*m_width*m_height + w*m_height + h;
      if (bitcopy.testBit(idx))
	{
	  int d0 = qMax(d-1, 0);
	  int d1 = qMin(d+1, m_depth-1);
	  int w0 = qMax(w-1, 0);
	  int w1 = qMin(w+1, m_width-1);
	  int h0 = qMax(h-1, 0);
	  int h1 = qMin(h+1, m_height-1);
	      
	  for(int d2=d0; d2<=d1; d2++)
	    for(int w2=w0; w2<=w1; w2++)
	      for(int h2=h0; h2<=h1; h2++)
		{
		  uchar mask;
		  qint64 idx = d2*m_width*m_height +
		               w2*m_height + h2;
		  uchar *mslice = m_maskFileManager.rawValueMem(d2, w2, h2);
		  if (mslice)
		    mask = mslice[0];

		  if (tag == mask ||
		      (tag && !mask) ||
		      (!tag && mask) )
		    {
		      if (   bitcopy.testBit(idx) &&
			  !m_bitmask.testBit(idx) )
			{
			  m_bitmask.setBit(idx);
			  stack.push(d2);
			  stack.push(w2);
			  stack.push(h2);
			}
		    }
		}
	}
    } // end find connected
  //------------------------------------------------------
}
