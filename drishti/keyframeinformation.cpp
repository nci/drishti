#include "keyframeinformation.h"
#include "global.h"
#include "enums.h"

void KeyFrameInformation::setDrawBox(bool flag) { m_drawBox = flag; }
void KeyFrameInformation::setDrawAxis(bool flag) { m_drawAxis = flag; }
void KeyFrameInformation::setBackgroundColor(Vec col) { m_backgroundColor = col; }
void KeyFrameInformation::setBackgroundImageFile(QString fl) { m_backgroundImageFile = fl; }
void KeyFrameInformation::setFrameNumber(int fn) { m_frameNumber = fn; }
void KeyFrameInformation::setFocusDistance(float fd, float es) { m_focusDistance = fd; m_eyeSeparation = es; }
void KeyFrameInformation::setVolumeNumber(int vn) { m_volumeNumber = vn; }
void KeyFrameInformation::setVolumeNumber2(int vn) { m_volumeNumber2 = vn; }
void KeyFrameInformation::setVolumeNumber3(int vn) { m_volumeNumber3 = vn; }
void KeyFrameInformation::setVolumeNumber4(int vn) { m_volumeNumber4 = vn; }
void KeyFrameInformation::setPosition(Vec pos) { m_position = pos; }
void KeyFrameInformation::setOrientation(Quaternion rot) { m_rotation = rot; }
void KeyFrameInformation::setLut(unsigned char* lut)
{
  if (!m_lut)
    m_lut = new unsigned char[Global::lutSize()*256*256*4];
  memcpy(m_lut, lut, Global::lutSize()*256*256*4);
}
void KeyFrameInformation::setLightInfo(LightingInformation li) { m_lightInfo = li; }
void KeyFrameInformation::setGiLightInfo(GiLightInfo li) { m_giLightInfo = li; }
void KeyFrameInformation::setClipInfo(ClipInformation ci) { m_clipInfo = ci; }
void KeyFrameInformation::setVolumeBounds(Vec bmin, Vec bmax) { m_volMin = bmin; m_volMax = bmax; }
void KeyFrameInformation::setImage(QImage pix) { m_image = pix; }
void KeyFrameInformation::setSplineInfo(QList<SplineInformation> si) { m_splineInfo = si; }
void KeyFrameInformation::setTick(int sz, int st,
				  QString xl, QString yl, QString zl)
{
  m_tickSize = sz;
  m_tickStep = st;
  m_labelX = xl;
  m_labelY = yl;
  m_labelZ = zl;
}
void KeyFrameInformation::setMix(int mv, bool mc, bool mo, bool mt)
{
  m_mixvol = mv;
  m_mixColor = mc;
  m_mixOpacity = mo;
  m_mixTag = mt;
}
void KeyFrameInformation::setBrickInfo(QList<BrickInformation> bi)
{
  m_brickInfo.clear();
  m_brickInfo += bi;
}
void KeyFrameInformation::setMorphTF(bool flag) { m_morphTF = flag; }
void KeyFrameInformation::setCaptions(QList<CaptionObject> co) { m_captions = co; }
void KeyFrameInformation::setColorBars(QList<ColorBarObject> co) { m_colorbars = co; }
void KeyFrameInformation::setScaleBars(QList<ScaleBarObject> co) { m_scalebars = co; }
void KeyFrameInformation::setPoints(QList<Vec> pts, QList<Vec> bpts, int psz, Vec pcol)
{ m_points=pts; m_barepoints=bpts; m_pointSize=psz; m_pointColor=pcol;}
void KeyFrameInformation::setPaths(QList<PathObject> paths) { m_paths = paths; }
void KeyFrameInformation::setGrids(QList<GridObject> grids) { m_grids = grids; }
void KeyFrameInformation::setCrops(QList<CropObject> crops) { m_crops = crops; }
void KeyFrameInformation::setPathGroups(QList<PathGroupObject> paths) { m_pathgroups = paths; }
void KeyFrameInformation::setTrisets(QList<TrisetInformation> tinfo) { m_trisets = tinfo; }
void KeyFrameInformation::setNetworks(QList<NetworkInformation> ninfo) { m_networks = ninfo; }
void KeyFrameInformation::setTagColors(unsigned char* tc) { memcpy(m_tagColors, tc, 1024); }
void KeyFrameInformation::setPruneBuffer(QByteArray pb) { m_pruneBuffer = pb; }
void KeyFrameInformation::setPruneBlend(bool pb) { m_pruneBlend = pb; }


