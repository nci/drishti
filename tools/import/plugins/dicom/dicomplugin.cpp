#include <QtGui>
#include "common.h"
#include "dicomplugin.h"

#include <QTextEdit>
#include <QVBoxLayout>

void DicomPlugin::generateHistogram() {} // to satisfy the interface

QStringList
DicomPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "DICOM Image Directory";
  //  regString << "files";
  //  regString << "DICOM Image Files";
  
  return regString;
}

void
DicomPlugin::init()
{
  m_dimg = 0;

  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
DicomPlugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
DicomPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
DicomPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString DicomPlugin::description() { return m_description; }
int DicomPlugin::voxelType() { return m_voxelType; }
int DicomPlugin::voxelUnit() { return m_voxelUnit; }
int DicomPlugin::headerBytes() { return m_headerBytes; }

void
DicomPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float DicomPlugin::rawMin() { return m_rawMin; }
float DicomPlugin::rawMax() { return m_rawMax; }
QList<uint> DicomPlugin::histogram() { return m_histogram; }

void
DicomPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
DicomPlugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;
}

bool
DicomPlugin::setFile(QStringList files)
{
  if (files.size() == 0)
    return false;

  //-------------------------
  // traverse to the sub directory which contains the .dcm files
  QString flnm0 = files[0];
  {
    QDirIterator it(flnm0, QDirIterator::Subdirectories);
    while (it.hasNext())
      {
	QFileInfo f(it.next());
	if (f.isFile() && f.suffix().toLower() == "dcm")
	  {
	    flnm0 = f.canonicalPath();
	    break;
	  }
      }
  }  
  //-------------------------

  //m_fileName = files;
  m_fileName << flnm0;

  m_imageList.clear();

  QFileInfo f(m_fileName[0]);
  if (f.isDir())
    {
      typedef itk::GDCMImageIO ImageIOType;
      ImageIOType::Pointer dicomIO = ImageIOType::New();

      typedef std::vector< std::string > FileNamesContainer;
      FileNamesContainer dcmFiles;

      typedef itk::GDCMSeriesFileNames NamesGeneratorType;
      NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();

      nameGenerator->SetUseSeriesDetails( true );

      nameGenerator->SetDirectory(m_fileName[0].toUtf8().data() );

      typedef std::vector< std::string >    SeriesIdContainer;      
      const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();

      std::string seriesIdentifier;

      if (seriesUID.size() == 0)
	{
	  QMessageBox::information(0, "", "No Dicom series found !");
	  return false;
	}

      //---------------------------
      // give the user to select a series, if there are more than one in the directory 
      if (seriesUID.size() == 1)
	{
	  seriesIdentifier = seriesUID.begin()->c_str();
	}
      else
	{
	  QStringList varNames;      
	  bool ok;
	  QString varName;  
	  int nser = seriesUID.size();
	  for(int i=0; i<nser; i++)
	    varNames << QString(seriesUID[i].c_str());
	  
	  varName = QInputDialog::getItem(0,
					  "Choose a series for extraction",
					  "Series",
					  varNames,
					  0,
					  false,
					  &ok);
	  if (!ok)
	    {
	      seriesIdentifier = seriesUID[0].c_str();
	      QMessageBox::information(0, "", "Loading series "+varNames[0]);
	    }
	  else
	    {
	      int sel = varNames.indexOf(varName);
	      seriesIdentifier = seriesUID[sel].c_str();
	    }
	}
      //---------------------------

      dcmFiles = nameGenerator->GetFileNames( seriesIdentifier );

      m_reader = ReaderType::New();
      m_reader->SetImageIO( dicomIO );
      m_reader->SetFileNames( dcmFiles );
      m_reader->Update();



//      //-----------------
//      {
//	QTextEdit *tedit = new QTextEdit();
//	typedef itk::MetaDataDictionary   DictionaryType;
//	const  DictionaryType & dictionary = dicomIO->GetMetaDataDictionary();
//	typedef itk::MetaDataObject< std::string > MetaDataStringType;
//	DictionaryType::ConstIterator itr = dictionary.Begin();
//	DictionaryType::ConstIterator end = dictionary.End();	
//	while( itr != end )
//	  {
//	    itk::MetaDataObjectBase::Pointer  entry = itr->second;
//	    MetaDataStringType::Pointer entryvalue =
//	      dynamic_cast<MetaDataStringType *>( entry.GetPointer() );
//	    if( entryvalue )
//	      {
//		std::string tagkey   = itr->first;
//		std::string tagvalue = entryvalue->GetMetaDataObjectValue();
//		tedit->insertPlainText(QString(tagkey.c_str()) + " = " + QString(tagvalue.c_str()) + "\n");
//	      }
//	    ++itr;
//	  }
//
//	
//	std::string tagkey;
//	std::string labelId;
//	std::string tagvalue;
//	{
//	  tagkey = "0028|0030"; // pixel spacing
//	  if (itk::GDCMImageIO::GetLabelFromTag(tagkey, labelId))
//	    {
//	      if (dicomIO->GetValueFromTag(tagkey, tagvalue))
//		{
//		  tedit->insertPlainText(QString(labelId.c_str()) + " = " + QString(tagvalue.c_str()) + "\n");
//		}
//	      else
//		{
//		  tedit->insertPlainText("\n\Pixel Spacing not available\n\n");
//		}
//	    }
//	  //}
//	  //{
//	  tagkey = "0018|0050"; // slice thickness
//	  if (itk::GDCMImageIO::GetLabelFromTag(tagkey, labelId))
//	    {
//	      if (dicomIO->GetValueFromTag(tagkey, tagvalue))
//		{
//		  tedit->insertPlainText(QString(labelId.c_str()) + " = " + QString(tagvalue.c_str()) + "\n");
//		}
//	      else
//		{
//		  tedit->insertPlainText("\n\nSlice Thickness not available\n\n");
//		}
//	    }
//	  //}	
//	  //{
//	  tagkey = "0020|0032"; // upper left corner
//	  if (itk::GDCMImageIO::GetLabelFromTag(tagkey, labelId))
//	    {
//	      if (dicomIO->GetValueFromTag(tagkey, tagvalue))
//		{
//		  tedit->insertPlainText(QString(labelId.c_str()) + " = " + QString(tagvalue.c_str()) + "\n");
//		}
//	      else
//		{
//		  tedit->insertPlainText("\n\Upper Left Corner not available\n\n");
//		}
//	    }	
//	}
//	
//	  
//	QVBoxLayout *layout = new QVBoxLayout();
//	layout->addWidget(tedit);
//	
//	QDialog *showMD = new QDialog();
//	showMD->setWindowTitle("Series MetaData");
//	showMD->setSizeGripEnabled(true);
//	showMD->setModal(true);
//	showMD->setLayout(layout);
//	showMD->exec();
//      }
//      //-----------------



      
      m_dimg = m_reader->GetOutput();
      
      m_voxelType = _Short;
  
      ImageType::SizeType imageSize = m_dimg->GetLargestPossibleRegion().GetSize();    
      m_height = imageSize[0];
      m_width = imageSize[1];
      m_depth = imageSize[2];

      ImageType::SpacingType imageSpacing = m_dimg->GetSpacing();    
      m_voxelSizeZ = imageSpacing[2];
      m_voxelSizeY = imageSpacing[1];
      m_voxelSizeX = imageSpacing[0];

      m_headerBytes = 0;
      m_bytesPerVoxel = 2;

      findMinMaxandGenerateHistogram();
    }
  else
    {
      QMessageBox::information(0, "Error", "Expecting DICOM Directory");
      return false;
    }

  return true;
}

