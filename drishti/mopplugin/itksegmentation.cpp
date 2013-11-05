#include "itksegmentation.h"
#include "itkImage.h"
#include "itkRelabelComponentImageFilter.h"
#include "itkConnectedComponentImageFilter.h"
#include "itkConnectedThresholdImageFilter.h"
#include "itkNeighborhoodConnectedImageFilter.h"
#include "itkConfidenceConnectedImageFilter.h"
#include "itkWatershedImageFilter.h"
#include "itkMorphologicalWatershedImageFilter.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "itkVotingBinaryIterativeHoleFillingImageFilter.h"

#include "itkConnectedThresholdImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkBinaryThinningImageFilter3D.h"

#include "itkRescaleIntensityImageFilter.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkCommand.h"

#include "propertyeditor.h"

QStringList
ITKSegmentation::registerPlugin()
{
  QStringList regString;

  regString << "ITK Plugins";

  return regString;
}

void
ITKSegmentation::init()
{
  m_nX = m_nY = m_nZ = 0;
  m_data = 0;
  m_points.clear();
  m_progress = 0;
  m_prog = 0;
}

void
ITKSegmentation::setData(int px, int py, int pz,
			 uchar* vol,
			 QList<Vec> pts)
{
  m_nX = px;
  m_nY = py;
  m_nZ = pz;
  m_data = vol;
  m_points = pts;
}

bool
ITKSegmentation::start()
{
  m_progress = new QProgressDialog("Applying ITK Filters",
				   QString(),
				   0, 100);
  return regionGrowing(m_nX, m_nY, m_nZ, m_data, m_points);
}

void
ITKSegmentation::next()
{
  m_progress->setValue(m_prog);
  m_prog = (m_prog+1)%100;
  qApp->processEvents();
}


bool
ITKSegmentation::regionGrowing(int nx, int ny, int nz, uchar* inVol,
			       QList<Vec> seeds)
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;
  vlist.clear();
  plist["command"] = vlist;

  vlist.clear();
  QFile helpFile(":/itk.help");
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
  keys << "command";
  keys << "commandhelp";

  propertyEditor.set("ITK Filters", plist, keys);
  
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    {
      QString cmd = propertyEditor.getCommandString();
      if (!cmd.isEmpty())
	{
	  QStringList list = cmd.toLower().split(" ", QString::SkipEmptyParts);	  
	  if (list.count() > 0)
	    {
	      if (list[0] == "skeletonize")
		{
		  binaryThinning(nx, ny, nz, inVol);
		}
	      else if (list[0] == "connectedcomponent")
		{
		  connectedComponent(nx, ny, nz, inVol);
		}
	      else
		{
		  int tag = 1;
		  if (list[0] == "connectedthreshold")
		    {
		      int lower = 5;
		      int upper = 255;
		      if (list.count() > 1)
			tag = list[1].toInt();
		      if (list.count() > 2)
			lower = list[2].toInt();
		      if (list.count() > 3)
			upper = list[3].toInt();

		      connectedThreshold(nx, ny, nz, inVol, seeds, tag,
						lower, upper);
		    }
		  else if (list[0] == "neighborhoodconnected")
		    {
		      int rad = 2;
		      int lower = 5;
		      int upper = 255;
		      if (list.count() > 1)
			tag = list[1].toInt();
		      if (list.count() > 2)
			lower = list[2].toInt();
		      if (list.count() > 3)
			upper = list[3].toInt();
		      if (list.count() > 4)
			rad = list[4].toInt();

		      neighborhoodConnected(nx, ny, nz, inVol, seeds, tag,
						   lower, upper, rad);
		    }
		  else if (list[0] == "confidenceconnected")
		    {
		      int rad = 2;
		      float multiplier = 2.5;
		      int niter = 5;
		      if (list.count() > 1)
			tag = list[1].toInt();
		      if (list.count() > 2)
			rad = list[2].toInt();
		      if (list.count() > 3)
			multiplier = list[3].toFloat();
		      if (list.count() > 4)
			niter = list[4].toInt();

		      confidenceConnected(nx, ny, nz, inVol, seeds, tag,
					  rad, multiplier, niter);
		    }
		}
	    }
	}
    }
  else
    {
      m_progress->setValue(100);
      return false;
    }

  m_progress->setValue(100);
  return true;
}