bool KeyFrameInformation::drawBox() { return m_drawBox; }
bool KeyFrameInformation::drawAxis() { return m_drawAxis; }
Vec KeyFrameInformation::backgroundColor() { return m_backgroundColor; }
QString KeyFrameInformation::backgroundImageFile() { return m_backgroundImageFile; }
int KeyFrameInformation::frameNumber() { return m_frameNumber; }
float KeyFrameInformation::focusDistance() { return m_focusDistance; }
float KeyFrameInformation::eyeSeparation() { return m_eyeSeparation; }
int KeyFrameInformation::volumeNumber() { return m_volumeNumber; }
int KeyFrameInformation::volumeNumber2() { return m_volumeNumber2; }
int KeyFrameInformation::volumeNumber3() { return m_volumeNumber3; }
int KeyFrameInformation::volumeNumber4() { return m_volumeNumber4; }
Vec KeyFrameInformation::position() { return m_position; }
Quaternion KeyFrameInformation::orientation() { return m_rotation; }
unsigned char* KeyFrameInformation::lut() { return m_lut; }
LightingInformation KeyFrameInformation::lightInfo() { return m_lightInfo; }
GiLightInfo KeyFrameInformation::giLightInfo() { return m_giLightInfo; }
ClipInformation KeyFrameInformation::clipInfo() { return m_clipInfo; }
void KeyFrameInformation::volumeBounds(Vec &bmin, Vec &bmax) { bmin = m_volMin; bmax = m_volMax; }
QImage KeyFrameInformation::image() { return m_image; }
QList<SplineInformation> KeyFrameInformation::splineInfo() { return m_splineInfo; }
void KeyFrameInformation::getTick(int &sz, int &st,
				  QString &xl, QString &yl, QString &zl)
{
  sz = m_tickSize;
  st = m_tickStep;
  xl = m_labelX;
  yl = m_labelY;
  zl = m_labelZ;
}
void KeyFrameInformation::getMix(int &mv, bool &mc, bool &mo, bool &mt)
{
  mv = m_mixvol;
  mc = m_mixColor;
  mo = m_mixOpacity;
  mt = m_mixTag;
}
QList<BrickInformation> KeyFrameInformation::brickInfo() { return m_brickInfo; }
bool KeyFrameInformation::morphTF() { return m_morphTF; }
QList<CaptionObject> KeyFrameInformation::captions() { return m_captions; }
QList<ColorBarObject> KeyFrameInformation::colorbars() { return m_colorbars; }
QList<ScaleBarObject> KeyFrameInformation::scalebars() { return m_scalebars; }
QList<Vec> KeyFrameInformation::points() { return m_points; }
QList<Vec> KeyFrameInformation::barepoints() { return m_barepoints; }
int KeyFrameInformation::pointSize() { return m_pointSize; }
Vec KeyFrameInformation::pointColor() { return m_pointColor; }
QList<PathObject> KeyFrameInformation::paths() { return m_paths; }
QList<GridObject> KeyFrameInformation::grids() { return m_grids; }
QList<CropObject> KeyFrameInformation::crops() { return m_crops; }
QList<PathGroupObject> KeyFrameInformation::pathgroups() { return m_pathgroups; }
QList<TrisetInformation> KeyFrameInformation::trisets() { return m_trisets; }
QList<NetworkInformation> KeyFrameInformation::networks() { return m_networks; }
unsigned char* KeyFrameInformation::tagColors() { return m_tagColors; }
QByteArray KeyFrameInformation::pruneBuffer() { return m_pruneBuffer; }
bool KeyFrameInformation::pruneBlend() { return m_pruneBlend; }


