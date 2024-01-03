#include "keyframeinformation.h"
#include "global.h"
#include "enums.h"

void KeyFrameInformation::setTitle(QString s) { m_title = s; }
void KeyFrameInformation::setDrawBox(bool flag) { m_drawBox = flag; }
void KeyFrameInformation::setDrawAxis(bool flag) { m_drawAxis = flag; }
void KeyFrameInformation::setShadowBox(bool b) { m_shadowBox = b; }
void KeyFrameInformation::setBackgroundColor(Vec col) { m_backgroundColor = col; }
void KeyFrameInformation::setBackgroundImageFile(QString fl) { m_backgroundImageFile = fl; }
void KeyFrameInformation::setFrameNumber(int fn) { m_frameNumber = fn; }
void KeyFrameInformation::setPosition(Vec pos) { m_position = pos; }
void KeyFrameInformation::setOrientation(Quaternion rot) { m_rotation = rot; }
void KeyFrameInformation::setLightInfo(LightingInformation li) { m_lightInfo = li; }
void KeyFrameInformation::setClipInfo(ClipInformation ci) { m_clipInfo = ci; }
void KeyFrameInformation::setImage(QImage pix) { m_image = pix; }
void KeyFrameInformation::setBrickInfo(QList<BrickInformation> bi)
{
  m_brickInfo.clear();
  m_brickInfo += bi;
}
void KeyFrameInformation::setCaptions(QList<CaptionObject> co) { m_captions = co; }
void KeyFrameInformation::setScaleBars(QList<ScaleBarObject> co) { m_scalebars = co; }
void KeyFrameInformation::setPoints(QList<Vec> pts, QList<Vec> bpts, int psz, Vec pcol)
{ m_points=pts; m_barepoints=bpts; m_pointSize=psz; m_pointColor=pcol;}
void KeyFrameInformation::setPaths(QList<PathObject> paths) { m_paths = paths; }
void KeyFrameInformation::setTrisets(QList<TrisetInformation> tinfo) { m_trisets = tinfo; }
void KeyFrameInformation::setTrisetsColors(QList<TrisetInformation> tinfo)
{
  for(int i=0; i<qMin(m_trisets.count(), tinfo.count()); i++)
    {
      m_trisets[i].color = tinfo[i].color;
    }
}
void KeyFrameInformation::setTrisetsLabels(QList<TrisetInformation> tinfo)
{
  for(int i=0; i<qMin(m_trisets.count(), tinfo.count()); i++)
    {
      m_trisets[i].captionText = tinfo[i].captionText;
      m_trisets[i].captionColor = tinfo[i].captionColor;
      m_trisets[i].captionFont = tinfo[i].captionFont;
      m_trisets[i].captionPosition = tinfo[i].captionPosition;
      m_trisets[i].cpDx = tinfo[i].cpDx;
      m_trisets[i].cpDy = tinfo[i].cpDy;
    }
}
void KeyFrameInformation::setGamma(float g)
{
  m_gamma = g;
}


QString KeyFrameInformation::title() { return m_title; }
bool KeyFrameInformation::drawBox() { return m_drawBox; }
bool KeyFrameInformation::drawAxis() { return m_drawAxis; }
bool KeyFrameInformation::shadowBox() { return m_shadowBox; }
Vec KeyFrameInformation::backgroundColor() { return m_backgroundColor; }
QString KeyFrameInformation::backgroundImageFile() { return m_backgroundImageFile; }
int KeyFrameInformation::frameNumber() { return m_frameNumber; }
Vec KeyFrameInformation::position() { return m_position; }
Quaternion KeyFrameInformation::orientation() { return m_rotation; }
LightingInformation KeyFrameInformation::lightInfo() { return m_lightInfo; }
ClipInformation KeyFrameInformation::clipInfo() { return m_clipInfo; }
QImage KeyFrameInformation::image() { return m_image; }
QList<BrickInformation> KeyFrameInformation::brickInfo() { return m_brickInfo; }
QList<CaptionObject> KeyFrameInformation::captions() { return m_captions; }
QList<ScaleBarObject> KeyFrameInformation::scalebars() { return m_scalebars; }
QList<Vec> KeyFrameInformation::points() { return m_points; }
QList<Vec> KeyFrameInformation::barepoints() { return m_barepoints; }
int KeyFrameInformation::pointSize() { return m_pointSize; }
Vec KeyFrameInformation::pointColor() { return m_pointColor; }
QList<PathObject> KeyFrameInformation::paths() { return m_paths; }
QList<TrisetInformation> KeyFrameInformation::trisets() { return m_trisets; }
float KeyFrameInformation::gamma() { return m_gamma; }