#define MINMAXANDHISTOGRAM()				\
  {							\
    for(uint j=0; j<m_width*m_height; j++)		\
      {							\
	int val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, (float)val);		\
	m_rawMax = qMax(m_rawMax, (float)val);		\
							\
	int idx = val-rMin;				\
	m_histogram[idx]++;				\
      }							\
  }


void
DicomPlugin::findMinMaxandGenerateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   0,
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  float rSize;
  float rMin;
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      if (m_voxelType == _UChar) rMin = 0;
      if (m_voxelType == _Char) rMin = -127;
      rSize = 255;
      for(uint i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
	   m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32767;
      rSize = 65535;
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      QMessageBox::information(0, "Error", "Why am i here ???");
      return;
    }

  int nbytes = m_width*m_height*m_bytesPerVoxel;


  m_rawMin = 10000000;
  m_rawMax = -10000000;

  itk::Size<3> size;
  size[0] = m_height; size[1] = m_width; size[2] = 1;
  itk::Index<3> index;
  index[0] = 0; index[1] = 0;
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      index[2] = i; 
      typedef itk::ImageRegion<3> RegionType;
      RegionType region(index,size);

      typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
      FilterType::Pointer filter = FilterType::New();
      filter->SetInput(m_dimg);
      filter->SetExtractionRegion(region);
      filter->SetDirectionCollapseToIdentity();
      filter->Update(); 
      char *tmp = (char*)(filter->GetOutput()->GetBufferPointer());

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = (uchar*)tmp;
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = (char*) tmp;
	  MINMAXANDHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = (ushort*) tmp;
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = (short*) tmp;
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = (int*) tmp;
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = (float*) tmp;
	  MINMAXANDHISTOGRAM();
	}
    }

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  qApp->processEvents();

}