// -- keyframe interpolation parameters
void KeyFrameInformation::setInterpBGColor(int i) {m_interpBGColor = i;}
void KeyFrameInformation::setInterpCaptions(int i) {m_interpCaptions = i;}
void KeyFrameInformation::setInterpFocus(int i) {m_interpFocus = i;}
void KeyFrameInformation::setInterpTagColors(int i) {m_interpTagColors = i;}
void KeyFrameInformation::setInterpTickInfo(int i) {m_interpTickInfo = i;}
void KeyFrameInformation::setInterpVolumeBounds(int i) {m_interpVolumeBounds = i;}
void KeyFrameInformation::setInterpCameraPosition(int i) {m_interpCameraPosition = i;}
void KeyFrameInformation::setInterpCameraOrientation(int i) {m_interpCameraOrientation = i;}
void KeyFrameInformation::setInterpBrickInfo(int i) {m_interpBrickInfo = i;}
void KeyFrameInformation::setInterpClipInfo(int i) {m_interpClipInfo = i;}
void KeyFrameInformation::setInterpLightInfo(int i) {m_interpLightInfo = i;}
void KeyFrameInformation::setInterpGiLightInfo(int i) {m_interpGiLightInfo = i;}
void KeyFrameInformation::setInterpTF(int i) {m_interpTF = i;}
void KeyFrameInformation::setInterpCrop(int i) {m_interpCrop = i;}
void KeyFrameInformation::setInterpMop(int i) {m_interpMop = i;}

int KeyFrameInformation::interpBGColor() { return m_interpBGColor; }
int KeyFrameInformation::interpCaptions() { return m_interpCaptions; }
int KeyFrameInformation::interpFocus() { return m_interpFocus; }
int KeyFrameInformation::interpTagColors() { return m_interpTagColors; }
int KeyFrameInformation::interpTickInfo() { return m_interpTickInfo; }
int KeyFrameInformation::interpVolumeBounds() { return m_interpVolumeBounds; }
int KeyFrameInformation::interpCameraPosition() { return m_interpCameraPosition; }
int KeyFrameInformation::interpCameraOrientation() { return m_interpCameraOrientation; }
int KeyFrameInformation::interpBrickInfo() { return m_interpBrickInfo; }
int KeyFrameInformation::interpClipInfo() { return m_interpClipInfo; }
int KeyFrameInformation::interpLightInfo() { return m_interpLightInfo; }
int KeyFrameInformation::interpGiLightInfo() { return m_interpGiLightInfo; }
int KeyFrameInformation::interpTF() { return m_interpTF; }
int KeyFrameInformation::interpCrop() { return m_interpCrop; }
int KeyFrameInformation::interpMop() { return m_interpMop; }

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
  m_drawBox = false;
  m_drawAxis = false;
  m_backgroundColor = Vec(0,0,0);
  m_backgroundImageFile.clear();
  m_frameNumber = 0;
  m_focusDistance = 0;
  m_eyeSeparation = 0.062;
  m_volumeNumber = 0;
  m_volumeNumber2 = 0;
  m_volumeNumber3 = 0;
  m_volumeNumber4 = 0;
  m_position = Vec(0,0,0);
  m_rotation = Quaternion(Vec(1,0,0), 0);
  //m_lut = new unsigned char[Global::lutSize()*256*256*4];
  m_lut = 0;
  m_tagColors = new unsigned char[1024];
  m_pruneBuffer.clear();
  m_pruneBlend = false;
  m_volMin = m_volMax = Vec(0,0,0);
  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_clipInfo.clear();
  m_tickSize = 6;
  m_tickStep = 10;
  m_labelX = "X";
  m_labelY = "Y";
  m_labelZ = "Z";
  m_mixvol = 0;
  m_mixColor = false;
  m_mixOpacity = false;
  m_mixTag = false;
  m_brickInfo.clear();
  m_splineInfo.clear();
  m_morphTF = false;
  m_captions.clear();
  m_colorbars.clear();
  m_scalebars.clear();
  m_points.clear();
  m_barepoints.clear();
  m_pointSize = 10;
  m_pointColor = Vec(0.0f, 0.5f, 1.0f);
  m_paths.clear();
  m_grids.clear();
  m_crops.clear();
  m_pathgroups.clear();
  m_trisets.clear();
  m_networks.clear();

  m_interpBGColor = Enums::KFIT_Linear;
  m_interpCaptions = Enums::KFIT_Linear;
  m_interpFocus = Enums::KFIT_Linear;
  m_interpTagColors = Enums::KFIT_Linear;
  m_interpTickInfo = Enums::KFIT_Linear;
  m_interpVolumeBounds = Enums::KFIT_Linear;
  m_interpCameraPosition = Enums::KFIT_Linear;
  m_interpCameraOrientation = Enums::KFIT_Linear;
  m_interpBrickInfo = Enums::KFIT_Linear;
  m_interpClipInfo = Enums::KFIT_Linear;
  m_interpLightInfo = Enums::KFIT_Linear;
  m_interpGiLightInfo = Enums::KFIT_None;
  m_interpTF = Enums::KFIT_Linear;
  m_interpCrop = Enums::KFIT_Linear;
  m_interpMop = Enums::KFIT_None;
}

