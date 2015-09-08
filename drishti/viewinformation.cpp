#include "glewinitialisation.h"
#include "viewinformation.h"

void ViewInformation::setVolumeNumber(int vn) { m_volumeNumber = vn; }
void ViewInformation::setVolumeNumber2(int vn) { m_volumeNumber2 = vn; }
void ViewInformation::setVolumeNumber3(int vn) { m_volumeNumber3 = vn; }
void ViewInformation::setVolumeNumber4(int vn) { m_volumeNumber4 = vn; }
void ViewInformation::setStepsizeStill(float ss) { m_stepsizeStill = ss; }
void ViewInformation::setStepsizeDrag(float sd) { m_stepsizeDrag = sd; }
void ViewInformation::setDrawBox(bool db) { m_drawBox = db; }
void ViewInformation::setDrawAxis(bool db) { m_drawAxis = db; }
void ViewInformation::setBackgroundColor(Vec col) { m_backgroundColor = col; }
void ViewInformation::setBackgroundImageFile(QString fl) { m_backgroundImageFile = fl; }
void ViewInformation::setPosition(Vec pos) { m_position = pos; }
void ViewInformation::setOrientation(Quaternion rot) { m_rotation = rot; }
void ViewInformation::setFocusDistance(float fd) { m_focusDistance = fd; }
void ViewInformation::setImage(QImage pix) { m_image = pix; }
void ViewInformation::setRenderQuality(int rq) { m_renderQuality = rq; }
void ViewInformation::setLightInfo(LightingInformation li) { m_lightInfo = li; }
void ViewInformation::setClipInfo(ClipInformation ci) { m_clipInfo = ci; }
void ViewInformation::setBrickInfo(QList<BrickInformation> bi) { m_brickInfo = bi; }
void ViewInformation::setVolumeBounds(Vec bmin, Vec bmax) { m_volMin = bmin; m_volMax = bmax; }
void ViewInformation::setSplineInfo(QList<SplineInformation> si) { m_splineInfo = si; }
void ViewInformation::setTick(int sz, int st,
			       QString xl, QString yl, QString zl)
{
  m_tickSize = sz;
  m_tickStep = st;
  m_labelX = xl;
  m_labelY = yl;
  m_labelZ = zl;
}
void ViewInformation::setCaptions(QList<CaptionObject> co) { m_captions = co; }
void ViewInformation::setPoints(QList<Vec> pts) { m_points = pts; }
void ViewInformation::setPaths(QList<PathObject> paths) { m_paths = paths; }
void ViewInformation::setPathGroups(QList<PathGroupObject> paths) { m_pathgroups = paths; }
void ViewInformation::setTrisets(QList<TrisetInformation> tinfo) { m_trisets = tinfo; }
void ViewInformation::setNetworks(QList<NetworkInformation> ninfo) { m_networks = ninfo; }
void ViewInformation::setTagColors(uchar* tc) { memcpy(m_tagColors, tc, 1024); }

int ViewInformation::volumeNumber() { return m_volumeNumber; }
int ViewInformation::volumeNumber2() { return m_volumeNumber2; }
int ViewInformation::volumeNumber3() { return m_volumeNumber3; }
int ViewInformation::volumeNumber4() { return m_volumeNumber4; }
float ViewInformation::stepsizeStill() { return m_stepsizeStill; }
float ViewInformation::stepsizeDrag() { return m_stepsizeDrag; }
bool ViewInformation::drawBox()  { return m_drawBox; }
bool ViewInformation::drawAxis()  { return m_drawAxis; }
Vec ViewInformation::backgroundColor() { return m_backgroundColor; }
QString ViewInformation::backgroundImageFile() { return m_backgroundImageFile; }
Vec ViewInformation::position() { return m_position; }
Quaternion ViewInformation::orientation() { return m_rotation; }
float ViewInformation::focusDistance() { return m_focusDistance; }
QImage ViewInformation::image() { return m_image; }
int ViewInformation::renderQuality() { return m_renderQuality; }
LightingInformation ViewInformation::lightInfo() { return m_lightInfo; }
ClipInformation ViewInformation::clipInfo() { return m_clipInfo; }
QList<BrickInformation> ViewInformation::brickInfo() { return m_brickInfo; }
void ViewInformation::volumeBounds(Vec &bmin, Vec &bmax) { bmin = m_volMin; bmax = m_volMax; }
QList<SplineInformation> ViewInformation::splineInfo() { return m_splineInfo; }
void ViewInformation::getTick(int &sz, int &st,
			       QString &xl, QString &yl, QString &zl)
{
  sz = m_tickSize;
  st = m_tickStep;
  xl = m_labelX;
  yl = m_labelY;
  zl = m_labelZ;
}
QList<CaptionObject> ViewInformation::captions() { return m_captions; }
QList<Vec> ViewInformation::points() { return m_points; }
QList<PathObject> ViewInformation::paths() { return m_paths; }
QList<PathGroupObject> ViewInformation::pathgroups() { return m_pathgroups; }
QList<TrisetInformation> ViewInformation::trisets() { return m_trisets; }
QList<NetworkInformation> ViewInformation::networks() { return m_networks; }
uchar* ViewInformation::tagColors() { return m_tagColors; }