void
DicomPlugin::getDepthSlice(int slc,
			   uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;

  itk::Size<3> size;
  itk::Index<3> index;
  size[0] = m_height; size[1] = m_width; size[2] = 1;
  index[0] = 0; index[1] = 0; index[2] = slc; 
  typedef itk::ImageRegion<3> RegionType;
  RegionType region(index,size);

  typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
  FilterType::Pointer filter = FilterType::New();
  filter->SetExtractionRegion(region);
  filter->SetInput(m_dimg);
  filter->SetDirectionCollapseToIdentity();
  filter->Update(); 
  ImageType *output = filter->GetOutput();
  char *tmp = (char*)(output->GetBufferPointer());

  memcpy(slice, tmp, nbytes);
}

//void
//DicomPlugin::getWidthSlice(int slc,
//			   uchar *slice)
//{
//  itk::Size<3> size;
//  size[0] = m_height; size[1] = 1; size[2] = m_depth;
//  itk::Index<3> index;
//  index[0] = 0; index[1] = slc;
//  index[2] = 0; 
//  typedef itk::ImageRegion<3> RegionType;
//  RegionType region(index,size);
//
//  typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
//  FilterType::Pointer filter = FilterType::New();
//  filter->SetExtractionRegion(region);
//  filter->SetInput(m_dimg);
//  filter->SetDirectionCollapseToIdentity();
//  filter->Update(); 
//  ImageType *output = filter->GetOutput();
//  char *tmp = (char*)(output->GetBufferPointer());
//
//  memcpy(slice, tmp, m_depth*m_height*m_bytesPerVoxel);
//}
//
//void
//DicomPlugin::getHeightSlice(int slc,
//			     uchar *slice)
//{
//  itk::Size<3> size;
//  size[0] = 1; size[1] = m_width; size[2] = m_depth;
//  itk::Index<3> index;
//  index[0] = slc; index[1] = 0;
//  index[2] = 0; 
//  typedef itk::ImageRegion<3> RegionType;
//  RegionType region(index,size);
//
//  typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
//  FilterType::Pointer filter = FilterType::New();
//  filter->SetExtractionRegion(region);
//  filter->SetInput(m_dimg);
//  filter->SetDirectionCollapseToIdentity();
//  filter->Update(); 
//  ImageType *output = filter->GetOutput();
//  char *tmp = (char*)(output->GetBufferPointer());
//
//  memcpy(slice, tmp, m_width*m_depth*m_bytesPerVoxel);
//}