void
KeyFrameInformation::clear()
{
  m_drawBox = false;
  m_drawAxis = false;
  m_backgroundColor = Vec(0,0,0);
  m_backgroundImageFile.clear();
  m_frameNumber = 0;
  m_focusDistance = 0;
  m_eyeSeparation = 0.062;
  m_volumeNumber = 0;
  m_volumeNumber2 = 0;
  m_volumeNumber3 = 0;
  m_volumeNumber4 = 0;
  m_position = Vec(0,0,0);
  m_rotation = Quaternion(Vec(1,0,0), 0);
  if (m_lut) delete [] m_lut;
  m_lut = 0;
  //m_lut = new unsigned char[Global::lutSize()*256*256*4];
  if (m_tagColors) delete [] m_tagColors;
  m_tagColors = new unsigned char[1024];
  m_pruneBuffer.clear();
  m_pruneBlend = false;
  m_volMin = m_volMax = Vec(0,0,0);
  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_lightInfo.clear();
  m_giLightInfo.clear();
  m_clipInfo.clear();
  m_brickInfo.clear();
  m_tickSize = 6;
  m_tickStep = 10;
  m_labelX = "X";
  m_labelY = "Y";
  m_labelZ = "Z";
  m_mixvol = 0;
  m_mixColor = false;
  m_mixOpacity = false;
  m_mixTag = false;
  m_splineInfo.clear();
  m_morphTF = false;
  m_captions.clear();
  m_colorbars.clear();
  m_scalebars.clear();
  m_points.clear();
  m_barepoints.clear();
  m_pointSize = 10;
  m_pointColor = Vec(0.0f, 0.5f, 1.0f);
  m_paths.clear();
  m_grids.clear();
  m_crops.clear();
  m_pathgroups.clear();
  m_trisets.clear();
  m_networks.clear();

  m_interpBGColor = Enums::KFIT_Linear;
  m_interpCaptions = Enums::KFIT_Linear;
  m_interpFocus = Enums::KFIT_Linear;
  m_interpTagColors = Enums::KFIT_Linear;
  m_interpTickInfo = Enums::KFIT_Linear;
  m_interpVolumeBounds = Enums::KFIT_Linear;
  m_interpCameraPosition = Enums::KFIT_Linear;
  m_interpCameraOrientation = Enums::KFIT_Linear;
  m_interpBrickInfo = Enums::KFIT_Linear;
  m_interpClipInfo = Enums::KFIT_Linear;
  m_interpLightInfo = Enums::KFIT_Linear;
  m_interpGiLightInfo = Enums::KFIT_None;
  m_interpTF = Enums::KFIT_Linear;
  m_interpCrop = Enums::KFIT_Linear;
  m_interpMop = Enums::KFIT_None;
}

