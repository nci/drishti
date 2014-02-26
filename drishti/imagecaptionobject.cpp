#include "global.h"
#include "imagecaptionobject.h"
#include "volumeinformation.h"

#include <QTextDocument>
#include <QTextStream>
#include <QApplication>
#include <QDesktopWidget>

int ImageCaptionObject::height() { return m_height; }
int ImageCaptionObject::width() { return m_width; }

ImageCaptionObject::ImageCaptionObject() { m_label = 0; m_videoPlayer = 0; clear(); }
ImageCaptionObject::~ImageCaptionObject() { clear(); }
void
ImageCaptionObject::clear()
{  
  m_redo = true;

  if (m_label) delete m_label;
  m_label = 0;

  if (m_videoPlayer) delete m_videoPlayer;
  m_videoPlayer = 0;

  m_active = false;
  m_pos = Vec(0,0,0);
  m_imageFile.clear();
  
  m_height = 0;
  m_width = 0;
}


void
ImageCaptionObject::saveSize()
{
  if (m_videoPlayer)
    {
      m_width = m_videoPlayer->width();
      m_height = m_videoPlayer->height();
    }
  else
    {
      m_width = m_label->width();
      m_height = m_label->height();
    }
  QMessageBox::information(0, "", QString("Caption size saved : %1 %2").arg(m_width).arg(m_height));
}

void
ImageCaptionObject::set(Vec pos, QString imgfl)
{
  clear();

  m_pos = pos;
  m_imageFile = imgfl;
}

void
ImageCaptionObject::set(Vec pos, QString imgfl, int wd, int ht)
{
  clear();

  m_pos = pos;
  m_imageFile = imgfl;
  m_width = wd;
  m_height = ht;
}

void
ImageCaptionObject::setImageCaption(ImageCaptionObject co)
{
  clear();

  m_pos = co.position();
  m_imageFile = co.imageFile();
  m_width = co.width();
  m_height = co.height();
}

void ImageCaptionObject::setPosition(Vec pos) { m_pos = pos; }

void
ImageCaptionObject::setImageFileName(QString imgfl)
{
  m_redo = true;

  m_imageFile = imgfl;
}

void
ImageCaptionObject::setActive(bool b)
{
  m_active = b;

  if (m_active)
    {
      if (m_redo) loadCaption();
	
      //--------------------------------------------
      // -- reposition the caption
      QDesktopWidget *dw = QApplication::desktop();
      int swd = dw->width();
      int sht = dw->height();
      QPoint p = QCursor::pos();

      int xpos = p.x()+20;
      if (xpos + m_width > swd)
	xpos = qMax(0, swd-m_width-20);

      int ypos = p.y() - m_height/2;
      if (ypos < 0) ypos = 0;
      if (ypos + m_height > sht)
	ypos = qMax(0, sht-m_height-20);      

      setCaptionPosition(xpos, ypos);
      //--------------------------------------------
	
      if (m_videoPlayer)
	{
	  m_videoPlayer->resize(m_width, m_height);
	  m_videoPlayer->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
	  m_videoPlayer->show();
	  m_videoPlayer->play();
	  return;
	}
      else if (m_label)
	{
	  m_label->resize(m_width, m_height);
	  m_label->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
	  m_label->show();
	}
    }
  else
    {
      if (m_label) m_label->hide();
      if (m_videoPlayer) m_videoPlayer->hide();
    }
}

void
ImageCaptionObject::setCaptionPosition(int xpos, int ypos)
{
  if (m_videoPlayer)
    m_videoPlayer->move(xpos, ypos);  
  if (m_label)
    m_label->move(xpos, ypos);  
}

void
ImageCaptionObject::loadCaption()
{
  m_redo = false;

  if (m_videoPlayer) delete m_videoPlayer;
  m_videoPlayer = 0;

  if (m_label) delete m_label;
  m_label = new QTextEdit();

  //----------------
  // file is assumed to be relative to .pvl.nc file
  // get the absolute path
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  QFileInfo fileInfo(pvlInfo.pvlFile);
  QString absoluteFilename = QFileInfo(fileInfo.absolutePath(),
				       m_imageFile).absoluteFilePath();
  //----------------
  
  QFileInfo f(absoluteFilename);
  
  if (m_imageFile.endsWith("txt") || m_imageFile.endsWith("html"))
    {
      QFile infl(absoluteFilename);
      infl.open(QFile::ReadOnly);
      QTextStream infile(&infl);
      QString fileText = infile.readAll();
      m_label->setText(fileText);      
      
      if (m_width == 0)
	{
	  m_width = m_label->width();
	  m_height = m_label->height();
	}
    }
  else
    {
      if (!m_imageFile.endsWith("wmv"))
	{
	  m_label->setHtml("<img src = " + absoluteFilename + ">");
	  if (m_width == 0)
	    {
	      m_width = m_label->width();
	      m_height = m_label->height();
	    }
	}
      else
	{
	  m_videoPlayer = new VideoPlayer();
	  m_videoPlayer->setFile(absoluteFilename);
	  if (m_width == 0)
	    {
	      m_width = m_videoPlayer->width();
	      m_height = m_videoPlayer->height();
	    }
	}
    }

  m_label->setReadOnly(true);
}

//  QFile infl(absoluteFile);
//  infl.open(QFile::ReadOnly);
//  QTextStream infile(&infl);
//  QString fileText = infile.readAll();
//  QTextDocument doc(fileText);
//
//  QImage cimage(500, 500, QImage::Format_ARGB32);
//  QPainter p(&cimage);
//  p.setRenderHint(QPainter::Antialiasing, true);
//  p.setPen(Qt::white);
//  p.setFont(QFont("Arial", 14));
//  p.drawText(1,1, 500, 500, Qt::AlignCenter|Qt::TextWordWrap, fileText);
//  //doc.drawContents(&p);
//
//  m_image = QImage(cimage).mirrored(false, true);
//
//  m_height = m_image.height();
//  m_width = m_image.width();
//}

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
      else if (strcmp(keyword, "size") == 0)
	{
	  fin.read((char*)&m_width, sizeof(int));
	  fin.read((char*)&m_height, sizeof(int));
	}
    }
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

  sprintf(keyword, "size");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_width, sizeof(int));
  fout.write((char*)&m_height, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "imagecaptionobjectend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