ViewInformation::ViewInformation()
{
  m_drawBox = false;
  m_drawAxis = false;
  m_backgroundColor = Vec(0,0,0);
  m_backgroundImageFile.clear();
  m_position = Vec(0,0,0);
  m_rotation = Quaternion(Vec(1,0,0), 0);
  m_focusDistance = 0;
  m_volumeNumber = 0;
  m_volumeNumber2 = 0;
  m_volumeNumber3 = 0;
  m_volumeNumber4 = 0;
  m_volMin = m_volMax = Vec(0,0,0);
  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_clipInfo.clear();
  m_brickInfo.clear();
  m_splineInfo.clear();
  m_tickSize = 6;
  m_tickStep = 10;
  m_labelX = "X";
  m_labelY = "Y";
  m_labelZ = "Z";
  m_captions.clear();
  m_points.clear();
  m_paths.clear();
  m_pathgroups.clear();
  m_trisets.clear();
  m_networks.clear();
  m_tagColors = new uchar[1024];
}

ViewInformation::~ViewInformation()
{
  m_backgroundImageFile.clear();
  m_clipInfo.clear();
  m_brickInfo.clear();
  m_splineInfo.clear();
  m_labelX.clear();
  m_labelY.clear();
  m_labelZ.clear();
  m_captions.clear();
  m_points.clear();
  m_paths.clear();
  m_pathgroups.clear();
  m_trisets.clear();
  m_networks.clear();
  if (m_tagColors) delete [] m_tagColors;
}

ViewInformation::ViewInformation(const ViewInformation& viewInfo)
{
  m_volumeNumber = viewInfo.m_volumeNumber;
  m_volumeNumber2 = viewInfo.m_volumeNumber2;
  m_volumeNumber3 = viewInfo.m_volumeNumber3;
  m_volumeNumber4 = viewInfo.m_volumeNumber4;
  m_stepsizeStill = viewInfo.m_stepsizeStill;
  m_stepsizeDrag = viewInfo.m_stepsizeDrag;
  m_drawBox = viewInfo.m_drawBox;
  m_drawAxis = viewInfo.m_drawAxis;
  m_backgroundColor = viewInfo.m_backgroundColor;
  m_backgroundImageFile = viewInfo.m_backgroundImageFile;
  m_position = viewInfo.m_position;
  m_rotation = viewInfo.m_rotation;
  m_focusDistance = viewInfo.m_focusDistance;
  m_image = viewInfo.m_image;
  m_renderQuality = viewInfo.m_renderQuality;
  m_lightInfo = viewInfo.m_lightInfo;
  m_clipInfo = viewInfo.m_clipInfo;
  m_brickInfo = viewInfo.m_brickInfo;
  m_volMin = viewInfo.m_volMin;
  m_volMax = viewInfo.m_volMax;
  m_splineInfo = viewInfo.m_splineInfo;
  m_tickSize = viewInfo.m_tickSize;
  m_tickStep = viewInfo.m_tickStep;
  m_labelX = viewInfo.m_labelX;
  m_labelY = viewInfo.m_labelY;
  m_labelZ = viewInfo.m_labelZ;
  m_captions = viewInfo.m_captions;
  m_points = viewInfo.m_points;
  m_paths = viewInfo.m_paths;
  m_pathgroups = viewInfo.m_pathgroups;
  m_trisets = viewInfo.m_trisets;
  m_networks = viewInfo.m_networks;

  m_tagColors = new uchar[1024];
  memcpy(m_tagColors, viewInfo.m_tagColors, 1024);
}


