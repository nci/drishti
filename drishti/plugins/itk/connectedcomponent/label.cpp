#include "staticfunctions.h"
#include "label.h"
#include "../itkmemoryadmission.h"

#include <QSaveFile>

#include <limits>
#include <memory>
#include <stdexcept>
#include "propertyeditor.h"

#include "itkConnectedComponentImageFilter.h"


#include "itkCommand.h"

void
Label::next()
{
  m_meshProgress->setValue(m_prog);
  m_prog = (m_prog+1)%100;
  qApp->processEvents();
}

Label::Label() {}
Label::~Label() {}

QString
Label::start(VolumeFileManager *vfm,
	     int nX, int nY, int nZ,
	     Vec dataMin, Vec dataMax,
	     QString prevDir,
	     int samplingLevel,
	     QList<Vec> clipPos,
	     QList<Vec> clipNormal,
	     QList<CropObject> crops,
	     QList<PathObject> paths,
	     uchar *lut,
	     int pruneLod, int pruneX, int pruneY, int pruneZ,
	     QVector<uchar> pruneData)
{
  m_vfm = vfm;
  m_voxelType = m_vfm->voxelType();
  m_depth = nX;
  m_width = nY;
  m_height = nZ;
  m_dataMin = dataMin;
  m_dataMax = dataMax;
  m_dataSize = m_dataMax - m_dataMin + Vec(1,1,1);
  m_nX = qMin(int(m_dataSize.z), m_depth);
  m_nY = qMin(int(m_dataSize.y), m_width);
  m_nZ = qMin(int(m_dataSize.x), m_height);
  m_crops = crops;
  m_paths = paths;
  m_pruneLod = pruneLod;
  m_pruneX = pruneX;
  m_pruneY = pruneY;
  m_pruneZ = pruneZ;
  m_pruneData = pruneData;
  m_samplingLevel = samplingLevel;

  // pruneLod that we get is wrt the original sized volume.
  // set pruneLod to reflect the selected sampling level.
  m_pruneLod = qMax((float)m_nX/(float)m_pruneZ,
		    qMax((float)m_nY/(float)m_pruneY,
			 (float)m_nZ/(float)m_pruneX));

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5 %6\n%7").\
//			   arg(m_nX).arg(m_nY).arg(m_nZ).\
//			   arg(m_pruneZ).arg(m_pruneY).arg(m_pruneX).\
//			   arg(m_pruneLod));

  m_meshLog = new QTextEdit;
  m_meshProgress = new QProgressBar;

  QVBoxLayout *meshLayout = new QVBoxLayout;
  meshLayout->addWidget(m_meshLog);
  meshLayout->addWidget(m_meshProgress);

  QWidget *meshWindow = new QWidget;
  meshWindow->setWindowTitle("Drishti - Connect Component Labeling Using Opacity Values");
  meshWindow->setLayout(meshLayout);
  meshWindow->show();
  meshWindow->resize(700, 300);

  m_meshLog->insertPlainText("ITK Connected Component Labeling Code written by Richard Beare.");

  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText(QString("Volume Size : %1 %2 %3\n").\
			   arg(m_depth).
			   arg(m_width).
			   arg(m_height));
  m_meshLog->insertPlainText(QString("DataMin : %1 %2 %3\n").\
			   arg(m_dataMin.z).
			   arg(m_dataMin.y).
			   arg(m_dataMin.x));
  m_meshLog->insertPlainText(QString("DataMax : %1 %2 %3\n").\
			   arg(m_dataMax.z).
			   arg(m_dataMax.y).
			   arg(m_dataMax.x));
  m_meshLog->insertPlainText(QString("DataSize : %1 %2 %3\n").\
			   arg(m_dataSize.z).
			   arg(m_dataSize.y).
			   arg(m_dataSize.x));
  m_meshLog->insertPlainText(QString("Grid size : %1 %2 %3\n").\
			   arg(m_nX).arg(m_nY).arg(m_nZ));
  

  int chan = 0; // mop channel

  bool fullyconnected = true;

  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(fullyconnected);
  plist["fully connected"] = vlist;

  vlist.clear();
  QString mesg;
  mesg += "File : "+m_vfm->fileName()+"\n";
  int d = m_vfm->depth();
  int w = m_vfm->width();
  int h = m_vfm->height();
  mesg += QString("Volume Size : %1 %2 %3\n").arg(h).arg(w).arg(d);
  mesg += QString("Data Min : %1 %2 %3\n").arg(m_dataMin.x).arg(m_dataMin.y).arg(m_dataMin.z);
  mesg += QString("Data Max : %1 %2 %3\n\n").arg(m_dataMax.x).arg(m_dataMax.y).arg(m_dataMax.z);
  mesg += "Transfer function opacity is used for identifying connected components.\n";
  mesg += "This plugin supports 26-connected (i.e. full connectivity) or 6-connected (i.e. face connected) options for connectivity.  Default is fully connected.\n\n";
  mesg += "The labels file will always be in unsigned short (2 bytes per voxel) raw format.\n";
  vlist << mesg;
  plist["message"] = vlist;
  
  QStringList keys;
  keys << "fully connected";
  keys << "message";

  propertyEditor.set("Connected Component Labelling Parameters", plist, keys);
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
  {  
    vmap = propertyEditor.get();

    for(int ik=0; ik<keys.count(); ik++)
      {
	QPair<QVariant, bool> pair = vmap.value(keys[ik]);
	
	if (pair.second)
	  {
	    if (keys[ik] == "fully connected")
	      fullyconnected = pair.first.toBool();
	  }
      }
  }

  QString flnm = QFileDialog::getSaveFileName(0,
					      "Save labels to file",
					      prevDir,
					      "*.raw");
  if (flnm.size() == 0)
    {
      meshWindow->close();
      return "";
    }
  //----------------------------


  bool succeeded = false;
  try
    {
      applyLabeling(flnm,
		    clipPos, clipNormal,
		    crops, paths,
		    lut,
		    chan,
		    fullyconnected);
      succeeded = true;
    }
  catch (const std::exception& error)
    {
      QMessageBox::critical(0, "Error",
                            QString("Cannot run connected-component labeling.\n%1")
                            .arg(QString::fromLocal8Bit(error.what())));
    }
  catch ( ... )
    {
      QMessageBox::critical(0, "Error", "Cannot run connected-component labeling because allocation or filter execution failed.");
    }

  if (!succeeded)
    {
      flnm.clear();
    }


  meshWindow->close();

  return flnm;
}
void
Label::applyTear(int d0, int d1, int nextra,
			 uchar *data0, uchar *data1,
			 bool flag)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("apply dissection ...\n");

  int dlen = d1-d0+1+2*nextra;
  for(int i0=d0-nextra; i0<=d1+nextra; i0++)
    {
      m_meshProgress->setValue((int)(100.0*(float)((i0-d0+nextra)/(float)(dlen))));
      qApp->processEvents();

      for(int j=0; j<m_nY; j++)
	for(int k=0; k<m_nZ; k++)
	  {
	    Vec po = Vec(m_dataMin.x+k, m_dataMin.y+j, m_dataMin.z+i0);
	    po *= m_samplingLevel;
	    for(int ci=0; ci<m_crops.count(); ci++)
	      {
		if (m_crops[ci].cropType() >= CropObject::Tear_Tear &&
		    m_crops[ci].cropType() <= CropObject::Tear_Curl)
		  {
		    Vec newPo;
		    float viewMix = m_crops[ci].checkTear(po, newPo);
		    if (viewMix > 0.01)
		      {
			if (!flag || m_voxelType == 0)
			  data1[(static_cast<qint64>(i0-d0+nextra)*m_nY+j)*m_nZ+k] = 0;
			else 
			  ((ushort*)data1)[(static_cast<qint64>(i0-d0+nextra)*m_nY+j)*m_nZ+k] = 0;
		      }
		    else
		      {
			newPo -= Vec(m_dataMin.x, m_dataMin.y, m_dataMin.z);
			int newi = ceil(newPo.z);
			int newj = ceil(newPo.y);
			int newk = ceil(newPo.x);
			newi = qBound(d0-nextra, newi, d1+nextra);
			//newi = qBound(0, newi, m_depth-1);
			newj = qBound(0, newj, m_nY-1);
			newk = qBound(0, newk, m_nZ-1);
			if (!flag || m_voxelType == 0)
			  data1[(static_cast<qint64>(i0-d0+nextra)*m_nY+j)*m_nZ+k] =
			    data0[(static_cast<qint64>(newi-d0+nextra)*m_nY+newj)*m_nZ+newk];
			else
			  ((ushort*)data1)[(static_cast<qint64>(i0-d0+nextra)*m_nY+j)*m_nZ+k] =
			    ((ushort*)data0)[(static_cast<qint64>(newi-d0+nextra)*m_nY+newj)*m_nZ+newk];
		      }
		  }
	      }
	  }
    }
  m_meshProgress->setValue(100);
}