void
ITKSegmentation::connectedComponent(int nx, int ny, int nz, uchar* inVol)
{
  typedef itk::Image< uchar, 3 >  ImageType;
  typedef itk::Image< unsigned long, 3 >  ulImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;

  ImageType::RegionType region(start, size);

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, nx*ny*nz);

  try {
    typedef itk::ConnectedComponentImageFilter<ImageType, ulImageType> Filter;
    Filter::Pointer filter = Filter::New();
    filter->SetInput( image );  

    typedef itk::RelabelComponentImageFilter<ulImageType, ImageType> rFilter;
    rFilter::Pointer rfilter = rFilter::New();
    rfilter->SetInput(filter->GetOutput());  

    // set up progress update
    m_progress->setLabelText("Connected Components\nData taken from channel 1.\nResults returned in channel 2");
    m_prog = 0;
    typedef itk::SimpleMemberCommand<ITKSegmentation> CommandProgress;
    CommandProgress::Pointer progressbar = CommandProgress::New();
    progressbar->SetCallbackFunction(this, &ITKSegmentation::next);
    rfilter->AddObserver(itk::ProgressEvent(), progressbar);
    
    rfilter->Update();

    ImageType *dimg = rfilter->GetOutput();
    uchar *outVol = (uchar*)(dimg->GetBufferPointer());

    memcpy(inVol, outVol, nx*ny*nz);
  }
  catch (...)
    {
      QMessageBox::information(0, "", "Error : cannot run the filter");
    }
}

void
ITKSegmentation::connectedThreshold(int nx, int ny, int nz, uchar* inVol,
				    QList<Vec> seeds, int tag,
				    int lower, int upper)
{
  typedef itk::Image< uchar, 3 >  ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;

  ImageType::RegionType region(start, size);

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, nx*ny*nz);

  typedef itk::ConnectedThresholdImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetLower(lower);
  filter->SetUpper(upper);
  filter->SetReplaceValue(tag);
  filter->ClearSeeds();
  ImageType::IndexType  index;
  for (int i=0; i<seeds.count(); i++)
    {
      index[0] = seeds[i].x;
      index[1] = seeds[i].y;
      index[2] = seeds[i].z;
      filter->AddSeed( index );
    }
  
  // set up progress update
  m_progress->setLabelText("Connected Threshold\nData taken from channel 1.\nResults returned in channel 2");
  m_prog = 0;
  typedef itk::SimpleMemberCommand<ITKSegmentation> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &ITKSegmentation::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);
    
  filter->Update();

  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());

  memcpy(inVol, outVol, nx*ny*nz);
}

void
ITKSegmentation::neighborhoodConnected(int nx, int ny, int nz, uchar* inVol,
				       QList<Vec> seeds, int tag,
				       int lower, int upper, int rad)
{
  typedef itk::Image< uchar, 3 >  ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;

  ImageType::SizeType radius;
  radius[0] = radius[1] = radius[2] = rad;

  ImageType::RegionType region(start, size);

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, nx*ny*nz);

  typedef itk::NeighborhoodConnectedImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetRadius(radius);
  filter->SetLower(lower);
  filter->SetUpper(upper);
  filter->SetReplaceValue(tag);
  filter->ClearSeeds();
  ImageType::IndexType  index;
  for (int i=0; i<seeds.count(); i++)
    {
      index[0] = seeds[i].x;
      index[1] = seeds[i].y;
      index[2] = seeds[i].z;
      filter->AddSeed( index );
    }

  // set up progress update
  m_progress->setLabelText("Neighborhood Connected\nData taken from channel 1.\nResults returned in channel 2");
  m_prog = 0;
  typedef itk::SimpleMemberCommand<ITKSegmentation> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &ITKSegmentation::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);
    
  filter->Update();

  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());

  memcpy(inVol, outVol, nx*ny*nz);
}

void
ITKSegmentation::confidenceConnected(int nx, int ny, int nz, uchar* inVol,
				     QList<Vec> seeds, int tag,
				     int rad, float multiplier, int niter)
{
  typedef itk::Image< uchar, 3 >  ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;

  ImageType::SizeType radius;
  radius[0] = radius[1] = radius[2] = rad;

  ImageType::RegionType region(start, size);

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, nx*ny*nz);

  typedef itk::ConfidenceConnectedImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetInitialNeighborhoodRadius(rad);
  filter->SetMultiplier(multiplier);
  filter->SetNumberOfIterations(niter);
  filter->SetReplaceValue(tag);
  filter->ClearSeeds();
  ImageType::IndexType  index;
  for (int i=0; i<seeds.count(); i++)
    {
      index[0] = seeds[i].x;
      index[1] = seeds[i].y;
      index[2] = seeds[i].z;
      filter->AddSeed( index );
    }
  
  // set up progress update
  m_progress->setLabelText("Confidence Connected\nData taken from channel 1.\nResults returned in channel 2");
  m_prog = 0;
  typedef itk::SimpleMemberCommand<ITKSegmentation> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &ITKSegmentation::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);
    
  filter->Update();

  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());

  memcpy(inVol, outVol, nx*ny*nz);
}