ViewInformation&
ViewInformation::operator=(const ViewInformation& viewInfo)
{
  m_volumeNumber = viewInfo.m_volumeNumber;
  m_volumeNumber2 = viewInfo.m_volumeNumber2;
  m_volumeNumber3 = viewInfo.m_volumeNumber3;
  m_volumeNumber4 = viewInfo.m_volumeNumber4;
  m_stepsizeStill = viewInfo.m_stepsizeStill;
  m_stepsizeDrag = viewInfo.m_stepsizeDrag;
  m_drawBox = viewInfo.m_drawBox;
  m_drawAxis = viewInfo.m_drawAxis;
  m_backgroundColor = viewInfo.m_backgroundColor;
  m_backgroundImageFile = viewInfo.m_backgroundImageFile;
  m_position = viewInfo.m_position;
  m_rotation = viewInfo.m_rotation;
  m_focusDistance = viewInfo.m_focusDistance;
  m_image = viewInfo.m_image;
  m_renderQuality = viewInfo.m_renderQuality;
  m_lightInfo = viewInfo.m_lightInfo;
  m_clipInfo = viewInfo.m_clipInfo;
  m_brickInfo = viewInfo.m_brickInfo;
  m_volMin = viewInfo.m_volMin;
  m_volMax = viewInfo.m_volMax;
  m_splineInfo = viewInfo.m_splineInfo;
  m_tickSize = viewInfo.m_tickSize;
  m_tickStep = viewInfo.m_tickStep;
  m_labelX = viewInfo.m_labelX;
  m_labelY = viewInfo.m_labelY;
  m_labelZ = viewInfo.m_labelZ;
  m_captions = viewInfo.m_captions;
  m_points = viewInfo.m_points;
  m_paths = viewInfo.m_paths;
  m_pathgroups = viewInfo.m_pathgroups;
  m_trisets = viewInfo.m_trisets;
  m_networks = viewInfo.m_networks;
  memcpy(m_tagColors, viewInfo.m_tagColors, 1024);

  return *this;
}

//--------------------------------
//---- load and save -------------
//--------------------------------
void
ViewInformation::load(fstream &fin)
{
  bool done = false;
  char keyword[100];
  float f[3];

  m_brickInfo.clear();
  m_splineInfo.clear();

  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "viewend") == 0)
	done = true;
      else if (strcmp(keyword, "volumenumber") == 0)
	fin.read((char*)&m_volumeNumber, sizeof(int));
      else if (strcmp(keyword, "volumenumber2") == 0)
	fin.read((char*)&m_volumeNumber2, sizeof(int));
      else if (strcmp(keyword, "volumenumber3") == 0)
	fin.read((char*)&m_volumeNumber3, sizeof(int));
      else if (strcmp(keyword, "volumenumber4") == 0)
	fin.read((char*)&m_volumeNumber4, sizeof(int));
      else if (strcmp(keyword, "stepsizestill") == 0)
	fin.read((char*)&m_stepsizeStill, sizeof(float));
      else if (strcmp(keyword, "stepsizedrag") == 0)
	fin.read((char*)&m_stepsizeDrag, sizeof(float));
      else if (strcmp(keyword, "drawbox") == 0)
	fin.read((char*)&m_drawBox, sizeof(bool));
      else if (strcmp(keyword, "drawaxis") == 0)
	fin.read((char*)&m_drawAxis, sizeof(bool));
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
      else if (strcmp(keyword, "focusdistance") == 0)
	fin.read((char*)&m_focusDistance, sizeof(float));
      else if (strcmp(keyword, "image") == 0)
	{
	  int n;
	  fin.read((char*)&n, sizeof(int));
	  uchar *tmp = new uchar[n+1];
	  fin.read((char*)tmp, n);
	  m_image = QImage::fromData(tmp, n);	 
	  delete [] tmp;
	}
      else if (strcmp(keyword, "renderquality") == 0)
	fin.read((char*)&m_renderQuality, sizeof(int));
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
      else if (strcmp(keyword, "volmin") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_volMin = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "volmax") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_volMax = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "splineinfostart") == 0)
	{
	  SplineInformation splineInfo;
	  splineInfo.load(fin);
	  m_splineInfo.append(splineInfo);
	}
      else if (strcmp(keyword, "tickinfo") == 0)
	{
	  fin.read((char*)&m_tickSize, sizeof(int));
	  fin.read((char*)&m_tickStep, sizeof(int));
	  char *str;
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_labelX = QString(str);
	  delete [] str;

	  fin.read((char*)&len, sizeof(int));
	  str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_labelY = QString(str);
	  delete [] str;

	  fin.read((char*)&len, sizeof(int));
	  str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_labelZ = QString(str);
	  delete [] str;
	}
      else if (strcmp(keyword, "captionobjectstart") == 0)
	{
	  CaptionObject co;
	  co.load(fin);
	  m_captions.append(co);
	}
      else if (strcmp(keyword, "pointsstart") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));
	      m_points.append(Vec(f[0], f[1], f[2]));
	    }
	  // "pointsend"
	  fin.getline(keyword, 100, 0);
	}
      else if (strcmp(keyword, "pathobjectstart") == 0)
	{
	  PathObject po;
	  po.load(fin);
	  m_paths.append(po);
	}
      else if (strcmp(keyword, "pathgroupobjectstart") == 0)
	{
	  PathGroupObject po;
	  po.load(fin);
	  m_pathgroups.append(po);
	}
      else if (strcmp(keyword, "trisetinformation") == 0)
	{
	  TrisetInformation ti;
	  ti.load(fin);
	  m_trisets.append(ti);
	}
      else if (strcmp(keyword, "networkinformation") == 0)
	{
	  NetworkInformation ni;
	  ni.load(fin);
	  m_networks.append(ni);
	}
      else if (strcmp(keyword, "tagcolors") == 0)
	{
	  int n;
	  fin.read((char*)&n, sizeof(int)); // n must be 1024
	  fin.read((char*)m_tagColors, 1024);
	}
    }
}