void
Label::applyOpacity(int iv,
		    uchar* cropped,
		    uchar* lut,
		    uchar* tmp)
{
  if (m_nY <= 0 || m_nZ <= 0)
    throw std::runtime_error("Opacity plane dimensions must be positive");
  const qint64 planeVoxels = static_cast<qint64>(m_nY)*m_nZ;
  qint64 jk = 0;
  for(int j=0; j<m_nY; j++)
    for(int k=0; k<m_nZ; k++)
      {
	if (jk < 0 || jk >= planeVoxels)
	  throw std::runtime_error("Opacity plane index is out of bounds");
	const size_t planeIndex = static_cast<size_t>(jk);
	if (cropped[planeIndex] > 0)
	  {
	    int v;
	    if (m_voxelType == 0)
	      v = tmp[planeIndex];
	    else
	      v = reinterpret_cast<const ushort*>(tmp)[planeIndex];

	    int tfSet = 0;

	    if (m_blendPresent)
	      { // calculate opacity
		for(int ci=0; ci<m_crops.count(); ci++)
		  {
		    if (m_crops[ci].cropType() > CropObject::Displace_Displace &&
			m_crops[ci].cropType() < CropObject::Glow_Ball)
		      {
			Vec po = Vec(m_dataMin.x+k, m_dataMin.y+j, iv);
			po *= m_samplingLevel;
			float viewMix = m_crops[ci].checkBlend(po);
			if (viewMix > 0.5)
			  {
			    tfSet = m_crops[ci].tfset();
			    tfSet *= 256*256*4;
			  }
		      }
		  }
	      }
	    if (m_pathBlendPresent)
	      { // calculate opacity
		for(int ci=0; ci<m_paths.count(); ci++)
		  {
		    if (m_paths[ci].blend())
		      {
			Vec po = Vec(m_dataMin.x+k, m_dataMin.y+j, iv);
			po *= m_samplingLevel;
			float viewMix = m_paths[ci].checkBlend(po);
			if (viewMix > 0.5)
			  {
			    tfSet = m_paths[ci].blendTF();
			    tfSet *= 256*256*4;
			  }
		      }
		  }
	      }
		
	    float mop = cropped[planeIndex]/255.0;
	    float opac = 0;
	    if (m_voxelType == 0)
	      opac = mop*lut[tfSet + 4*v + 3];
	    else
	      {
		int a = v%256;
		int b = v/256;
		opac = mop*lut[tfSet + 4*(256*a + b) + 3];		
	      }

	    if (opac > 0)
	      tmp[planeIndex] = 255;
	    else
	      tmp[planeIndex] = 0;
	  }
	else
	  tmp[planeIndex] = 0;
	jk++;
      } // tmp now contains binary data based on opacity

  if (jk != planeVoxels)
    throw std::runtime_error("Opacity plane traversal did not match its buffer");
}