// -- keyframe interpolation parameters
void KeyFrameInformation::setInterpBGColor(int i) {m_interpBGColor = i;}
void KeyFrameInformation::setInterpCaptions(int i) {m_interpCaptions = i;}
void KeyFrameInformation::setInterpCameraPosition(int i) {m_interpCameraPosition = i;}
void KeyFrameInformation::setInterpCameraOrientation(int i) {m_interpCameraOrientation = i;}
void KeyFrameInformation::setInterpBrickInfo(int i) {m_interpBrickInfo = i;}
void KeyFrameInformation::setInterpClipInfo(int i) {m_interpClipInfo = i;}
void KeyFrameInformation::setInterpLightInfo(int i) {m_interpLightInfo = i;}

int KeyFrameInformation::interpBGColor() { return m_interpBGColor; }
int KeyFrameInformation::interpCaptions() { return m_interpCaptions; }
int KeyFrameInformation::interpCameraPosition() { return m_interpCameraPosition; }
int KeyFrameInformation::interpCameraOrientation() { return m_interpCameraOrientation; }
int KeyFrameInformation::interpBrickInfo() { return m_interpBrickInfo; }
int KeyFrameInformation::interpClipInfo() { return m_interpClipInfo; }
int KeyFrameInformation::interpLightInfo() { return m_interpLightInfo; }

bool
KeyFrameInformation::hasCaption(QStringList str)
{
  for(int i=0; i<m_captions.size(); i++)
    if (m_captions[i].hasCaption(str))
      return true;

  for(int i=0; i<m_paths.size(); i++)
    if (m_paths[i].hasCaption(str))
      return true;

  return false;
}

KeyFrameInformation::KeyFrameInformation()
{
  m_title.clear();
  m_drawBox = false;
  m_drawAxis = false;
  m_shadowBox = false;
  m_backgroundColor = Vec(0,0,0);
  m_backgroundImageFile.clear();
  m_frameNumber = 0;
  m_position = Vec(0,0,0);
  m_rotation = Quaternion(Vec(1,0,0), 0);
  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_clipInfo.clear();
  m_brickInfo.clear();
  m_captions.clear();
  m_scalebars.clear();
  m_points.clear();
  m_barepoints.clear();
  m_pointSize = 5;
  m_pointColor = Vec(0.0f, 0.5f, 1.0f);
  m_paths.clear();
  m_trisets.clear();
  m_gamma = 1.0;
  
  m_interpBGColor = Enums::KFIT_Linear;
  m_interpCaptions = Enums::KFIT_Linear;
  m_interpCameraPosition = Enums::KFIT_Linear;
  m_interpCameraOrientation = Enums::KFIT_Linear;
  m_interpBrickInfo = Enums::KFIT_Linear;
  m_interpClipInfo = Enums::KFIT_Linear;
  m_interpLightInfo = Enums::KFIT_Linear;
}

void
KeyFrameInformation::clear()
{
  m_title.clear();
  m_drawBox = false;
  m_drawAxis = false;
  m_shadowBox = false;
  m_backgroundColor = Vec(0,0,0);
  m_backgroundImageFile.clear();
  m_frameNumber = 0;
  m_position = Vec(0,0,0);
  m_rotation = Quaternion(Vec(1,0,0), 0);
  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_lightInfo.clear();
  m_clipInfo.clear();
  m_brickInfo.clear();
  m_captions.clear();
  m_scalebars.clear();
  m_points.clear();
  m_barepoints.clear();
  m_pointSize = 5;
  m_pointColor = Vec(0.0f, 0.5f, 1.0f);
  m_paths.clear();
  m_trisets.clear();
  m_gamma = 1.0;

  m_interpBGColor = Enums::KFIT_Linear;
  m_interpCaptions = Enums::KFIT_Linear;
  m_interpCameraPosition = Enums::KFIT_Linear;
  m_interpCameraOrientation = Enums::KFIT_Linear;
  m_interpBrickInfo = Enums::KFIT_Linear;
  m_interpClipInfo = Enums::KFIT_Linear;
  m_interpLightInfo = Enums::KFIT_Linear;
}