void
ViewInformation::save(fstream &fout)
{
  QString keyword;
  float f[3];

  keyword = "viewstart";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  

  keyword = "volumenumber";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_volumeNumber, sizeof(int));


  keyword = "volumenumber2";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_volumeNumber2, sizeof(int));


  keyword = "volumenumber3";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_volumeNumber3, sizeof(int));


  keyword = "volumenumber4";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_volumeNumber4, sizeof(int));


  keyword = "stepsizestill";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_stepsizeStill, sizeof(float));

  keyword = "stepsizedrag";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_stepsizeDrag, sizeof(float));

  keyword = "drawbox";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_drawBox, sizeof(bool));

  keyword = "drawaxis";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_drawAxis, sizeof(bool));

  keyword = "backgroundcolor";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  f[0] = m_backgroundColor.x;
  f[1] = m_backgroundColor.y;
  f[2] = m_backgroundColor.z;
  fout.write((char*)&f, 3*sizeof(float));

  keyword = "backgroundimage";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  int len;
  len = m_backgroundImageFile.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_backgroundImageFile.toLatin1().data(), len*sizeof(char));

  keyword = "position";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  f[0] = m_position.x;
  f[1] = m_position.y;
  f[2] = m_position.z;
  fout.write((char*)&f, 3*sizeof(float));


  keyword = "rotation";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  Vec axis;
  qreal ang;
  m_rotation.getAxisAngle(axis, ang);
  f[0] = axis.x;
  f[1] = axis.y;
  f[2] = axis.z;
  fout.write((char*)&f, 3*sizeof(float));
  float angle = ang;
  fout.write((char*)&angle, sizeof(float));


  keyword = "focusdistance";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_focusDistance, sizeof(float));


  keyword = "image";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  m_image.save(&buffer, "PNG");
  int n = bytes.size();
  fout.write((char*)&n, sizeof(int));
  fout.write((char*)bytes.data(), n);


  keyword = "renderquality";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_renderQuality, sizeof(int));


  m_lightInfo.save(fout);
  m_clipInfo.save(fout);

  for(int i=0; i<m_brickInfo.size(); i++)
    m_brickInfo[i].save(fout);

  keyword = "volmin";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  f[0] = m_volMin.x;
  f[1] = m_volMin.y;
  f[2] = m_volMin.z;
  fout.write((char*)&f, 3*sizeof(float));


  keyword = "volmax";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  f[0] = m_volMax.x;
  f[1] = m_volMax.y;
  f[2] = m_volMax.z;
  fout.write((char*)&f, 3*sizeof(float));


  for(int i=0; i<m_splineInfo.size(); i++)
    m_splineInfo[i].save(fout);


  for(int i=0; i<m_captions.size(); i++)
    m_captions[i].save(fout);


  keyword = "pointsstart";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  int npts = m_points.count();
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<m_points.size(); i++)
    {
      f[0] = m_points[i].x;
      f[1] = m_points[i].y;
      f[2] = m_points[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }
  keyword = "pointsend";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  


  for(int i=0; i<m_paths.size(); i++)
    m_paths[i].save(fout);


  for(int i=0; i<m_pathgroups.size(); i++)
    m_pathgroups[i].save(fout);


  for(int i=0; i<m_trisets.size(); i++)
    m_trisets[i].save(fout);

  for(int i=0; i<m_networks.size(); i++)
    m_networks[i].save(fout);


  keyword = "tickinfo";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  fout.write((char*)&m_tickSize, sizeof(int));
  fout.write((char*)&m_tickStep, sizeof(int));
  len = m_labelX.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_labelX.toLatin1().data(), len*sizeof(char));
  len = m_labelY.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_labelY.toLatin1().data(), len*sizeof(char));
  len = m_labelZ.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_labelZ.toLatin1().data(), len*sizeof(char));


  keyword = "tagcolors";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
  int tcn = 1024;
  fout.write((char*)&tcn, sizeof(int));
  fout.write((char*)m_tagColors, 1024);


  keyword = "viewend";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
}