bool
Label::checkBlend(Vec po, ushort v, uchar* lut)
{
  for(int ci=0; ci<m_crops.count(); ci++)
    {
      if (m_crops[ci].cropType() > CropObject::Displace_Displace &&
	  m_crops[ci].cropType() < CropObject::Glow_Ball)
	{
	  float viewMix = m_crops[ci].checkBlend(po);
	  if (viewMix > 0.5)
	    {
	      int tfSet = m_crops[ci].tfset();
	      tfSet *= 256*256*4;
	      int a,b;
	      if (m_voxelType == 0)
		{
		  a = 0;
		  b = v;		  
		}
	      else
		{
		  a = v%256;
		  b = v/256;
		}
	      if (lut[tfSet+4*(256*a + b)+3] < 5)
		return false;
	    }
	}
    }
  return true;
}

bool
Label::checkCrop(Vec po)
{
  for(int ci=0; ci<m_crops.count(); ci++)
    {
      if (m_crops[ci].cropType() < CropObject::Tear_Tear)
	{
	  if (m_crops[ci].checkCropped(po) == false)
	    return false;
	}
    }
  return true;
}

bool
Label::checkPathBlend(Vec po, ushort v, uchar* lut)
{
  for(int ci=0; ci<m_paths.count(); ci++)
    {
      if (m_paths[ci].blend())
	{
	  float viewMix = m_paths[ci].checkBlend(po);
	  if (viewMix > 0.5)
	    {
	      int tfSet = m_paths[ci].blendTF();
	      tfSet *= 256*256*4;
	      int a,b;
	      if (m_voxelType == 0)
		{
		  a = 0;
		  b = v;		  
		}
	      else
		{
		  a = v%256;
		  b = v/256;
		}
	      if (lut[tfSet+4*(256*a + b)+3] < 5)
		return false;
	    }
	}
    }
  return true;
}