void
ITKSegmentation::watershed(int nx, int ny, int nz, uchar* inVol,
			   QList<Vec> seeds)
{
  bool ok;
  double threshold = 0.01;
  double level = 0.25;
  QString text = QInputDialog::getText(0, "Watershed Filter Bounds",
				       "Using opacity values for segmentation\nThreshold and Level",
				       QLineEdit::Normal,
				       QString("%1 %2").arg(threshold).arg(level),
				       &ok);
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	threshold = list[0].toFloat();
      if (list.count() > 1)
	level = list[1].toFloat();
    }

  float *floatVol = new float[nx*ny*nz];
  for(int i=0; i<nx*ny*nz; i++)
    floatVol[i] = (float)inVol[i]/255.0f;

  typedef itk::Image<uchar, 3 >  ImageType;
  typedef itk::Image<uchar, 3 >  ucImageType;
  typedef itk::Image<unsigned long, 3>  watershedImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;

  ImageType::RegionType region(start, size);

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, nx*ny*nz);
//  float *iptr = (float*)image->GetBufferPointer();
//  memcpy(iptr, floatVol, 4*nx*ny*nz);
//  delete [] floatVol;

  // find gradient magnitude to be used as height field
  typedef itk::GradientMagnitudeImageFilter<ImageType, ImageType> gmFilter;
  gmFilter::Pointer gmfilter = gmFilter::New();
  gmfilter->SetInput( image );
  gmfilter->Update();
  
  // pass this as input to watershed
  //typedef itk::MorphologicalWatershedImageFilter<ImageType, watershedImageType> Filter;
  typedef itk::WatershedImageFilter<ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetLevel(level);
  filter->SetInput(gmfilter->GetOutput());

  // set up progress update
  m_progress->setLabelText("Watershed\nData taken from channel 1.\nResults returned in channel 2");
  m_prog = 0;
  typedef itk::SimpleMemberCommand<ITKSegmentation> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &ITKSegmentation::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);
    
  filter->Update();


  // find min and max values of the labelled image
  typedef itk::MinimumMaximumImageCalculator <watershedImageType>
          ImageCalculatorFilterType; 
  ImageCalculatorFilterType::Pointer imageCalculatorFilter
          = ImageCalculatorFilterType::New ();
  imageCalculatorFilter->SetImage(filter->GetOutput());
  imageCalculatorFilter->Compute();
  unsigned long vmin, vmax;
  vmin = imageCalculatorFilter->GetMinimum();
  vmax = imageCalculatorFilter->GetMaximum();
  
  watershedImageType *wimg = filter->GetOutput();
  unsigned long *outVol = (unsigned long*)(wimg->GetBufferPointer());

  for(int i=0; i<nx*ny*nz; i++)
    {
      if (outVol[1] > 0)
	inVol[i] = 1 + 254*(float)(outVol[i])/vmax;
      else
	inVol[i] = 0;
    }
}

void
ITKSegmentation::binaryThinning(int nx, int ny, int nz, uchar* inVol)
{
  typedef itk::Image< uchar, 3 >  ImageType;
  typedef itk::Image< unsigned long, 3 >  ulImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;

  ImageType::RegionType region(start, size);

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, nx*ny*nz);

  try {
    typedef itk::BinaryThinningImageFilter3D< ImageType, ImageType > ThinningFilterType;
    ThinningFilterType::Pointer thinningFilter = ThinningFilterType::New();
    thinningFilter->SetInput( image );

    // set up progress update
    m_progress->setLabelText("Binary Thinning\nData taken from channel 1.\nResults returned in channel 2");
    m_prog = 0;
    typedef itk::SimpleMemberCommand<ITKSegmentation> CommandProgress;
    CommandProgress::Pointer progressbar = CommandProgress::New();
    progressbar->SetCallbackFunction(this, &ITKSegmentation::next);
    thinningFilter->AddObserver(itk::ProgressEvent(), progressbar);
    
    thinningFilter->Update();

    ImageType *dimg = thinningFilter->GetOutput();
    uchar *outVol = (uchar*)(dimg->GetBufferPointer());

    memcpy(inVol, outVol, nx*ny*nz);
  }
  catch (...)
    {
      QMessageBox::information(0, "", "Error : cannot run the filter");
    }
}

//-------------------------------
//-------------------------------
Q_EXPORT_PLUGIN2(itkplugin, ITKSegmentation);
