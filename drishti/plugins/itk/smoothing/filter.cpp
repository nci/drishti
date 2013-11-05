#include "staticfunctions.h"
#include "filter.h"
#include "propertyeditor.h"

#include "itkMeanImageFilter.h"
#include "itkMedianImageFilter.h"
#include "itkBinomialBlurImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkSmoothingRecursiveGaussianImageFilter.h"

SmoothingFilter::SmoothingFilter() {}
SmoothingFilter::~SmoothingFilter() {}

#include "itkCommand.h"

void
SmoothingFilter::next()
{
  m_meshProgress->setValue(m_prog);
  m_prog = (m_prog+1)%100;
  qApp->processEvents();
}


QString
SmoothingFilter::start(VolumeFileManager *vfm,
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
  meshWindow->setWindowTitle("Drishti - Smoothing Using Voxel Values");
  meshWindow->setLayout(meshLayout);
  meshWindow->show();
  meshWindow->resize(700, 300);

  m_meshLog->insertPlainText("ITK Smoothing Filters.");

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

  int filterType = 0;
  bool usePruneData = false;

  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant("combobox");
  vlist << 0;
  vlist << QVariant("mean");
  vlist << QVariant("median");
  vlist << QVariant("binomial");
  vlist << QVariant("discrete gaussian");
  vlist << QVariant("recursive gaussian");
  
  plist["filter type"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(usePruneData);
  plist["prune"] = vlist;

  vlist.clear();
  QString mesg;
  mesg += "File : "+m_vfm->fileName()+"\n";
  int d = m_vfm->depth();
  int w = m_vfm->width();
  int h = m_vfm->height();
  mesg += QString("Volume Size : %1 %2 %3\n").arg(h).arg(w).arg(d);
  mesg += QString("Data Min : %1 %2 %3\n").arg(m_dataMin.x).arg(m_dataMin.y).arg(m_dataMin.z);
  mesg += QString("Data Max : %1 %2 %3\n\n").arg(m_dataMax.x).arg(m_dataMax.y).arg(m_dataMax.z);
  mesg += "Voxel values are used for the smoothing operation.\n";
  vlist << mesg;
  plist["message"] = vlist;
  
  QStringList keys;
  keys << "filter type";
  keys << "prune";
  keys << "message";

  propertyEditor.set("Smoothing Parameters", plist, keys);
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
  {  
    vmap = propertyEditor.get();

    for(int ik=0; ik<keys.count(); ik++)
      {
	QPair<QVariant, bool> pair = vmap.value(keys[ik]);
	
	if (pair.second)
	  {
	    if (keys[ik] == "filter type")
	      filterType = pair.first.toInt();
	    else if (keys[ik] == "prune")
	      usePruneData = pair.first.toBool();
	  }
      }
  }
  else
    {
      meshWindow->close();
      return "";
    }

  //----------------------------
  QString flnm = QFileDialog::getSaveFileName(0,
					      "Save filtered data to file",
					      prevDir,
					      "*.raw");
  if (flnm.size() == 0)
    {
      meshWindow->close();
      return "";
    }
  //----------------------------


  try
    {
      applyFilter(flnm,
		  clipPos, clipNormal,
		  crops, paths,
		  lut,
		  chan,
		  filterType,
		  usePruneData);
    }
  catch ( ... )
    {
      QMessageBox::information(0, "Error", "Sorry cannot run the filter.\nProbably cannot allocate enough memory or failure in the filter code.");
    }


  meshWindow->close();

  return flnm;
}

void
SmoothingFilter::applyFilter(QString flnm,
			     QList<Vec> clipPos,
			     QList<Vec> clipNormal,
			     QList<CropObject> crops,
			     QList<PathObject> paths,
			     uchar *lut,
			     int chan,
			     int filterType,
			     bool usePruneData)
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

  uchar *rawVol = new uchar[bpv*m_nX*m_nY*m_nZ];
  
  uchar *tmp = new uchar[nbytes];

  int i0 = 0;
  for(int i=d0z; i<=d1z; i++)
    {
      m_meshProgress->setValue((int)(100.0*(float)i0/(float)m_nX));
      qApp->processEvents();

      int iv = qBound(0, i, m_depth-1);
      uchar *vslice = m_vfm->getSlice(iv);

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

      if (usePruneData)
	{
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
		
		if (!ok)
		  {
		    if (m_voxelType == 0)
		      tmp[jk] = 0;
		    else
		      ((ushort*)tmp)[jk] = 0;
		  }
		
		jk ++;
	      }
	}

      memcpy(rawVol + bpv*i0*m_nY*m_nZ, tmp, bpv*m_nY*m_nZ);
      
      i0++;
    }
  delete [] tmp;
  m_meshProgress->setValue(100);
  qApp->processEvents();


  if (filterType == 0)
    MeanFilter(rawVol); 
  else if (filterType == 1)
    MedianFilter(rawVol);
  else if (filterType == 2)
    BinomialFilter(rawVol);
  else if (filterType == 3)
     DiscreteGaussianFilter(rawVol);
  else if (filterType == 4)
    RecursiveGaussianFilter(rawVol);
   

  QFile fp;
  fp.setFileName(flnm);
  fp.open(QFile::WriteOnly);
  uchar vt = 0;
  fp.write((char*)&vt, 1);
  fp.write((char*)&m_nX, 4);
  fp.write((char*)&m_nY, 4);
  fp.write((char*)&m_nZ, 4);
  fp.write((char*)rawVol, m_nX*m_nY*m_nZ);
  fp.close();

  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("SmoothingFilter data saved in "+flnm);

  QMessageBox::information(0, "", QString("SmoothingFilter data saved in "+flnm));

  savePvl(flnm);
}

