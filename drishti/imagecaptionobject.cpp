#include "global.h"
#include "imagecaptionobject.h"
#include "volumeinformation.h"

ImageCaptionObject::ImageCaptionObject() { clear(); }
ImageCaptionObject::~ImageCaptionObject() { clear(); }
void
ImageCaptionObject::clear()
{
  m_active = false;
  m_image = QImage();
  m_pos = Vec(0,0,0);
  m_imageFile.clear();
  
  m_height = 0;
  m_width = 0;
}

void
ImageCaptionObject::set(Vec pos, QString imgfl)
{
  m_pos = pos;
  m_imageFile = imgfl;

  loadImage();
}

void ImageCaptionObject::setPosition(Vec pos) { m_pos = pos; }

void
ImageCaptionObject::setImageFileName(QString imgfl)
{
  m_imageFile = imgfl;
  loadImage();
}

void
ImageCaptionObject::loadImage()
{
  m_active = false;

  //----------------
  // file is assumed to be relative to .pvl.nc file
  // get the absolute path
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  QFileInfo fileInfo(pvlInfo.pvlFile);
  QString absoluteImageFile = QFileInfo(fileInfo.absolutePath(),
					m_imageFile).absoluteFilePath();
  //----------------

  QFileInfo f(absoluteImageFile);
  if (f.exists() == false)
    {
      m_height = m_width = 10;      
      return;
    }

  QMovie movie(absoluteImageFile);
  movie.setCacheMode(QMovie::CacheAll);
  movie.start();
  movie.setPaused(true);
  movie.jumpToFrame(0);

  m_image = QImage(movie.currentImage()).mirrored(false, true);
  m_height = m_image.height();
  m_width = m_image.width();
}

void
ImageCaptionObject::setImageCaption(ImageCaptionObject co)
{
  m_pos = co.position();
  m_imageFile = co.imageFile();

  loadImage();
}

void
ImageCaptionObject::load(fstream& fin)
{
  clear();

  int len;
  bool done = false;
  char keyword[100];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "imagecaptionobjectend") == 0)
	done = true;
      else if (strcmp(keyword, "position") == 0)
	{
	  float x, y, z;
	  fin.read((char*)&x, sizeof(float));
	  fin.read((char*)&y, sizeof(float));
	  fin.read((char*)&z, sizeof(float));
	  m_pos = Vec(x,y,z);
	}
      else if (strcmp(keyword, "imagefile") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  char *str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_imageFile = QString(str);
	  delete [] str;
	}
    }
  loadImage();
}

void
ImageCaptionObject::save(fstream& fout)
{
  char keyword[100];
  int len;

  memset(keyword, 0, 100);
  sprintf(keyword, "imagecaptionobjectstart");
  fout.write((char*)keyword, strlen(keyword)+1);

  float x = m_pos.x;
  float y = m_pos.y;
  float z = m_pos.z;
  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&x, sizeof(float));
  fout.write((char*)&y, sizeof(float));
  fout.write((char*)&z, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "imagefile");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_imageFile.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_imageFile.toLatin1().data(), len*sizeof(char));

  memset(keyword, 0, 100);
  sprintf(keyword, "imagecaptionobjectend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