KeyFrameInformation::KeyFrameInformation(const KeyFrameInformation& kfi)
{
  m_title = kfi.m_title;

  m_drawBox = kfi.m_drawBox;
  m_drawAxis = kfi.m_drawAxis;

  m_shadowBox = kfi.m_shadowBox;
  m_backgroundColor = kfi.m_backgroundColor;
  m_backgroundImageFile = kfi.m_backgroundImageFile;

  m_frameNumber = kfi.m_frameNumber;

  m_position = kfi.m_position;
  m_rotation = kfi.m_rotation;

  m_lightInfo = kfi.m_lightInfo;
  m_clipInfo = kfi.m_clipInfo;

  m_image = kfi.m_image;

  m_brickInfo = kfi.m_brickInfo;

  m_captions = kfi.m_captions;
  m_scalebars = kfi.m_scalebars;
  m_points = kfi.m_points;
  m_barepoints = kfi.m_barepoints;
  m_pointSize = kfi.m_pointSize;
  m_pointColor = kfi.m_pointColor;
  m_paths = kfi.m_paths;
  m_trisets = kfi.m_trisets;

  m_gamma = kfi.m_gamma;

  
  m_interpBGColor = kfi.m_interpBGColor;
  m_interpCaptions = kfi.m_interpCaptions;
  m_interpCameraPosition = kfi.m_interpCameraPosition;
  m_interpCameraOrientation = kfi.m_interpCameraOrientation;
  m_interpBrickInfo = kfi.m_interpBrickInfo;
  m_interpClipInfo = kfi.m_interpClipInfo;
  m_interpLightInfo = kfi.m_interpLightInfo;
}

KeyFrameInformation::~KeyFrameInformation()
{
  m_title.clear();
  m_clipInfo.clear();
  m_brickInfo.clear();
  m_captions.clear();
  m_scalebars.clear();
  m_points.clear();
  m_barepoints.clear();
  m_paths.clear();
  m_trisets.clear();
}

KeyFrameInformation&
KeyFrameInformation::operator=(const KeyFrameInformation& kfi)
{
  m_title = kfi.m_title;

  m_drawBox = kfi.m_drawBox;
  m_drawAxis = kfi.m_drawAxis;

  m_shadowBox = kfi.m_shadowBox;
  m_backgroundColor = kfi.m_backgroundColor;
  m_backgroundImageFile = kfi.m_backgroundImageFile;

  m_frameNumber = kfi.m_frameNumber;

  m_position = kfi.m_position;
  m_rotation = kfi.m_rotation;

  m_lightInfo = kfi.m_lightInfo;
  m_clipInfo = kfi.m_clipInfo;

  m_image = kfi.m_image;

  m_brickInfo = kfi.m_brickInfo;

  m_captions = kfi.m_captions;
  m_scalebars = kfi.m_scalebars;
  m_points = kfi.m_points;
  m_barepoints = kfi.m_barepoints;
  m_pointSize = kfi.m_pointSize;
  m_pointColor = kfi.m_pointColor;
  m_paths = kfi.m_paths;
  m_trisets = kfi.m_trisets;

  m_gamma = kfi.m_gamma;

  
  m_interpBGColor = kfi.m_interpBGColor;
  m_interpCaptions = kfi.m_interpCaptions;
  m_interpCameraPosition = kfi.m_interpCameraPosition;
  m_interpCameraOrientation = kfi.m_interpCameraOrientation;
  m_interpBrickInfo = kfi.m_interpBrickInfo;
  m_interpClipInfo = kfi.m_interpClipInfo;
  m_interpLightInfo = kfi.m_interpLightInfo;

  return *this;
}

//--------------------------------
//---- load and save -------------
//--------------------------------