void
SmoothingFilter::savePvl(QString flnm)
{
  bool ok = false;
  QStringList slevels;
  slevels << "Yes";  
  slevels << "No";
  QString option = QInputDialog::getItem(0,
		   "Save pvl.nc",
		   QString("Save .pvl.nc file (containing reference to %1) ?").arg(flnm),
		    slevels,
			  0,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "Yes")
	{
	  QFileInfo fi(flnm);
	  QString pvlflnm = QFileDialog::getSaveFileName(0,
					      "Save to .pvl.nc file",
					      fi.absolutePath(),
					      "*.pvl.nc");
	  if (pvlflnm.size() == 0)
	    return;

	  QFile fp(pvlflnm);
	  if (fp.open(QFile::WriteOnly | QFile::Truncate))
	    {
	      QDir pdir(QFileInfo(pvlflnm).absolutePath());
	      QTextStream out(&fp);
	      out << "<!DOCTYPE Drishti_Header>\n";
	      out << "<PvlDotNcFileHeader>\n";
	      out << QString("  <pvlnames>%1</pvlnames>\n").arg(pdir.relativeFilePath(flnm));
	      out << "  <voxeltype>unsigned char</voxeltype>\n";
	      out << "  <pvlvoxeltype>unsigned char</pvlvoxeltype>\n";
	      out << QString("  <gridsize>%1 %2 %3</gridsize>\n").arg(m_nX).arg(m_nY).arg(m_nZ);
	      out << "  <voxelunit>nounit</voxelunit>\n";
	      out << "  <voxelsize>1 1 1</voxelsize>\n";
	      out << "  <description></description>\n";
	      out << QString("  <slabsize>%1</slabsize>\n").arg(m_nX+1);
	      out << "  <rawmap>0 255 </rawmap>\n";
	      out << "  <pvlmap>0 255 </pvlmap>\n";
	      out << "</PvlDotNcFileHeader>\n";

	      QMessageBox::information(0, "", QString("pvl.nc information saved in "+pvlflnm));
	    }	  
	}
    }

}

void
SmoothingFilter::MeanFilter(uchar *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("Mean Filter applies an averaging filter to the volume.\n");
  m_meshLog->insertPlainText("Neighbourhood radius of (1 1 1) results in 3x3x3 neighbourhood.\n");
  m_meshLog->insertPlainText("Large neighbourhood result in more smoothed image.\n");
  m_meshLog->insertPlainText("Edges also get blurred with this filter.\n");

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

  ImageType::SizeType radius;
  radius[0] = radius[1] = radius[2] = 1;

  bool ok;
  QString text = QInputDialog::getText(0, "Neighbourhood Radius",
				       "Neighbourhood Radius",
				       QLineEdit::Normal,
				       "1 1 1",
				       &ok);

  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	radius[0] = radius[1] = radius[2] = list[0].toInt();
      if (list.count() > 1)
	radius[1] = radius[2] = list[1].toInt();
      if (list.count() > 2)
	 radius[2] = list[2].toInt();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, m_nX*m_nY*m_nZ);

  typedef itk::MeanImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetRadius(radius);

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());
  
  memcpy(inVol, outVol, m_nX*m_nY*m_nZ);
}