KeyFrameInformation::KeyFrameInformation(const KeyFrameInformation& kfi)
{
  m_drawBox = kfi.m_drawBox;
  m_drawAxis = kfi.m_drawAxis;

  m_backgroundColor = kfi.m_backgroundColor;
  m_backgroundImageFile = kfi.m_backgroundImageFile;

  m_frameNumber = kfi.m_frameNumber;
  m_focusDistance = kfi.m_focusDistance;

  m_eyeSeparation = kfi.m_eyeSeparation;

  m_volumeNumber = kfi.m_volumeNumber;
  m_volumeNumber2 = kfi.m_volumeNumber2;
  m_volumeNumber3 = kfi.m_volumeNumber3;
  m_volumeNumber4 = kfi.m_volumeNumber4;

  m_position = kfi.m_position;
  m_rotation = kfi.m_rotation;

  m_lut = new unsigned char[Global::lutSize()*256*256*4];
  memcpy(m_lut, kfi.m_lut, Global::lutSize()*256*256*4);

  m_tagColors = new unsigned char[1024];
  memcpy(m_tagColors, kfi.m_tagColors, 1024);

  m_pruneBuffer = kfi.m_pruneBuffer;
  m_pruneBlend = kfi.m_pruneBlend;

  m_lightInfo = kfi.m_lightInfo;
  m_giLightInfo = kfi.m_giLightInfo;
  m_clipInfo = kfi.m_clipInfo;

  m_volMin = kfi.m_volMin;
  m_volMax = kfi.m_volMax;

  m_image = kfi.m_image;

  m_brickInfo = kfi.m_brickInfo;

  m_tickSize = kfi.m_tickSize;
  m_tickStep = kfi.m_tickStep;
  m_labelX = kfi.m_labelX;
  m_labelY = kfi.m_labelY;
  m_labelZ = kfi.m_labelZ;

  m_mixvol = kfi.m_mixvol;
  m_mixColor = kfi.m_mixColor;
  m_mixOpacity = kfi.m_mixOpacity;
  m_mixTag = kfi.m_mixTag;

  m_splineInfo = kfi.m_splineInfo;
  m_morphTF = kfi.m_morphTF;

  m_captions = kfi.m_captions;
  m_colorbars = kfi.m_colorbars;
  m_scalebars = kfi.m_scalebars;
  m_points = kfi.m_points;
  m_barepoints = kfi.m_barepoints;
  m_pointSize = kfi.m_pointSize;
  m_pointColor = kfi.m_pointColor;
  m_paths = kfi.m_paths;
  m_grids = kfi.m_grids;
  m_crops = kfi.m_crops;
  m_pathgroups = kfi.m_pathgroups;
  m_trisets = kfi.m_trisets;
  m_networks = kfi.m_networks;

  m_interpBGColor = kfi.m_interpBGColor;
  m_interpCaptions = kfi.m_interpCaptions;
  m_interpFocus = kfi.m_interpFocus;
  m_interpTagColors = kfi.m_interpTagColors;
  m_interpTickInfo = kfi.m_interpTickInfo;
  m_interpVolumeBounds = kfi.m_interpVolumeBounds;
  m_interpCameraPosition = kfi.m_interpCameraPosition;
  m_interpCameraOrientation = kfi.m_interpCameraOrientation;
  m_interpBrickInfo = kfi.m_interpBrickInfo;
  m_interpClipInfo = kfi.m_interpClipInfo;
  m_interpLightInfo = kfi.m_interpLightInfo;
  m_interpGiLightInfo = kfi.m_interpGiLightInfo;
  m_interpTF = kfi.m_interpTF;
  m_interpCrop = kfi.m_interpCrop;
  m_interpMop = kfi.m_interpMop;
}

KeyFrameInformation::~KeyFrameInformation()
{
  if (m_lut)
    delete [] m_lut;
  if (m_tagColors)
    delete [] m_tagColors;
  m_clipInfo.clear();
  m_brickInfo.clear();
  m_labelX.clear();
  m_labelY.clear();
  m_labelZ.clear();
  m_splineInfo.clear();
  m_captions.clear();
  m_colorbars.clear();
  m_scalebars.clear();
  m_points.clear();
  m_barepoints.clear();
  m_paths.clear();
  m_grids.clear();
  m_crops.clear();
  m_pathgroups.clear();
  m_trisets.clear();
  m_networks.clear();
  m_pruneBuffer.clear();
  m_pruneBlend = false;
}