void
KeyFrameInformation::load(fstream &fin)
{
  bool done = false;
  char keyword[100];
  float f[3];

  m_title.clear();
  m_brickInfo.clear();
  m_gamma = 1.0;
  
  m_interpBGColor = Enums::KFIT_Linear;
  m_interpCaptions = Enums::KFIT_Linear;
  m_interpCameraPosition = Enums::KFIT_Linear;
  m_interpCameraOrientation = Enums::KFIT_Linear;
  m_interpBrickInfo = Enums::KFIT_Linear;
  m_interpClipInfo = Enums::KFIT_Linear;
  m_interpLightInfo = Enums::KFIT_Linear;

  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "keyframeend") == 0)
	done = true;
      else if (strcmp(keyword, "title") == 0)
	{
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  char *str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_title = QString(str);
	  delete [] str;
	}
      else if (strcmp(keyword, "brightness") == 0)
	fin.read((char*)&m_gamma, sizeof(float));
      else if (strcmp(keyword, "drawbox") == 0)
	fin.read((char*)&m_drawBox, sizeof(bool));
      else if (strcmp(keyword, "drawaxis") == 0)
	fin.read((char*)&m_drawAxis, sizeof(bool));
      else if (strcmp(keyword, "shadowbox") == 0)
	fin.read((char*)&m_shadowBox, sizeof(bool));
      else if (strcmp(keyword, "backgroundcolor") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_backgroundColor = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "backgroundimage") == 0)
	{
	  char *str;
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_backgroundImageFile = QString(str);
	  delete [] str;
	}
      else if (strcmp(keyword, "framenumber") == 0)
	{
	  fin.read((char*)&m_frameNumber, sizeof(int));
	  // frame numbers should be either -1 or greater than 0
	  if (m_frameNumber == 0 || m_frameNumber < -1)
	    m_frameNumber = 1;
	}
      else if (strcmp(keyword, "position") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_position = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "rotation") == 0)
	{
	  Vec axis;
	  float angle;
	  fin.read((char*)&f, 3*sizeof(float));
	  axis = Vec(f[0], f[1], f[2]);	
	  fin.read((char*)&angle, sizeof(float));
	  m_rotation.setAxisAngle(axis, angle);
	}
      else if (strcmp(keyword, "image") == 0)
	{
	  int n;
	  fin.read((char*)&n, sizeof(int));
	  unsigned char *tmp = new unsigned char[n+1];
	  fin.read((char*)tmp, n);
	  m_image = QImage::fromData(tmp, n);	 
	  delete [] tmp;
	}
      else if (strcmp(keyword, "lightinginformation") == 0)
	m_lightInfo.load(fin);
      else if (strcmp(keyword, "clipinformation") == 0)
	m_clipInfo.load(fin);
      else if (strcmp(keyword, "brickinformation") == 0)
	{
	  BrickInformation brickInfo;
	  brickInfo.load(fin);
	  m_brickInfo.append(brickInfo);
	}
      else if (strcmp(keyword, "captionobjectstart") == 0)
	{
	  CaptionObject co;
	  co.load(fin);
	  m_captions.append(co);
	}
      else if (strcmp(keyword, "scalebarobjectstart") == 0)
	{
	  ScaleBarObject so;
	  so.load(fin);
	  m_scalebars.append(so);
	}
      else if (strcmp(keyword, "pointsstart") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      Vec v;
	      float f[3];
	      fin.read((char*)&f, 3*sizeof(float));
	      v = Vec(f[0],f[1],f[2]);
	      m_points.append(v);
	    }
	  // "pointsend"
	  fin.getline(keyword, 100, 0);
	}
      else if (strcmp(keyword, "barepointsstart") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      Vec v;
	      float f[3];
	      fin.read((char*)&f, 3*sizeof(float));
	      v = Vec(f[0],f[1],f[2]);
	      m_barepoints.append(v);
	    }
	  // "barepointsend"
	  fin.getline(keyword, 100, 0);
	}
      else if (strcmp(keyword, "pointsize") == 0)
	{
	  int sz;
	  fin.read((char*)&sz, sizeof(int));
	  m_pointSize = sz;
	}
      else if (strcmp(keyword, "pointcolor") == 0)
	{
	  float f[3];
	  fin.read((char*)&f, 3*sizeof(float));
	  m_pointColor = Vec(f[0],f[1],f[2]);
	}
      else if (strcmp(keyword, "pathobjectstart") == 0)
	{
	  PathObject po;
	  po.load(fin);
	  m_paths.append(po);
	}
      else if (strcmp(keyword, "trisetinformation") == 0)
	{
	  TrisetInformation ti;
	  ti.load(fin);
	  m_trisets.append(ti);
	}
      else if (strcmp(keyword, "interpbgcolor") == 0)
	fin.read((char*)&m_interpBGColor, sizeof(int));
      else if (strcmp(keyword, "interpcaptions") == 0)
	fin.read((char*)&m_interpCaptions, sizeof(int));
      else if (strcmp(keyword, "interpcamerapos") == 0)
	fin.read((char*)&m_interpCameraPosition, sizeof(int));
      else if (strcmp(keyword, "interpcamerarot") == 0)
	fin.read((char*)&m_interpCameraOrientation, sizeof(int));
      else if (strcmp(keyword, "interpbrickinfo") == 0)
	fin.read((char*)&m_interpBrickInfo, sizeof(int));
      else if (strcmp(keyword, "interpclipinfo") == 0)
	fin.read((char*)&m_interpClipInfo, sizeof(int));
      else if (strcmp(keyword, "interplightinfo") == 0)
	fin.read((char*)&m_interpLightInfo, sizeof(int));
    }
}