void
SmoothingFilter::MedianFilter(uchar *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("Apply median filter to the volume.\n");
  m_meshLog->insertPlainText("Neighbourhood radius of (1 1 1) results in 3x3x3 neighbourhood.\n");
  m_meshLog->insertPlainText("Large neighbourhood result in more smoothed image.\n");
  m_meshLog->insertPlainText("Edges also get blurred with this filter.\n");

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

  ImageType::SizeType radius;
  radius[0] = radius[1] = radius[2] = 1;

  bool ok;
  QString text = QInputDialog::getText(0, "Neighbourhood Radius",
				       "Neighbourhood Radius",
				       QLineEdit::Normal,
				       "1 1 1",
				       &ok);

  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	radius[0] = radius[1] = radius[2] = list[0].toInt();
      if (list.count() > 1)
	radius[1] = radius[2] = list[1].toInt();
      if (list.count() > 2)
	 radius[2] = list[2].toInt();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, m_nX*m_nY*m_nZ);

  typedef itk::MedianImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetRadius(radius);

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());

  memcpy(inVol, outVol, m_nX*m_nY*m_nZ);
}

void
SmoothingFilter::BinomialFilter(uchar *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("Performs a separable blur on each dimension of the volume.\nThe binomial blur consists of a nearest neighbor average along each image dimension. The net result after n-iterations approaches convolution with a gaussian.\n");
  m_meshLog->insertPlainText("Edges also get blurred with this filter.\n");

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

  int ntimes = 3;

  bool ok;
  QString text = QInputDialog::getText(0, "Number of iterations",
				       "iterations",
				       QLineEdit::Normal,
				       "3",
				       &ok);

  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	ntimes = list[0].toInt();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, m_nX*m_nY*m_nZ);

  typedef itk::BinomialBlurImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetRepetitions(ntimes);

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());

  memcpy(inVol, outVol, m_nX*m_nY*m_nZ);
}

void
SmoothingFilter::DiscreteGaussianFilter(uchar *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("Blurs an image by separable convolution with discrete gaussian kernels.  This filter performs Gaussian blurring by separable convolution and a discrete Gaussian operator (kernel).\n");
  m_meshLog->insertPlainText("Large Gaussian variances will produce large convolution kernels and correspondingly slower computational times.");
  m_meshLog->insertPlainText("Edges also get blurred with this filter.\n");

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

  float variance = 0.5;
  int kernelWidth = 10;

  bool ok;
  QString text = QInputDialog::getText(0, "Gaussian Variance and Maximum Kernel Width",
				       "Gaussian Variance and Maximum Kernel Width",
				       QLineEdit::Normal,
				       "0.5 10",
				       &ok);

  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	variance = list[0].toFloat();
      if (list.count() > 1)
	kernelWidth = list[1].toInt();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, m_nX*m_nY*m_nZ);

  typedef itk::DiscreteGaussianImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetVariance(variance);
  filter->SetMaximumKernelWidth(kernelWidth);

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());

  memcpy(inVol, outVol, m_nX*m_nY*m_nZ);
}

void
SmoothingFilter::RecursiveGaussianFilter(uchar *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("Computes the smoothing of an image by convolution with the Gaussian kernels implemented as Infinite Impulse Response (IIR) filters. This filter is implemented using the recursive gaussian filters.\n");
  m_meshLog->insertPlainText("Large sigma values result in more smoothing.\n");
  m_meshLog->insertPlainText("Edges also get blurred with this filter.\n");

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

  float sigma = 1.0;

  bool ok;
  QString text = QInputDialog::getText(0, "Gaussian Sigma",
				       "Gaussian Sigma",
				       QLineEdit::Normal,
				       "1.0",
				       &ok);

  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	sigma = list[0].toFloat();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, m_nX*m_nY*m_nZ);

  typedef itk::SmoothingRecursiveGaussianImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetSigma(sigma);
  filter->SetInput( image );

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());

  memcpy(inVol, outVol, m_nX*m_nY*m_nZ);
}

bool
SmoothingFilter::checkBlend(Vec po, ushort v, uchar* lut)
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
SmoothingFilter::checkCrop(Vec po)
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
SmoothingFilter::checkPathBlend(Vec po, ushort v, uchar* lut)
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
SmoothingFilter::checkPathCrop(Vec po)
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