QVariant
DicomPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }


  itk::Size<3> size;
  size[0] = 1; size[1] = 1; size[2] = 1;
  itk::Index<3> index;
  index[0] = h; index[1] = w;
  index[2] = d; 
  typedef itk::ImageRegion<3> RegionType;
  RegionType region(index,size);

  typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
  FilterType::Pointer filter = FilterType::New();
  filter->SetExtractionRegion(region);
  filter->SetInput(m_dimg);
  filter->SetDirectionCollapseToIdentity();
  filter->Update(); 
  ImageType *output = filter->GetOutput();
  char *tmp = (char*)(output->GetBufferPointer());

  if (m_voxelType == _UChar)
    {
      uchar a = *tmp;
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char a = *tmp;
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      ushort a = *(ushort*)tmp;
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      ushort a = *(short*)tmp;
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a = *(float*)tmp;
      v = QVariant((double)a);
    }

  return v;
}

//void
//DicomPlugin::saveTrimmed(QString trimFile,
//			    int dmin, int dmax,
//			    int wmin, int wmax,
//			    int hmin, int hmax)
//{
//  QProgressDialog progress("Saving trimmed volume",
//			   0,
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);
//
//  int nX, nY, nZ;
//  nX = m_depth;
//  nY = m_width;
//  nZ = m_height;
//
//  int mX, mY, mZ;
//  mX = dmax-dmin+1;
//  mY = wmax-wmin+1;
//  mZ = hmax-hmin+1;
//
//  int nbytes = nY*nZ*m_bytesPerVoxel;
//  uchar *tmp = new uchar[nbytes];
//
//  uchar vt;
//  if (m_voxelType == _UChar) vt = 0; // unsigned byte
//  if (m_voxelType == _Char) vt = 1; // signed byte
//  if (m_voxelType == _UShort) vt = 2; // unsigned short
//  if (m_voxelType == _Short) vt = 3; // signed short
//  if (m_voxelType == _Int) vt = 4; // int
//  if (m_voxelType == _Float) vt = 8; // float
//  
//  QFile fout(trimFile);
//  fout.open(QFile::WriteOnly);
//
//  fout.write((char*)&vt, 1);
//  fout.write((char*)&mX, 4);
//  fout.write((char*)&mY, 4);
//  fout.write((char*)&mZ, 4);
//
//  itk::Size<3> size;
//  size[0] = m_height; size[1] = m_width; size[2] = 1;
//  itk::Index<3> index;
//  index[0] = 0; index[1] = 0;
//
//  for(int i=dmax; i>=dmin; i--)
//    {
//      index[2] = i; 
//      typedef itk::ImageRegion<3> RegionType;
//      RegionType region(index,size);
//
//      typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
//      FilterType::Pointer filter = FilterType::New();
//      filter->SetExtractionRegion(region);
//      filter->SetInput(m_dimg);
//      filter->SetDirectionCollapseToIdentity();
//      filter->Update(); 
//      ImageType *output = filter->GetOutput();
//      char *tmp1 = (char*)(output->GetBufferPointer());
//
//      memcpy(tmp, tmp1, nbytes);
//      
//      for(uint j=wmin; j<=wmax; j++)
//	{
//	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
//		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
//		 mZ*m_bytesPerVoxel);
//	}
//	  
//      fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);
//
//      progress.setValue((int)(100*(float)(dmax-i)/(float)mX));
//      qApp->processEvents();
//    }
//
//  fout.close();
//
//  delete [] tmp;
//
//  progress.setValue(100);
//
//  m_headerBytes = 13; // to be used for applyMapping function
//}
