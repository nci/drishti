#include "staticfunctions.h"
#include "skeletonizer.h"

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"

#include "itkConnectedThresholdImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkBinaryThinningImageFilter3D.h"


#include "itkCommand.h"

void
Skeletonizer::next()
{
  m_meshProgress->setValue(m_prog);
  m_prog = (m_prog+1)%100;
  qApp->processEvents();
}


Skeletonizer::Skeletonizer() {}
Skeletonizer::~Skeletonizer() {}

QString
Skeletonizer::start(VolumeFileManager *vfm,
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
  meshWindow->setWindowTitle("Drishti - Binary Thinning Using Opacity Values");
  meshWindow->setLayout(meshLayout);
  meshWindow->show();
  meshWindow->resize(700, 300);

  m_meshLog->insertPlainText("ITK 3D Binary Thinning Code written by Homann H.");

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
  

  QString flnm = QFileDialog::getSaveFileName(0,
					      "Save skeleton to file",
					      prevDir,
					      "*.raw");
  if (flnm.size() == 0)
    {
      meshWindow->close();
      return "";
    }
  //----------------------------

  int chan = 0; // mop channel

  try
    {
      applyBinaryThinning(flnm,
			  clipPos, clipNormal,
			  crops, paths,
			  lut,
			  chan);
    }
  catch ( ... )
    {
      QMessageBox::information(0, "Error", "Sorry cannot run the filter.\nProbably cannot allocate enough memory or failure in the filter code.");
    }
  

  meshWindow->close();

  return flnm;
}
void
Skeletonizer::applyTear(int d0, int d1, int nextra,
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
			  data1[(i0-d0+nextra)*m_nY*m_nZ + j*m_nZ + k] = 0;
			else 
			  ((ushort*)data1)[(i0-d0+nextra)*m_nY*m_nZ + j*m_nZ + k] = 0;
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
			  data1[(i0-d0+nextra)*m_nY*m_nZ + j*m_nZ + k] = 
			    data0[(newi-d0+nextra)*m_nY*m_nZ + newj*m_nZ + newk];
			else
			  ((ushort*)data1)[(i0-d0+nextra)*m_nY*m_nZ + j*m_nZ + k] = 
			    ((ushort*)data0)[(newi-d0+nextra)*m_nY*m_nZ + newj*m_nZ + newk];
		      }
		  }
	      }
	  }
    }
  m_meshProgress->setValue(100);
}

void
Skeletonizer::applyOpacity(int iv,
			    uchar* cropped,
			    uchar* lut,
			    uchar* tmp)
{
  int jk = 0;
  for(int j=0; j<m_nY; j++)
    for(int k=0; k<m_nZ; k++)
      {
	if (cropped[jk] > 0)
	  {
	    int v;
	    if (m_voxelType == 0)
	      v = tmp[jk];
	    else
	      v = ((ushort*)tmp)[jk];

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
		
	    float mop = cropped[jk]/255.0;
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
	      tmp[jk] = 255;
	    else
	      tmp[jk] = 0;
	  }
	else
	  tmp[jk] = 0;
	jk++;
      } // tmp now contains binary data based on opacity
}