void
KeyFrameInformation::save(fstream &fout)
{
  char keyword[100];
  float f[3];
  int len;

  memset(keyword, 0, 100);
  sprintf(keyword, "keyframestart");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "title");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_title.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_title.toLatin1().data(), len*sizeof(char));


  memset(keyword, 0, 100);
  sprintf(keyword, "framenumber");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_frameNumber, sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "brightness");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_gamma, sizeof(float));

  
  memset(keyword, 0, 100);
  sprintf(keyword, "drawbox");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_drawBox, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "drawaxis");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_drawAxis, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "shadowbox");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_shadowBox, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "backgroundcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_backgroundColor.x;
  f[1] = m_backgroundColor.y;
  f[2] = m_backgroundColor.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "backgroundimage");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_backgroundImageFile.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_backgroundImageFile.toLatin1().data(), len*sizeof(char));

  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_position.x;
  f[1] = m_position.y;
  f[2] = m_position.z;
  fout.write((char*)&f, 3*sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "rotation");
  fout.write((char*)keyword, strlen(keyword)+1);
  Vec axis;
  qreal angle;
  float fangle;
  m_rotation.getAxisAngle(axis, angle);
  f[0] = axis.x;
  f[1] = axis.y;
  f[2] = axis.z;
  fangle = angle;
  fout.write((char*)&f, 3*sizeof(float));
  fout.write((char*)&fangle, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "image");
  fout.write((char*)keyword, strlen(keyword)+1);
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  m_image.save(&buffer, "PNG");
  int n = bytes.size();
  fout.write((char*)&n, sizeof(int));
  fout.write((char*)bytes.data(), n);


  m_lightInfo.save(fout);
  m_clipInfo.save(fout);


  for(int i=0; i<m_brickInfo.size(); i++)
    m_brickInfo[i].save(fout);


  for(int i=0; i<m_captions.size(); i++)
    m_captions[i].save(fout);


  for(int i=0; i<m_scalebars.size(); i++)
    m_scalebars[i].save(fout);

  memset(keyword, 0, 100);
  sprintf(keyword, "pointsstart");
  fout.write((char*)keyword, strlen(keyword)+1);
  int npts = m_points.count();
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<m_points.size(); i++)
    {
      f[0] = m_points[i].x;
      f[1] = m_points[i].y;
      f[2] = m_points[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }
  memset(keyword, 0, 100);
  sprintf(keyword, "pointsend");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "barepointsstart");
  fout.write((char*)keyword, strlen(keyword)+1);
  npts = m_barepoints.count();
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<m_barepoints.size(); i++)
    {
      f[0] = m_barepoints[i].x;
      f[1] = m_barepoints[i].y;
      f[2] = m_barepoints[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }
  memset(keyword, 0, 100);
  sprintf(keyword, "barepointsend");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "pointsize");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_pointSize, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "pointcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_pointColor.x;
  f[1] = m_pointColor.y;
  f[2] = m_pointColor.z;
  fout.write((char*)&f, 3*sizeof(float));

  for(int i=0; i<m_paths.size(); i++)
    m_paths[i].save(fout);



  for(int i=0; i<m_trisets.size(); i++)
    m_trisets[i].save(fout);
  

  //------------------------------------------------------
  // -- write keyframe interpolation types
  //------------------------------------------------------

  memset(keyword, 0, 100);
  sprintf(keyword, "interpbgcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpBGColor, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interpcaptions");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpCaptions, sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "interpcamerapos");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpCameraPosition, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interpcamerarot");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpCameraOrientation, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interpbrickinfo");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpBrickInfo, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interpclipinfo");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpClipInfo, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interplightinfo");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpLightInfo, sizeof(int));

  //------------------------------------------------------


  memset(keyword, 0, 100);
  sprintf(keyword, "keyframeend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