KeyFrameInformation&
KeyFrameInformation::operator=(const KeyFrameInformation& kfi)
{
  m_drawBox = kfi.m_drawBox;
  m_drawAxis = kfi.m_drawAxis;

  m_backgroundColor = kfi.m_backgroundColor;
  m_backgroundImageFile = kfi.m_backgroundImageFile;

  m_frameNumber = kfi.m_frameNumber;
  m_focusDistance = kfi.m_focusDistance;

  m_eyeSeparation = kfi.m_eyeSeparation;

  m_volumeNumber = kfi.m_volumeNumber;
  m_volumeNumber2 = kfi.m_volumeNumber2;
  m_volumeNumber3 = kfi.m_volumeNumber3;
  m_volumeNumber4 = kfi.m_volumeNumber4;

  m_position = kfi.m_position;
  m_rotation = kfi.m_rotation;

  if (!m_lut)
    m_lut = new unsigned char[Global::lutSize()*256*256*4];
  memcpy(m_lut, kfi.m_lut, Global::lutSize()*256*256*4);

  memcpy(m_tagColors, kfi.m_tagColors, 1024);

  m_pruneBuffer = kfi.m_pruneBuffer;
  m_pruneBlend = kfi.m_pruneBlend;

  m_lightInfo = kfi.m_lightInfo;
  m_giLightInfo = kfi.m_giLightInfo;
  m_clipInfo = kfi.m_clipInfo;

  m_volMin = kfi.m_volMin;
  m_volMax = kfi.m_volMax;

  m_image = kfi.m_image;

  m_brickInfo = kfi.m_brickInfo;

  m_tickSize = kfi.m_tickSize;
  m_tickStep = kfi.m_tickStep;
  m_labelX = kfi.m_labelX;
  m_labelY = kfi.m_labelY;
  m_labelZ = kfi.m_labelZ;

  m_mixvol = kfi.m_mixvol;
  m_mixColor = kfi.m_mixColor;
  m_mixOpacity = kfi.m_mixOpacity;
  m_mixTag = kfi.m_mixTag;

  m_splineInfo = kfi.m_splineInfo;
  m_morphTF = kfi.m_morphTF;

  m_captions = kfi.m_captions;
  m_colorbars = kfi.m_colorbars;
  m_scalebars = kfi.m_scalebars;
  m_points = kfi.m_points;
  m_barepoints = kfi.m_barepoints;
  m_pointSize = kfi.m_pointSize;
  m_pointColor = kfi.m_pointColor;
  m_paths = kfi.m_paths;
  m_grids = kfi.m_grids;
  m_crops = kfi.m_crops;
  m_pathgroups = kfi.m_pathgroups;
  m_trisets = kfi.m_trisets;
  m_networks = kfi.m_networks;

  m_interpBGColor = kfi.m_interpBGColor;
  m_interpCaptions = kfi.m_interpCaptions;
  m_interpFocus = kfi.m_interpFocus;
  m_interpTagColors = kfi.m_interpTagColors;
  m_interpTickInfo = kfi.m_interpTickInfo;
  m_interpVolumeBounds = kfi.m_interpVolumeBounds;
  m_interpCameraPosition = kfi.m_interpCameraPosition;
  m_interpCameraOrientation = kfi.m_interpCameraOrientation;
  m_interpBrickInfo = kfi.m_interpBrickInfo;
  m_interpClipInfo = kfi.m_interpClipInfo;
  m_interpLightInfo = kfi.m_interpLightInfo;
  m_interpGiLightInfo = kfi.m_interpGiLightInfo;
  m_interpTF = kfi.m_interpTF;
  m_interpCrop = kfi.m_interpCrop;
  m_interpMop = kfi.m_interpMop;

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

  m_brickInfo.clear();
  m_pruneBuffer.clear();
  m_pruneBlend = false;

  m_interpBGColor = Enums::KFIT_Linear;
  m_interpCaptions = Enums::KFIT_Linear;
  m_interpFocus = Enums::KFIT_Linear;
  m_interpTagColors = Enums::KFIT_Linear;
  m_interpTickInfo = Enums::KFIT_Linear;
  m_interpVolumeBounds = Enums::KFIT_Linear;
  m_interpCameraPosition = Enums::KFIT_Linear;
  m_interpCameraOrientation = Enums::KFIT_Linear;
  m_interpBrickInfo = Enums::KFIT_Linear;
  m_interpClipInfo = Enums::KFIT_Linear;
  m_interpLightInfo = Enums::KFIT_Linear;
  m_interpGiLightInfo = Enums::KFIT_None;
  m_interpTF = Enums::KFIT_Linear;
  m_interpCrop = Enums::KFIT_Linear;
  m_interpMop = Enums::KFIT_None;

  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "keyframeend") == 0)
	done = true;
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
      else if (strcmp(keyword, "framenumber") == 0)
	fin.read((char*)&m_frameNumber, sizeof(int));
      else if (strcmp(keyword, "morphtf") == 0)
	fin.read((char*)&m_morphTF, sizeof(bool));
      else if (strcmp(keyword, "focusdistance") == 0)
	fin.read((char*)&m_focusDistance, sizeof(float));
      else if (strcmp(keyword, "eyeseparation") == 0)
	fin.read((char*)&m_eyeSeparation, sizeof(float));
      else if (strcmp(keyword, "volumenumber4") == 0)
	fin.read((char*)&m_volumeNumber4, sizeof(int));
      else if (strcmp(keyword, "volumenumber3") == 0)
	fin.read((char*)&m_volumeNumber3, sizeof(int));
      else if (strcmp(keyword, "volumenumber2") == 0)
	fin.read((char*)&m_volumeNumber2, sizeof(int));
      else if (strcmp(keyword, "volumenumber") == 0)
	fin.read((char*)&m_volumeNumber, sizeof(int));
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
      else if (strcmp(keyword, "lookuptable") == 0)
	{
	  if (!m_lut)
	    m_lut = new unsigned char[Global::lutSize()*256*256*4];
	  int n;
	  fin.read((char*)&n, sizeof(int));
	  n = qMin(n, Global::lutSize());
	  fin.read((char*)m_lut, n*256*256*4);
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
      else if (strcmp(keyword, "gilightinfo") == 0)
	m_giLightInfo.load(fin);
      else if (strcmp(keyword, "clipinformation") == 0)
	m_clipInfo.load(fin);
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
      else if (strcmp(keyword, "mixinfo") == 0)
	{
	  fin.read((char*)&m_mixvol, sizeof(int));
	  fin.read((char*)&m_mixColor, sizeof(bool));
	  fin.read((char*)&m_mixOpacity, sizeof(bool));
	}
      else if (strcmp(keyword, "mixtag") == 0)
	  fin.read((char*)&m_mixTag, sizeof(bool));
      else if (strcmp(keyword, "brickinformation") == 0)
	{
	  BrickInformation brickInfo;
	  brickInfo.load(fin);
	  m_brickInfo.append(brickInfo);
	}
      else if (strcmp(keyword, "splineinfostart") == 0)
	{
	  SplineInformation splineInfo;
	  splineInfo.load(fin);
	  m_splineInfo.append(splineInfo);
	}
      else if (strcmp(keyword, "captionobjectstart") == 0)
	{
	  CaptionObject co;
	  co.load(fin);
	  m_captions.append(co);
	}
      else if (strcmp(keyword, "colorbarobjectstart") == 0)
	{
	  ColorBarObject co;
	  co.load(fin);
	  m_colorbars.append(co);
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
      else if (strcmp(keyword, "gridobjectstart") == 0)
	{
	  GridObject go;
	  go.load(fin);
	  m_grids.append(go);
	}
      else if (strcmp(keyword, "cropobjectstart") == 0)
	{
	  CropObject co;
	  co.load(fin);
	  m_crops.append(co);
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
      else if (strcmp(keyword, "pruneblend") == 0)
	{
	  fin.read((char*)&m_pruneBlend, sizeof(bool));
	}
      else if (strcmp(keyword, "prunebuffer") == 0)
	{
	  int n;
	  fin.read((char*)&n, sizeof(int));
	  char *data = new char[n];	  
	  fin.read(data, n);
	  m_pruneBuffer = QByteArray(data, n);
	  delete [] data;
	}
      else if (strcmp(keyword, "interpbgcolor") == 0)
	fin.read((char*)&m_interpBGColor, sizeof(int));
      else if (strcmp(keyword, "interpcaptions") == 0)
	fin.read((char*)&m_interpCaptions, sizeof(int));
      else if (strcmp(keyword, "interpfocus") == 0)
	fin.read((char*)&m_interpFocus, sizeof(int));
      else if (strcmp(keyword, "interptagcolors") == 0)
	fin.read((char*)&m_interpTagColors, sizeof(int));
      else if (strcmp(keyword, "interptickinfo") == 0)
	fin.read((char*)&m_interpTickInfo, sizeof(int));
      else if (strcmp(keyword, "interpvolumebounds") == 0)
	fin.read((char*)&m_interpVolumeBounds, sizeof(int));
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
      else if (strcmp(keyword, "interpgilightinfo") == 0)
	fin.read((char*)&m_interpGiLightInfo, sizeof(int));
      else if (strcmp(keyword, "interptf") == 0)
	fin.read((char*)&m_interpTF, sizeof(int));
      else if (strcmp(keyword, "interpcrop") == 0)
	fin.read((char*)&m_interpCrop, sizeof(int));
      else if (strcmp(keyword, "interpmop") == 0)
	fin.read((char*)&m_interpMop, sizeof(int));
    }
}

void
KeyFrameInformation::save(fstream &fout)
{
  char keyword[100];
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "keyframestart");
  fout.write((char*)keyword, strlen(keyword)+1);


  memset(keyword, 0, 100);
  sprintf(keyword, "framenumber");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_frameNumber, sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "drawbox");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_drawBox, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "drawaxis");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_drawAxis, sizeof(bool));


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
  int len;
  len = m_backgroundImageFile.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_backgroundImageFile.toAscii().data(), len*sizeof(char));


  memset(keyword, 0, 100);
  sprintf(keyword, "morphtf");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_morphTF, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "focusdistance");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_focusDistance, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "eyeseparation");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_eyeSeparation, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "volumenumber");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_volumeNumber, sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "volumenumber2");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_volumeNumber2, sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "volumenumber3");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_volumeNumber3, sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "volumenumber4");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_volumeNumber4, sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "volmin");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_volMin.x;
  f[1] = m_volMin.y;
  f[2] = m_volMin.z;
  fout.write((char*)&f, 3*sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "volmax");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_volMax.x;
  f[1] = m_volMax.y;
  f[2] = m_volMax.z;
  fout.write((char*)&f, 3*sizeof(float));


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
  float angle;
  m_rotation.getAxisAngle(axis, angle);
  f[0] = axis.x;
  f[1] = axis.y;
  f[2] = axis.z;
  fout.write((char*)&f, 3*sizeof(float));
  fout.write((char*)&angle, sizeof(float));


  if (m_lut)
    {
      memset(keyword, 0, 100);
      sprintf(keyword, "lookuptable");
      fout.write((char*)keyword, strlen(keyword)+1);
      int n = Global::lutSize();
      fout.write((char*)&n, sizeof(int));
      fout.write((char*)m_lut, Global::lutSize()*256*256*4);
    }

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
  m_giLightInfo.save(fout);
  m_clipInfo.save(fout);

  memset(keyword, 0, 100);
  sprintf(keyword, "tickinfo");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_tickSize, sizeof(int));
  fout.write((char*)&m_tickStep, sizeof(int));
  len = m_labelX.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_labelX.toAscii().data(), len*sizeof(char));
  len = m_labelY.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_labelY.toAscii().data(), len*sizeof(char));
  len = m_labelZ.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_labelZ.toAscii().data(), len*sizeof(char));


  memset(keyword, 0, 100);
  sprintf(keyword, "mixinfo");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_mixvol, sizeof(int));
  fout.write((char*)&m_mixColor, sizeof(bool));
  fout.write((char*)&m_mixOpacity, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "mixtag");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_mixTag, sizeof(bool));


  for(int i=0; i<m_brickInfo.size(); i++)
    m_brickInfo[i].save(fout);


  for(int i=0; i<m_splineInfo.size(); i++)
    m_splineInfo[i].save(fout);


  for(int i=0; i<m_captions.size(); i++)
    m_captions[i].save(fout);

  for(int i=0; i<m_colorbars.size(); i++)
    m_colorbars[i].save(fout);

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

  for(int i=0; i<m_grids.size(); i++)
    m_grids[i].save(fout);

  for(int i=0; i<m_crops.size(); i++)
    m_crops[i].save(fout);

  for(int i=0; i<m_pathgroups.size(); i++)
    m_pathgroups[i].save(fout);


  for(int i=0; i<m_trisets.size(); i++)
    m_trisets[i].save(fout);

  for(int i=0; i<m_networks.size(); i++)
    m_networks[i].save(fout);

  memset(keyword, 0, 100);
  sprintf(keyword, "tagcolors");
  fout.write((char*)keyword, strlen(keyword)+1);
  int tcn = 1024;
  fout.write((char*)&tcn, sizeof(int));
  fout.write((char*)m_tagColors, 1024);


  if (!m_pruneBuffer.isEmpty())
    {
      memset(keyword, 0, 100);
      sprintf(keyword, "pruneblend");
      fout.write((char*)keyword, strlen(keyword)+1);
      fout.write((char*)&m_pruneBlend, sizeof(bool));

      int pn = m_pruneBuffer.count();
      memset(keyword, 0, 100);
      sprintf(keyword, "prunebuffer");
      fout.write((char*)keyword, strlen(keyword)+1);
      fout.write((char*)&pn, sizeof(int));
      fout.write((char*)m_pruneBuffer.data(), pn);
    }
  
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
  sprintf(keyword, "interpfocus");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpFocus, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interptagcolors");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpTagColors, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interptickinfo");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpTickInfo, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interpvolumebounds");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpVolumeBounds, sizeof(int));

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

  memset(keyword, 0, 100);
  sprintf(keyword, "interpgilightinfo");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpGiLightInfo, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interptf");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpTF, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interpcrop");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpCrop, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interpmop");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpMop, sizeof(int));

  //------------------------------------------------------


  memset(keyword, 0, 100);
  sprintf(keyword, "keyframeend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