bool
Skeletonizer::checkBlend(Vec po, ushort v, uchar* lut)
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
Skeletonizer::checkCrop(Vec po)
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
Skeletonizer::checkPathBlend(Vec po, ushort v, uchar* lut)
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
Skeletonizer::checkPathCrop(Vec po)
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
Skeletonizer::applyBinaryThinning(QString flnm,
				   QList<Vec> clipPos,
				   QList<Vec> clipNormal,
				   QList<CropObject> crops,
				   QList<PathObject> paths,
				   uchar *lut,
				   int chan)
{
  int bpv = 1;
  if (m_voxelType > 0) bpv = 2;
  int nbytes = bpv*m_nY*m_nZ;

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

  uchar *opacityVol = new uchar[m_nX*m_nY*m_nZ];
  
  uchar *cropped = new uchar[nbytes];
  uchar *tmp = new uchar[nbytes];

  int i0 = 0;
  for(int i=d0z; i<=d1z; i++)
    {
      m_meshProgress->setValue((int)(100.0*(float)i0/(float)m_nX));
      qApp->processEvents();

      int iv = qBound(0, i, m_depth-1);
      uchar *vslice = m_vfm->getSlice(iv);

      memset(cropped, 0, nbytes);

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
		  tmp[w*m_nZ + h] = vslice[(wmin+w)*m_height + (hmin+h)];
	    }
	  else
	    {
	      for(int w=0; w<m_nY; w++)
		for(int h=0; h<m_nZ; h++)
		  ((ushort*)tmp)[w*m_nZ + h] = ((ushort*)vslice)[(wmin+w)*m_height + (hmin+h)];
	    }
	}

      int jk = 0;
      for(int j=0; j<m_nY; j++)
	for(int k=0; k<m_nZ; k++)
	  {
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
	      int mopidx = ppk*m_pruneY*m_pruneX + ppj*m_pruneX + ppi;
	      mop = m_pruneData[3*mopidx + chan];
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
		  v = tmp[j*m_nZ + k];
		else
		  v = ((ushort*)tmp)[j*m_nZ + k];
		ok = checkBlend(po, v, lut);
	      }
	    
	    if (ok && m_pathBlendPresent)
	      {
		ushort v;
		if (m_voxelType == 0)
		  v = tmp[j*m_nZ + k];
		else
		  v = ((ushort*)tmp)[j*m_nZ + k];
		ok = checkPathBlend(po, v, lut);
	      }
	    
	    if (ok)
	      cropped[jk] = mop;
	    else
	      cropped[jk] = 0;
	    
	    jk ++;
	  }
      
      if (m_voxelType == 0)
	{
	  for(int j=0; j<m_nY*m_nZ; j++)
	    {
	      if (cropped[j] == 0)
		tmp[j] = 0;
	    }
	}
      else
	{
	  for(int j=0; j<m_nY*m_nZ; j++)
	    {
	      if (cropped[j] == 0)
		((ushort*)tmp)[j] = 0;
	    }
	}
      
      applyOpacity(iv, cropped, lut, tmp);
      memcpy(opacityVol + i0*m_nY*m_nZ, tmp, m_nY*m_nZ);
      
      i0++;
    }
  delete [] tmp;
  delete [] cropped;
  m_meshProgress->setValue(100);
  qApp->processEvents();

  //------------
  if (m_tearPresent)
    {
      uchar *data0 = new uchar[m_nX*m_nY*m_nZ];
      memcpy(data0, opacityVol, m_nX*m_nY*m_nZ);
      applyTear(d0, d1, 0,
		data0, opacityVol, false);
      
      delete [] data0;
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
  memcpy(iptr, opacityVol, m_nX*m_nY*m_nZ);


  typedef itk::BinaryThinningImageFilter3D< ImageType, ImageType > ThinningFilterType;
  ThinningFilterType::Pointer thinningFilter = ThinningFilterType::New();
  thinningFilter->SetInput( image );

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<Skeletonizer> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &Skeletonizer::next);
  thinningFilter->AddObserver(itk::ProgressEvent(), progressbar);

  thinningFilter->Update();

  QFile fp;
  fp.setFileName(flnm);
  fp.open(QFile::WriteOnly);
  uchar vt = 0;
  fp.write((char*)&vt, 1);
  fp.write((char*)&m_nX, 4);
  fp.write((char*)&m_nY, 4);
  fp.write((char*)&m_nZ, 4);
  ImageType *dimg = thinningFilter->GetOutput();
  char *tdata = (char*)(dimg->GetBufferPointer());
  fp.write(tdata, m_nX*m_nY*m_nZ);
  fp.close();


  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Binary thinned data saved in "+flnm);

  QMessageBox::information(0, "", QString("Binary thinned data saved in "+flnm));
}