bool
Label::checkPathCrop(Vec po)
{
  for(int ci=0; ci<m_paths.count(); ci++)
    {
      if (m_paths[ci].crop())
	{
	  if (m_paths[ci].checkCropped(po) == false)
	    return false;
	}
    }
  return true;
}

void
Label::applyLabeling(QString flnm,
		     QList<Vec> clipPos,
		     QList<Vec> clipNormal,
		     QList<CropObject> crops,
		     QList<PathObject> paths,
		     uchar *lut,
		     int chan,
		     bool fullyconnected)
{
  if (m_voxelType != VolumeFileManager::_UChar &&
      m_voxelType != VolumeFileManager::_UShort)
    throw std::runtime_error(
        "Connected components supports only unsigned 8-bit and 16-bit volumes");
  const int bpv = (m_voxelType == VolumeFileManager::_UShort ? 2 : 1);
  if (m_nX <= 0 || m_nY <= 0 || m_nZ <= 0)
    throw std::runtime_error("Label volume dimensions must be positive");

  qint64 voxelCount = m_nX;
  if (voxelCount > std::numeric_limits<qint64>::max()/m_nY)
    throw std::runtime_error("Label volume dimensions overflow addressable memory");
  voxelCount *= m_nY;
  if (voxelCount > std::numeric_limits<qint64>::max()/m_nZ)
    throw std::runtime_error("Label volume dimensions overflow addressable memory");
  voxelCount *= m_nZ;

  const qint64 planeVoxels = static_cast<qint64>(m_nY)*m_nZ;
  if (planeVoxels > std::numeric_limits<qint64>::max()/bpv)
    throw std::runtime_error("Label volume dimensions overflow addressable memory");
  const qint64 planeBytes64 = planeVoxels*bpv;
  if (static_cast<quint64>(voxelCount) > std::numeric_limits<size_t>::max() ||
      static_cast<quint64>(planeBytes64) > std::numeric_limits<size_t>::max())
    throw std::runtime_error("Label volume exceeds addressable memory");

  const size_t nbytes = static_cast<size_t>(planeBytes64);
  const size_t planeVoxelCount = static_cast<size_t>(planeVoxels);

  if (m_width <= 0 || m_height <= 0)
    throw std::runtime_error("Source slice dimensions must be positive");
  const qint64 sourcePlaneVoxels = static_cast<qint64>(m_width)*m_height;
  if (sourcePlaneVoxels > std::numeric_limits<qint64>::max()/bpv)
    throw std::runtime_error("Source slice dimensions overflow addressable memory");
  const qint64 sourcePlaneBytes64 = sourcePlaneVoxels*bpv;
  if (planeBytes64 > sourcePlaneBytes64 ||
      static_cast<quint64>(sourcePlaneBytes64) >
      std::numeric_limits<size_t>::max())
    throw std::runtime_error("Source slice is smaller than the requested label plane");

  const auto checkedPlaneIndex = [=](int row, int column) -> size_t
    {
      if (row < 0 || row >= m_nY || column < 0 || column >= m_nZ)
        throw std::runtime_error("Label plane index is out of bounds");
      const qint64 index = static_cast<qint64>(row)*m_nZ + column;
      if (index < 0 || index >= planeVoxels)
        throw std::runtime_error("Label plane index exceeds its buffer");
      return static_cast<size_t>(index);
    };
  const auto checkedSourceIndex = [=](qint64 row, qint64 column) -> size_t
    {
      if (row < 0 || row >= m_width || column < 0 || column >= m_height)
        throw std::runtime_error("Requested source voxel is outside its slice");
      const qint64 index = row*m_height + column;
      if (index < 0 || index >= sourcePlaneVoxels)
        throw std::runtime_error("Source voxel index exceeds its slice buffer");
      return static_cast<size_t>(index);
    };

  if (m_pruneX <= 0 || m_pruneY <= 0 || m_pruneZ <= 0 ||
      m_pruneLod <= 0 || chan < 0 || chan >= 3)
    throw std::runtime_error("Prune data geometry or channel is invalid");
  const qint64 prunePlane = static_cast<qint64>(m_pruneY)*m_pruneX;
  if (prunePlane > std::numeric_limits<qint64>::max()/m_pruneZ)
    throw std::runtime_error("Prune data dimensions overflow addressable memory");
  const qint64 pruneVoxelCount = prunePlane*m_pruneZ;
  if (pruneVoxelCount > std::numeric_limits<qint64>::max()/3 ||
      3*pruneVoxelCount > static_cast<qint64>(m_pruneData.size()))
    throw std::runtime_error("Prune data buffer is smaller than its geometry");

  requireItkMemoryAdmission(
    ItkMemoryWorkload::ConnectedComponents,
    static_cast<std::uint64_t>(m_nX),
    static_cast<std::uint64_t>(m_nY),
    static_cast<std::uint64_t>(m_nZ));

  bool trim = (qRound(m_dataSize.x) < m_height ||
	       qRound(m_dataSize.y) < m_width ||
	       qRound(m_dataSize.z) < m_depth);
  bool clipPresent = (clipPos.count() > 0);

  m_cropPresent = false;
  m_tearPresent = false;
  m_blendPresent = false;
  for(int ci=0; ci<m_crops.count(); ci++)
    {
      if (crops[ci].cropType() < CropObject::Tear_Tear)
	m_cropPresent = true;
      else if (crops[ci].cropType() < CropObject::View_Tear)
	m_tearPresent = true;
      else if (m_crops[ci].cropType() > CropObject::Displace_Displace &&
	       m_crops[ci].cropType() < CropObject::Glow_Ball)
	m_blendPresent = true;
    }

  m_pathCropPresent = false;
  m_pathBlendPresent = false;
  for (int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i].blend()) m_pathBlendPresent = true;
      if (m_paths[i].crop()) m_pathCropPresent = true;
    }

  m_meshLog->moveCursor(QTextCursor::End);
  int d0 = 0;
  int d1 = m_nX-1;
  int d0z = d0 + qRound(m_dataMin.z);
  int d1z = d1 + qRound(m_dataMin.z);

  std::unique_ptr<uchar[]> opacityOwner(
      new uchar[static_cast<size_t>(voxelCount)]);
  uchar *opacityVol = opacityOwner.get();

  std::unique_ptr<uchar[]> croppedOwner(new uchar[planeVoxelCount]);
  std::unique_ptr<uchar[]> tmpOwner(new uchar[nbytes]);
  uchar *cropped = croppedOwner.get();
  uchar *tmp = tmpOwner.get();

  int i0 = 0;
  for(int i=d0z; i<=d1z; i++)
    {
      m_meshProgress->setValue((int)(100.0*(float)i0/(float)m_nX));
      qApp->processEvents();

      int iv = qBound(0, i, m_depth-1);
      uchar *vslice = m_vfm->getSlice(iv);
      if (!vslice)
        throw std::runtime_error(
            QString("Cannot read source slice %1: %2")
            .arg(iv).arg(m_vfm->lastError()).toStdString());

      memset(cropped, 0, planeVoxelCount);

      if (!trim)
	memcpy(tmp, vslice, nbytes);
      else
	{
	  int wmin = qRound(m_dataMin.y);
	  int hmin = qRound(m_dataMin.x);
	  if (m_voxelType == 0)
	    {
	      for(int w=0; w<m_nY; w++)
		for(int h=0; h<m_nZ; h++)
		  {
		    const size_t targetIndex = checkedPlaneIndex(w, h);
		    const size_t sourceIndex = checkedSourceIndex(
		        static_cast<qint64>(wmin)+w,
		        static_cast<qint64>(hmin)+h);
		    tmp[targetIndex] = vslice[sourceIndex];
		  }
	    }
	  else
	    {
	      for(int w=0; w<m_nY; w++)
		for(int h=0; h<m_nZ; h++)
		  {
		    const size_t targetIndex = checkedPlaneIndex(w, h);
		    const size_t sourceIndex = checkedSourceIndex(
		        static_cast<qint64>(wmin)+w,
		        static_cast<qint64>(hmin)+h);
		    reinterpret_cast<ushort*>(tmp)[targetIndex] =
		        reinterpret_cast<const ushort*>(vslice)[sourceIndex];
		  }
	    }
	}

      for(int j=0; j<m_nY; j++)
	for(int k=0; k<m_nZ; k++)
	  {
	    const size_t planeIndex = checkedPlaneIndex(j, k);
	    Vec po = Vec(m_dataMin.x+k, m_dataMin.y+j, iv);
	    bool ok = true;
	    
	    // we don't want to scale before pruning
	    int mop = 0;
	    {
	      Vec pp = po - m_dataMin;
	      int ppi = pp.x/m_pruneLod;
	      int ppj = pp.y/m_pruneLod;
	      int ppk = pp.z/m_pruneLod;
	      ppi = qBound(0, ppi, m_pruneX-1);
	      ppj = qBound(0, ppj, m_pruneY-1);
	      ppk = qBound(0, ppk, m_pruneZ-1);
	      const qint64 mopidx =
	          (static_cast<qint64>(ppk)*m_pruneY + ppj)*m_pruneX + ppi;
	      const qint64 mopOffset = 3*mopidx + chan;
	      if (mopidx < 0 || mopidx >= pruneVoxelCount ||
	          mopOffset < 0 ||
	          mopOffset >= static_cast<qint64>(m_pruneData.size()) ||
	          mopOffset > std::numeric_limits<int>::max())
	        throw std::runtime_error("Prune data index is out of bounds");
	      mop = m_pruneData.at(static_cast<int>(mopOffset));
	      ok = (mop > 0);
	    }
	    
	    po *= m_samplingLevel;
	    
	    if (ok && clipPresent)
	      ok = StaticFunctions::getClip(po, clipPos, clipNormal);
	    
	    if (ok && m_cropPresent)
	      ok = checkCrop(po);
	    
	    if (ok && m_pathCropPresent)
	      ok = checkPathCrop(po);
	    
	    if (ok && m_blendPresent)
	      {
		ushort v;
		if (m_voxelType == 0)
		  v = tmp[planeIndex];
		else
		  v = reinterpret_cast<const ushort*>(tmp)[planeIndex];
		ok = checkBlend(po, v, lut);
	      }
	    
	    if (ok && m_pathBlendPresent)
	      {
		ushort v;
		if (m_voxelType == 0)
		  v = tmp[planeIndex];
		else
		  v = reinterpret_cast<const ushort*>(tmp)[planeIndex];
		ok = checkPathBlend(po, v, lut);
	      }
	    
	    if (ok)
	      cropped[planeIndex] = mop;
	    else
	      cropped[planeIndex] = 0;
	  }
      
      if (m_voxelType == 0)
	{
	  for(qint64 j=0; j<planeVoxels; j++)
	    {
	      if (cropped[j] == 0)
		tmp[j] = 0;
	    }
	}
      else
	{
	  for(qint64 j=0; j<planeVoxels; j++)
	    {
	      if (cropped[j] == 0)
		((ushort*)tmp)[j] = 0;
	    }
	}
      
      applyOpacity(iv, cropped, lut, tmp);
      memcpy(opacityVol + static_cast<size_t>(i0)*static_cast<size_t>(planeVoxels),
             tmp, static_cast<size_t>(planeVoxels));
      
      i0++;
    }
  m_meshProgress->setValue(100);
  qApp->processEvents();

  //------------
  if (m_tearPresent)
    {
      std::unique_ptr<uchar[]> data0Owner(
          new uchar[static_cast<size_t>(voxelCount)]);
      uchar *data0 = data0Owner.get();
      memcpy(data0, opacityVol, static_cast<size_t>(voxelCount));
      applyTear(d0, d1, 0,
		data0, opacityVol, false);
    }


  typedef uchar PixelType;
  const unsigned int Dimension = 3;
  typedef itk::Image< PixelType, Dimension > ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = m_nZ;
  size[1] = m_nY;
  size[2] = m_nX;

  ImageType::RegionType region(start, size);

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, opacityVol, static_cast<size_t>(voxelCount));

  typedef itk::Image< ushort, 3 > OutputImageType;
  typedef itk::ConnectedComponentImageFilter<ImageType, OutputImageType> LabelFilter;
  LabelFilter::Pointer labelFilter = LabelFilter::New();
  labelFilter->SetFullyConnected(fullyconnected);
  labelFilter->SetInput( image );


  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<Label> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &Label::next);
  labelFilter->AddObserver(itk::ProgressEvent(), progressbar);

  labelFilter->Update();

  QSaveFile fp(flnm);
  fp.setDirectWriteFallback(false);
  if (!fp.open(QFile::WriteOnly))
    throw std::runtime_error(
        QString("Cannot open output file: %1").arg(fp.errorString()).toStdString());
  uchar vt = 2;
  OutputImageType *dimg = labelFilter->GetOutput();
  char *tdata = (char*)(dimg->GetBufferPointer());
  if (voxelCount > std::numeric_limits<qint64>::max()/2)
    throw std::runtime_error("Label output size overflows file offsets");
  const qint64 outputBytes = 2*voxelCount;
  if (fp.write((char*)&vt, 1) != 1 ||
      fp.write((char*)&m_nX, 4) != 4 ||
      fp.write((char*)&m_nY, 4) != 4 ||
      fp.write((char*)&m_nZ, 4) != 4 ||
      fp.write(tdata, outputBytes) != outputBytes)
    {
      const QString detail = fp.errorString();
      fp.cancelWriting();
      throw std::runtime_error(
          QString("Cannot write complete label volume: %1")
          .arg(detail).toStdString());
    }
  if (!fp.commit())
    throw std::runtime_error(
        QString("Cannot commit label volume: %1")
        .arg(fp.errorString()).toStdString());

  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Label data saved in "+flnm);

  QMessageBox::information(0, "", QString("Label data saved in "+flnm));
}
