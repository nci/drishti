#include "global.h"
#include "staticfunctions.h"
#include "networkobject.h"
#include "matrix.h"
#include <ncVar.h>
#include <ncFile.h>
#include <ncDim.h>
#include <ncException.h>
#include <netcdf>
#include <map>
using namespace netCDF;
using namespace netCDF::exceptions;

void NetworkObject::setScale(float s) { m_scaleV = m_scaleE = s; }
float NetworkObject::scaleV() { return m_scaleV; }
void NetworkObject::setScaleV(float s) { m_scaleV = s; }
float NetworkObject::scaleE() { return m_scaleE; }
void NetworkObject::setScaleE(float s) { m_scaleE = s; }

void
NetworkObject::gridSize(int &nx, int &ny, int &nz)
{
  nx = m_nX;
  ny = m_nY;
  nz = m_nZ;
}

void
NetworkObject::setVstops(QGradientStops stops)
{
  m_Vstops = stops;
  m_resampledVstops = StaticFunctions::resampleGradientStops(stops);
}

void
NetworkObject::setEstops(QGradientStops stops)
{
  m_Estops = stops;
  m_resampledEstops = StaticFunctions::resampleGradientStops(stops);
}

QString
NetworkObject::vertexAttributeString(int va)
{
  if (va < m_vertexAttribute.count())
    return m_vertexAttribute[va].first;

  return QString();
}

QString
NetworkObject::edgeAttributeString(int ea)
{
  if (ea < m_edgeAttribute.count())
    return m_edgeAttribute[ea].first;

  return QString();
}


bool
NetworkObject::vMinmax(int va, float& vmin, float& vmax)
{
  if (va < m_vertexAttribute.count())
    {
      vmin = m_Vminmax[va].first;
      vmax = m_Vminmax[va].second;
      return true;
    }
  return false;
}

bool
NetworkObject::userVminmax(int va, float& vmin, float& vmax)
{
  if (va < m_vertexAttribute.count())
    {
      vmin = m_userVminmax[va].first;
      vmax = m_userVminmax[va].second;
      return true;
    }
  return false;
}

void
NetworkObject::setVminmax(int va, float vmin, float vmax)
{
  if (va < m_vertexAttribute.count())
    m_Vminmax[va] = qMakePair(vmin, vmax);
}

void
NetworkObject::setUserVminmax(int va, float vmin, float vmax)
{
  if (va < m_vertexAttribute.count())
    m_userVminmax[va] = qMakePair(vmin, vmax);
}
void
NetworkObject::setUserVminmax(float vmin, float vmax)
{
  m_userVminmax[m_Vatt] = qMakePair(vmin, vmax);
}


bool
NetworkObject::eMinmax(int ea, float& emin, float& emax)
{
  if (ea < m_edgeAttribute.count())
    {
      emin = m_Eminmax[ea].first;
      emax = m_Eminmax[ea].second;
      return true;
    }
  return false;
}

bool
NetworkObject::userEminmax(int ea, float& emin, float& emax)
{
  if (ea < m_edgeAttribute.count())
    {
      emin = m_userEminmax[ea].first;
      emax = m_userEminmax[ea].second;
      return true;
    }
  return false;
}

void
NetworkObject::setEminmax(int ea, float emin, float emax)
{
  if (ea < m_edgeAttribute.count())
    m_Eminmax[ea] = qMakePair(emin, emax);
}

void
NetworkObject::setUserEminmax(int ea, float emin, float emax)
{
  if (ea < m_edgeAttribute.count())
    m_userEminmax[ea] = qMakePair(emin, emax);
}
void
NetworkObject::setUserEminmax(float emin, float emax)
{
  m_userEminmax[m_Eatt] = qMakePair(emin, emax);
}



NetworkObject::NetworkObject() { clear(); }

NetworkObject::~NetworkObject() { clear(); }

void
NetworkObject::enclosingBox(Vec &boxMin,
			    Vec &boxMax)
{
  boxMin = m_enclosingBox[0];
  boxMax = m_enclosingBox[6];
}

void
NetworkObject::clear()
{
  m_show = true;

  m_fileName.clear();
  m_centroid = Vec(0,0,0);
  m_nX = m_nY = m_nZ = 0;
  m_Vopacity = 1.0;
  m_Eopacity = 1.0;

  m_Vstops << QGradientStop(0.0, QColor(250,230,200,255))
	   << QGradientStop(1.0, QColor(200,100,50,255));
  m_resampledVstops = StaticFunctions::resampleGradientStops(m_Vstops);

  m_Estops << QGradientStop(0.0, QColor(200,230,250,255))
	   << QGradientStop(1.0, QColor(50,100,200,255));
  m_resampledEstops = StaticFunctions::resampleGradientStops(m_Estops);


  m_Vatt = 0;
  m_Eatt = 0;

  m_Vminmax.clear();
  m_Eminmax.clear();
  m_userVminmax.clear();
  m_userEminmax.clear();

  m_Vvertices.clear();
  m_Vovertices.clear();
  m_Vocolor.clear();
  m_VtexValues.clear();

  m_Evertices.clear();
  m_Eovertices.clear();
  m_EtexValues.clear();

  m_scaleV = 1;
  m_scaleE = 1;
  m_vertexRadiusAttribute = -1;
  m_edgeRadiusAttribute = -1;
  m_vertexAttribute.clear();
  m_edgeAttribute.clear();
  m_vertexCenters.clear();
  m_edgeNeighbours.clear();
}

bool
NetworkObject::load(QString flnm)
{
  if (StaticFunctions::checkExtension(flnm, "network"))
    return loadTextNetwork(flnm);
  else if (StaticFunctions::checkExtension(flnm, "graphml"))
    return loadGraphML(flnm);
  else
    return loadNetCDF(flnm);

  return false;
}

bool
NetworkObject::loadTextNetwork(QString flnm)
{
  m_fileName = flnm;
  
  m_nodeAtt.clear();
  m_edgeAtt.clear();

  QFile fl(m_fileName);
  if (!fl.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  QTextStream in(&fl);
  QString line;
  QStringList words;
  int nvert = 0;
  int nedge = 0;
  int nva = 0;
  int nea = 0;

  line = in.readLine();
  words = line.split(" ", QString::SkipEmptyParts);
  nvert = words[0].toInt();
  if (words.count() > 1)
    nva = words[1].toInt();

  line = in.readLine();
  words = line.split(" ", QString::SkipEmptyParts);
  nedge = words[0].toInt();
  if (words.count() > 1)
    nea = words[1].toInt();

  for(int i=0; i<nva; i++)
    {
      line = in.readLine();
      words = line.split("#", QString::SkipEmptyParts);
      m_nodeAtt << words[0];
    }
    
  for(int i=0; i<nea; i++)
    {
      line = in.readLine();
      words = line.split("#", QString::SkipEmptyParts);
      m_edgeAtt << words[0];
    }

  m_vertexRadiusAttribute = -1;
  m_edgeRadiusAttribute = -1;

  for(int i=0; i<m_nodeAtt.count(); i++)
    {
      if (m_nodeAtt[i].contains("radius", Qt::CaseInsensitive) ||
	  m_nodeAtt[i].contains("diameter", Qt::CaseInsensitive))
	{
	  m_vertexRadiusAttribute = i;
	  break;
	}
    }
  for(int i=0; i<m_edgeAtt.count(); i++)
    {
      if (m_edgeAtt[i].contains("radius", Qt::CaseInsensitive) ||
	  m_edgeAtt[i].contains("diameter", Qt::CaseInsensitive))
	{
	  m_edgeRadiusAttribute = i;
	  break;
	}
    }

  //------------------------------------
  // read node information
  {
    m_vertexAttribute.clear();
    m_vertexCenters.clear();

    QVector<float> *vat; // not expecting more than 20 attributes
    int nvat = m_nodeAtt.count();
    vat = new QVector<float> [nvat];

    for(int i=0; i<nvert; i++)
      {
	line = in.readLine();
	line = line.simplified();
	words = line.split(" ", QString::SkipEmptyParts);
	float x, y, z;
	x = words[0].toFloat();
	y = words[1].toFloat();
	z = words[2].toFloat();
	m_vertexCenters << Vec(x,y,z);
	for (int j=0; j<qMin((int)(words.count()-3),nvat); j++)
	  vat[j] << words[3+j].toFloat();
      }
    
    int nv = m_vertexCenters.count();
    for(int c=0; c<m_nodeAtt.count(); c++)
      {
	if (vat[c].count() < nv)
	  vat[c] << 0.0;
      }
    
    for(int vi=0; vi<m_nodeAtt.count(); vi++)
      m_vertexAttribute << qMakePair(m_nodeAtt[vi], vat[vi]);      

    if (m_vertexRadiusAttribute == -1)
      {
	m_vertexRadiusAttribute = m_nodeAtt.count();
	QVector<float> rad;
	rad.resize(m_vertexCenters.count());
	rad.fill(2);
	m_nodeAtt << "vertex_radius";
	m_vertexAttribute << qMakePair(QString("vertex_radius"), rad);
      }
    
    for(int i=0; i<nvat; i++)
      vat[i].clear();
    delete [] vat;
  }
  //------------------------------------
  
  //------------------------------------
  // read edge information
  {
    m_edgeAttribute.clear();
    m_edgeNeighbours.clear();

    QVector<float> *vat; // not expecting more than 20 attributes
    int nvat = m_edgeAtt.count();
    vat = new QVector<float> [nvat];

    for(int i=0; i<nedge; i++)
      {
	line = in.readLine();
	line = line.simplified();
	words = line.split(" ", QString::SkipEmptyParts);
	int a, b;
	a = words[0].toInt();
	b = words[1].toInt();
	m_edgeNeighbours << qMakePair(a,b);
	for (int j=0; j<qMin((int)(words.count()-2),nvat); j++)
	  vat[j] << words[2+j].toFloat();
      }
    
    int nv = m_edgeNeighbours.count();
    for(int c=0; c<m_edgeAtt.count(); c++)
      {
	if (vat[c].count() < nv)
	  vat[c] << 0.0;
      }

    for(int e=0; e<m_edgeAtt.count(); e++)
      {
	if (m_edgeAtt[e].contains("diameter", Qt::CaseInsensitive))
	  {
	    for(int vi=0; vi<vat[e].count(); vi++)
	      vat[e][vi] /= 2;
	  }
      }

    for(int vi=0; vi<m_edgeAtt.count(); vi++)
      m_edgeAttribute << qMakePair(m_edgeAtt[vi], vat[vi]);      
    
    if (m_edgeRadiusAttribute == -1)
      {
	m_edgeRadiusAttribute = m_edgeAtt.count();
	QVector<float> rad;
	rad.resize(m_edgeNeighbours.count());
	rad.fill(2);
	m_edgeAtt << "edge_radius";
	m_edgeAttribute << qMakePair(QString("edge_radius"), rad);
      }
    
    for(int i=0; i<nvat; i++)
      vat[i].clear();
    delete [] vat;
  }
  //------------------------------------
    
  //---------------------
  Vec bmin = m_vertexCenters[0];
  Vec bmax = m_vertexCenters[0];
  for(int i=0; i<m_vertexCenters.count(); i++)
    {
      bmin = StaticFunctions::minVec(bmin, m_vertexCenters[i]);
      bmax = StaticFunctions::maxVec(bmax, m_vertexCenters[i]);
    }
  m_centroid = (bmin + bmax)/2;
  
  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);


//  m_nZ = (bmax.x - bmin.x) + 1;
//  m_nY = (bmax.y - bmin.y) + 1;
//  m_nX = (bmax.z - bmin.z) + 1;
  m_nZ = bmax.x;
  m_nY = bmax.y;
  m_nX = bmax.z;

  if (!Global::batchMode())
    {
      QString str;
      str = QString("Grid Size : %1 %2 %3\n").arg(m_nX).arg(m_nY).arg(m_nZ);
      str += QString("Vertices : %1\n").arg(m_vertexCenters.count());
      str += QString("Edges : %1\n").arg(m_edgeNeighbours.count());
      str += QString("\n");
      str += QString("Vertex Attributes : %1\n").arg(m_vertexAttribute.count());
      for(int i=0; i<m_vertexAttribute.count(); i++)
	str += QString(" %1\n").arg(m_vertexAttribute[i].first);
      str += QString("\n");
      str += QString("Edge Attributes : %1\n").arg(m_edgeAttribute.count());
      for(int i=0; i<m_edgeAttribute.count(); i++)
	str += QString(" %1\n").arg(m_edgeAttribute[i].first);
      
      QMessageBox::information(0, "Network loaded", str);
    }

  m_Vatt = 0;
  m_Eatt = 0;
  m_Vminmax.clear();
  m_Eminmax.clear();
  m_userVminmax.clear();
  m_userEminmax.clear();

  for(int i=0; i<m_vertexAttribute.count(); i++)
    {
      float vmin = m_vertexAttribute[i].second[0];
      float vmax = m_vertexAttribute[i].second[0];
      for(int j=1; j<m_vertexAttribute[i].second.count(); j++)
	{
	  vmin = qMin((float)m_vertexAttribute[i].second[j], vmin);
	  vmax = qMax((float)m_vertexAttribute[i].second[j], vmax);
	}
      m_Vminmax.append(qMakePair(vmin, vmax));
      m_userVminmax.append(qMakePair((vmin+vmax)/2, vmax));
    }

  for(int i=0; i<m_edgeAttribute.count(); i++)
    {
      float emin = m_edgeAttribute[i].second[0];
      float emax = m_edgeAttribute[i].second[0];
      for(int j=1; j<m_edgeAttribute[i].second.count(); j++)
	{
	  emin = qMin((float)m_edgeAttribute[i].second[j], emin);
	  emax = qMax((float)m_edgeAttribute[i].second[j], emax);
	}
      m_Eminmax.append(qMakePair(emin, emax));
      m_userEminmax.append(qMakePair((emin+emax)/2, emax));
    }

  return true;
}

bool
NetworkObject::loadNetCDF(QString flnm)
{
#if defined(Q_OS_WIN32)
  m_fileName = flnm;

  NcFile ncfFile;
  try
    {
      ncfFile.open(flnm.toStdString(), NcFile::read);
    }
  catch(NcException &e)
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid NetCDF file").\
			       arg(flnm));
      return false;
    }
  
  // ---- get gridsize -----
  NcGroupAtt att = ncfFile.getAtt("gridsize");
  double values[10];
  att.getValues(&values[0]);
  m_nX = values[0];
  m_nY = values[1];
  m_nZ = values[2];
  //------------------------

  int nv = 0; // number of vertices  
  // ---- get vertex centers -----
  {
    NcVar var = ncfFile.getVar("vertex_centers");
    if (var.isNull())
      var = ncfFile.getVar("vertex_center");
    if (var.isNull())
      var = ncfFile.getVar("vertex_centres");
    if (var.isNull())
      var = ncfFile.getVar("vertex_centre");
    
    nv = var.getDim(0).getSize();
    float *vc = new float [3*nv];
    var.getVar(vc);
    m_vertexCenters.resize(nv);
    for(int i=0; i<nv; i++)
      m_vertexCenters[i] = Vec(vc[3*i+0], vc[3*i+1], vc[3*i+2]);
    delete [] vc;
  }
  //------------------------

  int ne = 0;  // number of edges
  // ---- get edges -----
  {
    NcVar var = ncfFile.getVar("edge_neighbours");
    ne = var.getDim(0).getSize();
    int *ed = new int [2*ne];
    var.getVar(ed);
    m_edgeNeighbours.resize(ne);
    for(int i=0; i<ne; i++)
      m_edgeNeighbours[i] = qMakePair(ed[2*i], ed[2*i+1]);
    delete [] ed;
  }
  //------------------------


  Vec bmin = m_vertexCenters[0];
  Vec bmax = m_vertexCenters[0];
  for(int i=0; i<m_vertexCenters.count(); i++)
    {
      bmin = StaticFunctions::minVec(bmin, m_vertexCenters[i]);
      bmax = StaticFunctions::maxVec(bmax, m_vertexCenters[i]);
    }
  m_centroid = (bmin + bmax)/2;
  
  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);
    


  multimap<string, NcVar> groupMap;
  groupMap = ncfFile.getVars();
  QList<QString> varNames;
  for (const auto &p : groupMap)
    {
      varNames.append(QString::fromStdString(p.first));
    }
  int nvars = varNames.count();

  
  m_vertexAttribute.clear();
  m_edgeAttribute.clear();

  m_vertexRadiusAttribute = -1;
  m_edgeRadiusAttribute = -1;

  int vri = 0;
  int eri = 0;
  for (int i=0; i < nvars; i++)
    {
      NcVar var = ncfFile.getVar(varNames[i].toStdString());      
      QString attname = varNames[i];
      attname.toLower();

      if (attname.contains("vertex_") &&
	  attname != "vertex_center" &&
	  attname != "vertex_centre" &&
	  attname != "vertex_centers" &&
	  attname != "vertex_centres")
	{
	  if (attname == "vertex_radius")
	    m_vertexRadiusAttribute = vri;
	  vri++;

	  QVector<float> val;
	  val.clear();

	  if (var.getType() == ncByte || var.getType() == ncChar)
	    {
	      uchar *v = new uchar[nv];
	      var.getVar((unsigned char*)v);
	      for(int j=0; j<nv; j++)
		val.append(v[j]);
	      delete [] v;
	    }
	  else if (var.getType() == ncShort)
	    {
	      short *v = new short[nv];
	      var.getVar((short *)v);
	      for(int j=0; j<nv; j++)
		val.append(v[j]);
	      delete [] v;
	    }
	  else if (var.getType() == ncInt)
	    {
	      int *v = new int[nv];
	      var.getVar((int *)v);
	      for(int j=0; j<nv; j++)
		val.append(v[j]);
	      delete [] v;
	    }
	  else if (var.getType() == ncFloat)
	    {
	      float *v = new float[nv];
	      var.getVar((float *)v);
	      for(int j=0; j<nv; j++)
		val.append(v[j]);
	      delete [] v;
	    }

	  if (val.count() > 0)
	    m_vertexAttribute.append(qMakePair(attname, val));
	}
      else if (attname.contains("edge_") &&
	       attname != "edge_neighbours")
	{
	  if (attname == "edge_radius")
	    m_edgeRadiusAttribute = eri;
	  eri++;

	  QVector<float> val;
	  val.clear();

	  if (var.getType() == ncByte || var.getType() == ncChar)
	    {
	      uchar *v = new uchar[ne];
	      var.getVar((unsigned char*)v);
	      for(int j=0; j<ne; j++)
		val.append(v[j]);
	      delete [] v;
	    }
	  else if (var.getType() == ncShort)
	    {
	      short *v = new short[ne];
	      var.getVar((short *)v);
	      for(int j=0; j<ne; j++)
		val.append(v[j]);
	      delete [] v;
	    }
	  else if (var.getType() == ncInt)
	    {
	      int *v = new int[ne];
	      var.getVar((int *)v);
	      for(int j=0; j<ne; j++)
		val.append(v[j]);
	      delete [] v;
	    }
	  else if (var.getType() == ncFloat)
	    {
	      float *v = new float[ne];
	      var.getVar((float *)v);
	      for(int j=0; j<ne; j++)
		val.append(v[j]);
	      delete [] v;
	    }

	  if (val.count() > 0)
	    m_edgeAttribute.append(qMakePair(attname, val));
	}
    }

  ncfFile.close();
  
  if (!Global::batchMode())
    {
      QString str;
      str = QString("Grid Size : %1 %2 %3\n").arg(m_nX).arg(m_nY).arg(m_nZ);
      str += QString("Vertices : %1\n").arg(m_vertexCenters.count());
      str += QString("Edges : %1\n").arg(m_edgeNeighbours.count());
      str += QString("\n");
      str += QString("Vertex Attributes : %1\n").arg(m_vertexAttribute.count());
      for(int i=0; i<m_vertexAttribute.count(); i++)
	str += QString(" %1\n").arg(m_vertexAttribute[i].first);
      str += QString("\n");
      str += QString("Edge Attributes : %1\n").arg(m_edgeAttribute.count());
      for(int i=0; i<m_edgeAttribute.count(); i++)
	str += QString(" %1\n").arg(m_edgeAttribute[i].first);
      
      QMessageBox::information(0, "Network loaded", str);
    }

  m_Vatt = 0;
  m_Eatt = 0;
  m_Vminmax.clear();
  m_Eminmax.clear();
  m_userVminmax.clear();
  m_userEminmax.clear();

  for(int i=0; i<m_vertexAttribute.count(); i++)
    {
      float vmin = m_vertexAttribute[i].second[0];
      float vmax = m_vertexAttribute[i].second[0];
      for(int j=1; j<m_vertexAttribute[i].second.count(); j++)
	{
	  vmin = qMin((float)m_vertexAttribute[i].second[j], vmin);
	  vmax = qMax((float)m_vertexAttribute[i].second[j], vmax);
	}
      m_Vminmax.append(qMakePair(vmin, vmax));
      m_userVminmax.append(qMakePair((vmin+vmax)/2, vmax));
    }


  for(int i=0; i<m_edgeAttribute.count(); i++)
    {
      float emin = m_edgeAttribute[i].second[0];
      float emax = m_edgeAttribute[i].second[0];
      for(int j=1; j<m_edgeAttribute[i].second.count(); j++)
	{
	  emin = qMin((float)m_edgeAttribute[i].second[j], emin);
	  emax = qMax((float)m_edgeAttribute[i].second[j], emax);
	}
      m_Eminmax.append(qMakePair(emin, emax));
      m_userEminmax.append(qMakePair((emin+emax)/2, emax));
    }
#endif
  return true;
}

bool
NetworkObject::save(QString flnm)
{
  return true;
}


void
NetworkObject::postdraw(QGLViewer *viewer,
		       int x, int y,
		       bool active, int idx)
{
  if (!m_show || !active)
    return;

  viewer->startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  QString str = QString("network %1").arg(idx);
  QFont font = QFont();
  QFontMetrics metric(font);
  int ht = metric.height();
  int wd = metric.width(str);
  //x -= wd/2;
  x += 10;

  StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::white);

  viewer->stopScreenCoordinatesSystem();
}

void
NetworkObject::draw(QGLViewer *viewer,
		    bool active,
		    float pnear, float pfar,
		    bool backToFront)
{
  if (!m_show)
    return;

  if (active)
    {
      Vec lineColor = Vec(0.7f, 0.3f, 0.0f);
      StaticFunctions::drawEnclosingCube(m_tenclosingBox, lineColor);
    }
  
  if (m_Vopacity < 0.05 && m_Eopacity < 0.05)
    return;

  glShadeModel(GL_SMOOTH);

  // emissive when active
  if (active)
    {
      float emiss[] = { 0.5f, 0, 0, 1.0f };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }
  else
    {
      float emiss[] = { 0, 0, 0, 1.0f };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }

  drawNetwork(pnear, pfar, backToFront);

  { // reset emissivity
    float emiss[] = { 0, 0, 0, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
  }
}

void
NetworkObject::predraw(QGLViewer *viewer,
		       double *Xform,
		       Vec pn,
		       QList<Vec> clipPos,
		       QList<Vec> clipNormal,
		       QList<CropObject> crops,
		       Vec lightVector)
{
  if (!m_show)
    return;

  generateSphereSpriteTexture(lightVector);
  generateCylinderSpriteTexture(lightVector);


  predrawVertices(viewer, Xform, pn, clipPos, clipNormal, crops);
  predrawEdges(viewer, Xform, pn, clipPos, clipNormal, crops);

//  m_tcentroid = Matrix::xformVec(Xform, m_centroid);
//
//  for(int i=0; i<8; i++)
//    m_tenclosingBox[i] = Matrix::xformVec(Xform, m_enclosingBox[i]);

  Vec bmin, bmax;
  Global::bounds(bmin, bmax);

  Vec vmin = m_enclosingBox[0];
  Vec vmax = m_enclosingBox[6];

  vmin.x = qMax(vmin.x, bmin.x);
  vmin.y = qMax(vmin.y, bmin.y);
  vmin.z = qMax(vmin.z, bmin.z);
  vmax.x = qMin(vmax.x, bmax.x);
  vmax.y = qMin(vmax.y, bmax.y);
  vmax.z = qMin(vmax.z, bmax.z);

  Vec centroid = (vmin + vmax)/2;
  m_tcentroid = Matrix::xformVec(Xform, centroid);

  
  Vec box[8];
  box[0] = Vec(vmin.x, vmin.y, vmin.z);
  box[1] = Vec(vmax.x, vmin.y, vmin.z);
  box[2] = Vec(vmax.x, vmax.y, vmin.z);
  box[3] = Vec(vmin.x, vmax.y, vmin.z);
  box[4] = Vec(vmin.x, vmin.y, vmax.z);
  box[5] = Vec(vmax.x, vmin.y, vmax.z);
  box[6] = Vec(vmax.x, vmax.y, vmax.z);
  box[7] = Vec(vmin.x, vmax.y, vmax.z);
  
  for(int i=0; i<8; i++)
    m_tenclosingBox[i] = Matrix::xformVec(Xform, box[i]);

}

void
NetworkObject::generateSphereSpriteTexture(Vec lightVector)
{
  int texsize = 64;
  int t2 = texsize/2;
  int fx, fy;
  fx = t2 - (t2-1)*lightVector.x;
  fy = t2 + (t2-1)*lightVector.y;
  QRadialGradient rg(t2, t2,
		     t2-1,
		     fx, fy);
  rg.setColorAt(0.0, Qt::white);
  rg.setColorAt(1.0, Qt::black);

  QImage texImage(texsize, texsize, QImage::Format_ARGB32);
  texImage.fill(0);
  QPainter p(&texImage);
  p.setBrush(QBrush(rg));
  p.setPen(Qt::transparent);
  p.drawEllipse(0, 0, texsize, texsize);


  uchar *thetexture = new uchar[2*texsize*texsize];
  const uchar *bits = texImage.bits();
  for(int i=0; i<texsize*texsize; i++)
    {
      uchar lum = 255;
      float a = (float)bits[4*i+2]/255.0f;
      a = 1-a;

      if (a < 0.5)
	{
	  if (lightVector.z >= 0.0)
	    a = qMax(a/0.5f, 0.5f);
	  else
	    {
	      a = 0.7f;
	      lum = 50;
	    }
	}
      else if (a >= 1)
	{
	  a = 0;
	  lum = 0;
	}
      else
	{
	  if (lightVector.z >= 0.0)
	    {
	      a = 1-(a-0.5f)/0.5f;
	      lum *= a;
	      a = 0.9f;
	    }
	  else
	    {
	      lum *= 1-fabs(a-0.75f)/0.25f;
	      a = 0.9f;
	    }
	}

      a *= 255;

      thetexture[2*i] = lum;
      thetexture[2*i+1] = a;
    }


  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::sphereTexture());
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
	       texsize, texsize, 0,
	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
	       thetexture);

  delete [] thetexture;
}

void
NetworkObject::generateCylinderSpriteTexture(Vec lightVector)
{
  int texsize = 64;
  float fp = 0.5f+0.5f*lightVector.y;
  QLinearGradient lg(0, 0,
		     texsize, 0);
  lg.setColorAt(0.0, Qt::black);
  lg.setColorAt(1.0, Qt::black);
  lg.setColorAt(fp, Qt::white);

  QImage texImage(texsize, texsize, QImage::Format_ARGB32);
  texImage.fill(0);
  QPainter p(&texImage);
  p.setBrush(QBrush(lg));
  p.setPen(Qt::transparent);
  //p.drawEllipse(0, 0, texsize, texsize);
  p.drawRect(0, 0, texsize, texsize);


  uchar *thetexture = new uchar[2*texsize*texsize];
  const uchar *bits = texImage.bits();
  for(int i=0; i<texsize*texsize; i++)
    {
      uchar lum = 255;
      float a = (float)bits[4*i+2]/255.0f;
      a = 1-a;

      if (a < 0.5)
	{
	  if (lightVector.z >= 0.0)
	    a = qMax(a/0.5f, 0.5f);
	  else
	    {
	      a = 0.7f;
	      lum = 50;
	    }
	}
      else if (a >= 1)
	{
	  a = 0;
	  lum = 0;
	}
      else
	{
	  if (lightVector.z >= 0.0f)
	    {
	      a = 1-(a-0.5f)/0.5f;
	      lum *= a;
	      a = 0.9f;
	    }
	  else
	    {
	      lum *= 1-fabs(a-0.75f)/0.25f;
	      a = 0.9f;
	    }
	}

      a *= 255;

      thetexture[2*i] = lum;
      thetexture[2*i+1] = a;
    }


  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::cylinderTexture());
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
	       texsize, texsize, 0,
	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
	       thetexture);

  delete [] thetexture;
}

void
NetworkObject::predrawVertices(QGLViewer *viewer,
			       double *Xform,
			       Vec pn,
			       QList<Vec> clipPos,
			       QList<Vec> clipNormal,
			       QList<CropObject> crops)
{
  Vec bmin, bmax;
  Global::bounds(bmin, bmax);

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5 %6").\
//			   arg(bmin.x).arg(bmin.y).arg(bmin.z).\
//			   arg(bmax.x).arg(bmax.y).arg(bmax.z));

  m_Vovertices.resize(m_vertexCenters.count());
  m_Vocolor.resize(m_vertexCenters.count());

  QVector<float> rad;
  rad.resize(m_vertexCenters.count());

  int stopsCount = m_resampledVstops.count()-1;
  float aint = (m_userVminmax[m_Vatt].second-m_userVminmax[m_Vatt].first);
  if (aint <= 0.0001) aint = 1;

  QString txt;
  int nv=0;
  for(int i=0; i<m_vertexCenters.count(); i++)
    {
      bool ok = true;

      if (m_vertexAttribute[m_Vatt].second[i] < m_userVminmax[m_Vatt].first ||
	  m_vertexAttribute[m_Vatt].second[i] > m_userVminmax[m_Vatt].second)
	ok = false;

      if (ok)
	{
	  for(int c=0; c<clipPos.count(); c++)
	    {
	      if ((m_vertexCenters[i]-clipPos[c])*clipNormal[c] > 0)
		{
		  ok = false;
		  break;
		}
	    }

	  if (ok)
	    {
	      for(int ci=0; ci<crops.count(); ci++)
		{
		  ok &= crops[ci].checkCropped(m_vertexCenters[i]);
		  if (!ok) break;
		}
	    }

	  if (ok)
	    {
	      if (m_vertexCenters[i].x >= bmin.x &&
		  m_vertexCenters[i].y >= bmin.y &&
		  m_vertexCenters[i].z >= bmin.z &&
		  m_vertexCenters[i].x <= bmax.x &&
		  m_vertexCenters[i].y <= bmax.y &&
		  m_vertexCenters[i].z <= bmax.z)
		{
		  m_Vovertices[nv] = Matrix::xformVec(Xform, m_vertexCenters[i]);

		  //rad[nv] = 1.5*m_vertexAttribute[m_vertexRadiusAttribute].second[i];
		  rad[nv] = m_scaleV * m_vertexAttribute[m_vertexRadiusAttribute].second[i];

		  float stp = (m_vertexAttribute[m_Vatt].second[i] -
			     m_userVminmax[m_Vatt].first) / aint;
		  QColor col = m_resampledVstops[stp*stopsCount].second;
		  float r = col.red()/255.0;
		  float g = col.green()/255.0;
		  float b = col.blue()/255.0;
		  m_Vocolor[nv] = Vec(r,g,b);
		  nv++;
		}
	    }
	}
    }
  m_Vovertices.resize(nv+1);

  m_VtexValues.resize(m_Vovertices.count());
  for(int i=0; i<m_Vovertices.count(); i++)
    m_VtexValues[i] = pn*m_Vovertices[i];


  Vec p = pn^viewer->camera()->upVector();
  Vec q = pn^p;
  p.normalize();
  q.normalize();


  m_Vvertices.resize(4*m_Vovertices.count());
  for(int i=0; i<m_Vovertices.count(); i++)
    {
      m_Vvertices[4*i+0] = m_Vovertices[i]-rad[i]*p-rad[i]*q;
      m_Vvertices[4*i+1] = m_Vovertices[i]-rad[i]*p+rad[i]*q;
      m_Vvertices[4*i+2] = m_Vovertices[i]+rad[i]*p+rad[i]*q;
      m_Vvertices[4*i+3] = m_Vovertices[i]+rad[i]*p-rad[i]*q;
    }
  
}

void
NetworkObject::predrawEdges(QGLViewer *viewer,
			    double *Xform,
			    Vec pn,
			    QList<Vec> clipPos,
			    QList<Vec> clipNormal,
			    QList<CropObject> crops)
{
  Vec bmin, bmax;
  Global::bounds(bmin, bmax);

  m_Eovertices.resize(2*m_edgeNeighbours.count());
  m_Eocolor.resize(m_edgeNeighbours.count());

  QVector<float> rad;
  rad.resize(m_edgeNeighbours.count());

  int stopsCount = m_resampledEstops.count()-1;
  float aint = (m_userEminmax[m_Eatt].second-m_userEminmax[m_Eatt].first);
  if (aint <= 0.0001) aint = 1;

  int nv = 0;
  for(int i=0; i<m_edgeNeighbours.count(); i++)
    {
      bool ok = true;

      if (m_edgeAttribute[m_Eatt].second[i] < m_userEminmax[m_Eatt].first ||
	  m_edgeAttribute[m_Eatt].second[i] > m_userEminmax[m_Eatt].second)
	ok = false;

      if (ok)
	{
	  int a = m_edgeNeighbours[i].first;
	  int b = m_edgeNeighbours[i].second;

	  Vec pa = m_vertexCenters[a];
	  Vec pb = m_vertexCenters[b];

	  for(int c=0; c<clipPos.count(); c++)
	    {
	      if ((pa-clipPos[c])*clipNormal[c] > 0 ||
		  (pb-clipPos[c])*clipNormal[c] > 0)
		{
		  ok = false;
		  break;
		}
	    }

	  if (ok)
	    {
	      for(int ci=0; ci<crops.count(); ci++)
		{
		  ok &= crops[ci].checkCropped(pa);
		  if (!ok) break;
		  ok &= crops[ci].checkCropped(pb);
		  if (!ok) break;
		}
	    }

	  if (ok)
	    {
	      pa = Matrix::xformVec(Xform, pa);
	      pb = Matrix::xformVec(Xform, pb);
	      
	      if (m_vertexCenters[a].x >= bmin.x &&
		  m_vertexCenters[a].y >= bmin.y &&
		  m_vertexCenters[a].z >= bmin.z &&
		  m_vertexCenters[a].x <= bmax.x &&
		  m_vertexCenters[a].y <= bmax.y &&
		  m_vertexCenters[a].z <= bmax.z &&
		  m_vertexCenters[b].x >= bmin.x &&
		  m_vertexCenters[b].y >= bmin.y &&
		  m_vertexCenters[b].z >= bmin.z &&
		  m_vertexCenters[b].x <= bmax.x &&
		  m_vertexCenters[b].y <= bmax.y &&
		  m_vertexCenters[b].z <= bmax.z)
		{
		  if (m_Vopacity > 0.5)
		    {
		      Vec p = pb-pa;
		      p.normalize();
		      pa = pa + p*m_vertexAttribute[m_vertexRadiusAttribute].second[a];
		      pb = pb - p*m_vertexAttribute[m_vertexRadiusAttribute].second[b];
		    }
		  
		  m_Eovertices[2*nv] = pa;
		  m_Eovertices[2*nv+1] = pb;		  
		  
		  //rad[nv] = m_edgeAttribute[m_edgeRadiusAttribute].second[i];
		  rad[nv] = m_scaleE * m_edgeAttribute[m_edgeRadiusAttribute].second[i];

		  float stp = (m_edgeAttribute[m_Eatt].second[i] -
			     m_userEminmax[m_Eatt].first) / aint;
		  QColor col = m_resampledEstops[stp*stopsCount].second;
		  float r = col.red()/255.0;
		  float g = col.green()/255.0;
		  float b = col.blue()/255.0;
		  m_Eocolor[nv] = Vec(r,g,b);

		  nv ++;
		}
	    }
	}
    }
  m_Eovertices.resize(2*(nv+1));  

  m_EtexValues.resize(m_Eovertices.count());
  for(int i=0; i<m_Eovertices.count(); i++)
    m_EtexValues[i] = pn*m_Eovertices[i];


  Vec p = pn^viewer->camera()->upVector();
  Vec q = pn^p;
  p.normalize();
  q.normalize();

  m_Evertices.resize(2*m_Eovertices.count());
  for(int i=0; i<m_Eovertices.count()/2; i++)
    {
      Vec pa = m_Eovertices[2*i];
      Vec pb = m_Eovertices[2*i+1];
      Vec p = pn^(pa-pb);
      p.normalize();

      p *= rad[i];

      m_Evertices[4*i+0] = pa-p;
      m_Evertices[4*i+1] = pb-p;
      m_Evertices[4*i+2] = pb+p;
      m_Evertices[4*i+3] = pa+p;
    }
  
}

void
NetworkObject::drawNetwork(float pnear, float pfar,
			   bool backToFront)
{
  if (backToFront)
    {
      if (m_Eopacity > 0.01) drawEdges(pnear, pfar);
      if (m_Vopacity > 0.01) drawVertices(pnear, pfar);
    }
  else
    {
      if (m_Vopacity > 0.01) drawVertices(pnear, pfar);
      if (m_Eopacity > 0.01) drawEdges(pnear, pfar);
    }
}

void
NetworkObject::drawEdges(float pnear, float pfar)
{
//  glColor4f(m_Ecolor.x*m_Eopacity,
//	    m_Ecolor.y*m_Eopacity,
//	    m_Ecolor.z*m_Eopacity,
//	    m_Eopacity);
  
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::cylinderTexture());
  glBegin(GL_QUADS);
  for(int i=0; i<m_Eovertices.count()/2; i++)
    {
      if ( ! (m_EtexValues[2*i]   < pnear &&
	      m_EtexValues[2*i+1] < pnear) ||
	   (m_EtexValues[2*i]   > pfar && 
	    m_EtexValues[2*i+1] > pfar) )
	{
	  glColor4f(m_Eocolor[i].x*m_Eopacity,
		    m_Eocolor[i].y*m_Eopacity,
		    m_Eocolor[i].z*m_Eopacity,
		    m_Eopacity);

	  glTexCoord2f(0, 0);
	  glVertex3fv(m_Evertices[4*i+0]);
	  
	  glTexCoord2f(0, 1);
	  glVertex3fv(m_Evertices[4*i+1]);
	  
	  glTexCoord2f(1, 1);
	  glVertex3fv(m_Evertices[4*i+2]);
	  
	  glTexCoord2f(1, 0);
	  glVertex3fv(m_Evertices[4*i+3]);
	}
    }
  glEnd();
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
}

void
NetworkObject::drawVertices(float pnear, float pfar)
{

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::sphereTexture());
  glBegin(GL_QUADS);
  for(int i=0; i<m_Vovertices.count(); i++)
    {
      if ( m_VtexValues[i] >= pnear &&
	   m_VtexValues[i] <= pfar )
	{
	  glColor4f(m_Vocolor[i].x*m_Vopacity,
		    m_Vocolor[i].y*m_Vopacity,
		    m_Vocolor[i].z*m_Vopacity,
		    m_Vopacity);

	  glTexCoord2f(0, 0);
	  glVertex3fv(m_Vvertices[4*i+0]);
	  
	  glTexCoord2f(0, 1);
	  glVertex3fv(m_Vvertices[4*i+1]);
	  
	  glTexCoord2f(1, 1);
	  glVertex3fv(m_Vvertices[4*i+2]);
	  
	  glTexCoord2f(1, 0);
	  glVertex3fv(m_Vvertices[4*i+3]);
	}
    }
  glEnd();
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
}

QDomElement
NetworkObject::domElement(QDomDocument &doc)
{
  QDomElement de = doc.createElement("network");
  
  {
    QDomElement de0 = doc.createElement("name");
    QDomText tn0 = doc.createTextNode(m_fileName);
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("vopacity");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_Vopacity));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("vstops");
    QString str;
    for(int s=0; s<m_Vstops.count(); s++)
      {
	float pos = m_Vstops[s].first;
	QColor col = m_Vstops[s].second;
	int r = col.red();
	int g = col.green();
	int b = col.blue();
	int a = col.alpha();
	str += QString("%1 %2 %3 %4 %5 ").\
	  arg(pos).arg(r).arg(g).arg(b).arg(a);
      }
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("eopacity");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_Eopacity));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
    
  {
    QDomElement de0 = doc.createElement("estops");
    QString str;
    for(int s=0; s<m_Estops.count(); s++)
      {
	float pos = m_Estops[s].first;
	QColor col = m_Estops[s].second;
	int r = col.red();
	int g = col.green();
	int b = col.blue();
	int a = col.alpha();
	str += QString("%1 %2 %3 %4 %5 ").\
	  arg(pos).arg(r).arg(g).arg(b).arg(a);
      }
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  return de;
}

bool
NetworkObject::fromDomElement(QDomElement de)
{
  clear();

  bool ok = false;

  QString name;
  QDomNodeList dlist = de.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      QDomElement dnode = dlist.at(i).toElement();
      QString str = dnode.toElement().text();
      if (dnode.tagName() == "name")
	ok = load(str);
      else if (dnode.tagName() == "vopacity")
	m_Vopacity = str.toFloat();
      else if (dnode.tagName() == "vstops")
	{
	  QStringList xyz = str.split(" ");
	  for(int s=0; s<xyz.count()/5; s++)
	    {
	      float pos;
	      int r,g,b,a;
	      pos = xyz[5*s+0].toFloat();
	      r = xyz[5*s+1].toInt();
	      g = xyz[5*s+2].toInt();
	      b = xyz[5*s+3].toInt();
	      a = xyz[5*s+4].toInt();
	      m_Vstops << QGradientStop(pos, QColor(r,g,b,a));
	    }
	}
      else if (dnode.tagName() == "eopacity")
	m_Eopacity = str.toFloat();
      else if (dnode.tagName() == "estops")
	{
	  QStringList xyz = str.split(" ");
	  for(int s=0; s<xyz.count()/5; s++)
	    {
	      float pos;
	      int r,g,b,a;
	      pos = xyz[5*s+0].toFloat();
	      r = xyz[5*s+1].toInt();
	      g = xyz[5*s+2].toInt();
	      b = xyz[5*s+3].toInt();
	      a = xyz[5*s+4].toInt();
	      m_Vstops << QGradientStop(pos, QColor(r,g,b,a));
	    }
	}
    }

  return ok;
}

NetworkInformation
NetworkObject::get()
{
  NetworkInformation ti;
  ti.filename = m_fileName;
  ti.Vopacity = m_Vopacity;
  ti.Eopacity = m_Eopacity;
  ti.Vatt = m_Vatt;
  ti.Eatt = m_Eatt;
  ti.userVmin = m_userVminmax[m_Vatt].first;
  ti.userVmax = m_userVminmax[m_Vatt].second;
  ti.userEmin = m_userEminmax[m_Eatt].first;
  ti.userEmax = m_userEminmax[m_Eatt].second;
  ti.Vstops = m_Vstops;
  ti.Estops = m_Estops;
  ti.scalee = m_scaleE;
  ti.scalev = m_scaleV;

  return ti;
}

bool
NetworkObject::set(NetworkInformation ti)
{
  bool ok = false;

  if (m_fileName != ti.filename)
    ok = load(ti.filename);
  else
    ok = true;


  m_Vopacity = ti.Vopacity;
  m_Eopacity = ti.Eopacity;
  m_Vatt = ti.Vatt;
  m_Eatt = ti.Eatt;
  m_userVminmax[m_Vatt] = qMakePair(ti.userVmin, ti.userVmax);
  m_userEminmax[m_Eatt] = qMakePair(ti.userEmin, ti.userEmax);

  m_Vstops = ti.Vstops;
  m_resampledVstops = StaticFunctions::resampleGradientStops(m_Vstops);

  m_Estops = ti.Estops;
  m_resampledEstops = StaticFunctions::resampleGradientStops(m_Estops);

  m_scaleE = ti.scalee;
  m_scaleV = ti.scalev;

  return ok;
}

void
NetworkObject::getKey(QDomElement main)
{
  m_nodeAtt.clear();
  m_edgeAtt.clear();
  m_nodeAttName.clear();
  m_edgeAttName.clear();

  m_nodePosAttr.clear();
  m_nodePosAttr << "x";
  m_nodePosAttr << "y";
  m_nodePosAttr << "z";

  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).isElement())
	{
	  QDomElement ele = dlist.at(i).toElement();
	  QString str = ele.nodeName();
	  if (str == "key")
	    {
	      QDomNamedNodeMap attr = ele.attributes();
	      int nattr = attr.count();
	      bool isnode = false;
	      bool isedge = false;
	      for(int na=0; na<nattr; na++)
		{
		  QDomNode node = attr.item(na);
		  QString name = node.nodeName();
		  QString val = node.nodeValue();
		  if (name == "for" && val == "node") { isnode = true; break; }
		  if (name == "for" && val == "edge") { isedge = true; break; }
		}
	      if (isedge)
		{
		  bool notstr = true;
		  for(int na=0; na<nattr; na++)
		    {
		      QDomNode node = attr.item(na);
		      QString name = node.nodeName();
		      QString val = node.nodeValue();

		      if (name == "attr.type" &&
			  val == "string")
			{
			  notstr = false;
			  break;
			}
		    }
		  if (notstr)
		    {
		      for(int na=0; na<nattr; na++)
			{
			  QDomNode node = attr.item(na);
			  QString name = node.nodeName();
			  QString val = node.nodeValue();
			  
			  if (name == "id")
			    m_edgeAtt << val;
			  if (name == "attr.name")
			    m_edgeAttName << val;
			}
		    }
		}

	      if (isnode)
		{
		  bool posatt = false;
		  for(int na=0; na<nattr; na++)
		    {
		      QDomNode node = attr.item(na);
		      QString name = node.nodeName();
		      QString val = node.nodeValue();

		      if (name == "attr.name" &&
			  (val == "Position X" ||
			   val == "Position Y" ||
			   val == "Position Z"))
			{
			  posatt = true;
			  for(int a=0; a<nattr; a++)
			    {
			      QDomNode nde = attr.item(a);
			      QString nme = nde.nodeName();
			      QString vle = nde.nodeValue();
			      if (nme == "id")
				{
				  int pa = 0;
				  if (val == "Position X") pa = 0;
				  if (val == "Position Y") pa = 1;
				  if (val == "Position Z") pa = 2;
				  m_nodePosAttr[pa] = vle;
				}
			    }
			  break;
			}
		    }		    
		  if (!posatt) // not position attribute
		    {
		      for(int na=0; na<nattr; na++)
			{
			  QDomNode node = attr.item(na);
			  QString name = node.nodeName();
			  QString val = node.nodeValue();
			  
			  //if (val == "attr.name")
			  if (name == "id")
			    m_nodeAtt << val;
			  if (name == "attr.name")
			    m_nodeAttName << val;
			}
		    }
		}
	    }
	}
    }

  m_vertexRadiusAttribute = -1;
  m_edgeRadiusAttribute = -1;

  for(int i=0; i<m_nodeAtt.count(); i++)
    {
      if (m_nodeAtt[i].contains("radius", Qt::CaseInsensitive) ||
	  m_nodeAtt[i].contains("diameter", Qt::CaseInsensitive))
	{
	  m_vertexRadiusAttribute = i;
	  break;
	}
    }
  for(int i=0; i<m_edgeAtt.count(); i++)
    {
      if (m_edgeAtt[i].contains("radius", Qt::CaseInsensitive) ||
	  m_edgeAtt[i].contains("diameter", Qt::CaseInsensitive))
	{
	  m_edgeRadiusAttribute = i;
	  break;
	}
    }
}

void
NetworkObject::loadNodeInfo(QDomNodeList nnodes)
{
  m_vertexAttribute.clear();
  m_vertexCenters.clear();
  m_nodeId.clear();

  QVector<float> *vat; // not expecting more than 20 attributes
  int nvat = m_nodeAtt.count();
  vat = new QVector<float> [nvat];

  int nn = nnodes.count();
  for(int n=0; n<nn; n++)
    {
      QDomElement ele = nnodes.item(n).toElement();
      QString id = ele.attributeNode("id").value();
      m_nodeId << id;

      Vec pos;

      QDomNodeList nlist = nnodes.item(n).childNodes();
      int nc = nlist.count();
      for(int c=0; c<nc; c++)
	{
	  QDomNode node = nlist.item(c);
	  QDomElement ele = node.toElement();
	  QString et = ele.text();

	  QDomNamedNodeMap attr = ele.attributes();
	  QString name = attr.item(0).nodeName();
	  QString val = attr.item(0).nodeValue();
	  if (val == m_nodePosAttr[0]) pos.x = et.toFloat();
	  else if (val == m_nodePosAttr[1]) pos.y = et.toFloat();
	  else if (val == m_nodePosAttr[2]) pos.z = et.toFloat();
	  else 
	    {
	      int vi = m_nodeAtt.indexOf(val);
	      if (vi >= 0)
		vat[vi] << et.toFloat();
	    }
	}
      m_vertexCenters << pos;

      int nv = m_vertexCenters.count();
      for(int c=0; c<m_nodeAtt.count(); c++)
	{
	  if (vat[c].count() < nv)
	    vat[c] << 0.0;
	}
    }

  for(int e=0; e<m_nodeAtt.count(); e++)
    {
      if (m_nodeAtt[e].contains("diameter", Qt::CaseInsensitive))
	{
	  for(int vi=0; vi<vat[e].count(); vi++)
	    vat[e][vi] /= 2;
	}
    }


  for(int vi=0; vi<m_nodeAtt.count(); vi++)
    m_vertexAttribute << qMakePair(m_nodeAttName[vi], vat[vi]);      

  if (m_vertexRadiusAttribute == -1)
    {
      m_vertexRadiusAttribute = m_nodeAtt.count();
      QVector<float> rad;
      rad.resize(m_vertexCenters.count());
      rad.fill(2);
      m_nodeAtt << "vertex_radius";
      m_vertexAttribute << qMakePair(QString("vertex_radius"), rad);
    }

  for(int i=0; i<nvat; i++)
    vat[i].clear();
  delete [] vat;
}

void
NetworkObject::loadEdgeInfo(QDomNodeList nnodes)
{
  m_edgeAttribute.clear();
  m_edgeNeighbours.clear();

  QVector<float> *vat; // not expecting more than 20 attributes
  int nvat = m_edgeAtt.count();
  vat = new QVector<float> [nvat];

  int nn = nnodes.count();
  for(int n=0; n<nn; n++)
    {
      QDomElement ele = nnodes.item(n).toElement();
      QString id = ele.attributeNode("id").value();
      QString src = ele.attributeNode("source").value();
      QString tar = ele.attributeNode("target").value();
      int a,b;
      a = m_nodeId.indexOf(src);
      b = m_nodeId.indexOf(tar);
      if (a >= 0 && b >= 0)
	m_edgeNeighbours << qMakePair(a,b);

      QDomNodeList nlist = nnodes.item(n).childNodes();
      int nc = nlist.count();
      for(int c=0; c<nc; c++)
	{
	  QDomNode node = nlist.item(c);
	  QDomElement ele = node.toElement();
	  QString et = ele.text();
	  QDomNamedNodeMap attr = ele.attributes();
	  QString name = attr.item(0).nodeName();
	  QString val = attr.item(0).nodeValue();

	  int vi = m_edgeAtt.indexOf(val);
	  if (vi >= 0)
	    vat[vi] << et.toFloat();
	}

      int nv = m_edgeNeighbours.count();
      for(int c=0; c<m_edgeAtt.count(); c++)
	{
	  if (vat[c].count() < nv)
	    vat[c] << 0.0;
	}
    }

  for(int e=0; e<m_edgeAtt.count(); e++)
    {
      if (m_edgeAtt[e].contains("diameter", Qt::CaseInsensitive))
	{
	  for(int vi=0; vi<vat[e].count(); vi++)
	    vat[e][vi] /= 2;
	}
    }

  for(int vi=0; vi<m_edgeAtt.count(); vi++)
    m_edgeAttribute << qMakePair(m_edgeAttName[vi], vat[vi]);      

  if (m_edgeRadiusAttribute == -1)
    {
      m_edgeRadiusAttribute = m_edgeAtt.count();
      QVector<float> rad;
      rad.resize(m_edgeNeighbours.count());
      rad.fill(2);
      m_edgeAtt << "edge_radius";
      m_edgeAttribute << qMakePair(QString("edge_radius"), rad);
    }

  for(int i=0; i<nvat; i++)
    vat[i].clear();
  delete [] vat;
}

bool
NetworkObject::loadGraphML(QString flnm)
{
  m_fileName = flnm;

  QDomDocument doc;
  QFile f(flnm.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      doc.setContent(&f);
      f.close();
    }

  QDomElement main = doc.documentElement();
  getKey(main);

//  m_log->insertPlainText("Node Attributes \n");
//  for(int i=0; i<m_nodeAtt.count(); i++)
//    m_log->insertPlainText("     "+m_nodeAtt[i]+"\n");
//  m_log->insertPlainText("\n");
//  m_log->insertPlainText("Edge Attributes \n");
//  for(int i=0; i<m_edgeAtt.count(); i++)
//    m_log->insertPlainText("     "+m_edgeAtt[i]+"\n");
//  m_log->insertPlainText("\n");
 
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).isElement())
	{
	  QDomElement ele = dlist.at(i).toElement();
	  QString str = ele.nodeName();
	  if (str == "graph")
	    {
	      QDomNodeList nnodes = ele.elementsByTagName("node");
	      loadNodeInfo(nnodes);

	      QDomNodeList nedges = ele.elementsByTagName("edge");
	      loadEdgeInfo(nedges);
	    }
	}
    }

  //---------------------
  Vec bmin = m_vertexCenters[0];
  Vec bmax = m_vertexCenters[0];
  for(int i=0; i<m_vertexCenters.count(); i++)
    {
      bmin = StaticFunctions::minVec(bmin, m_vertexCenters[i]);
      bmax = StaticFunctions::maxVec(bmax, m_vertexCenters[i]);
    }
  m_centroid = (bmin + bmax)/2;
  
  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);


//  m_nX = (bmax.x - bmin.x) + 1;
//  m_nY = (bmax.y - bmin.y) + 1;
//  m_nZ = (bmax.z - bmin.z) + 1;
  m_nX = bmax.x;
  m_nY = bmax.y;
  m_nZ = bmax.z;

  if (!Global::batchMode())
    {
      QString str;
      str = QString("Grid Size : %1 %2 %3\n").arg(m_nX).arg(m_nY).arg(m_nZ);
      str += QString("Vertices : %1\n").arg(m_vertexCenters.count());
      str += QString("Edges : %1\n").arg(m_edgeNeighbours.count());
      str += QString("\n");
      str += QString("Vertex Attributes : %1\n").arg(m_vertexAttribute.count());
      for(int i=0; i<m_vertexAttribute.count(); i++)
	str += QString(" %1\n").arg(m_vertexAttribute[i].first);
      str += QString("\n");
      str += QString("Edge Attributes : %1\n").arg(m_edgeAttribute.count());
      for(int i=0; i<m_edgeAttribute.count(); i++)
	str += QString(" %1\n").arg(m_edgeAttribute[i].first);
      
      QMessageBox::information(0, "Network loaded", str);
    }

  m_Vatt = 0;
  m_Eatt = 0;
  m_Vminmax.clear();
  m_Eminmax.clear();
  m_userVminmax.clear();
  m_userEminmax.clear();

  for(int i=0; i<m_vertexAttribute.count(); i++)
    {
      float vmin = m_vertexAttribute[i].second[0];
      float vmax = m_vertexAttribute[i].second[0];
      for(int j=1; j<m_vertexAttribute[i].second.count(); j++)
	{
	  vmin = qMin((float)m_vertexAttribute[i].second[j], vmin);
	  vmax = qMax((float)m_vertexAttribute[i].second[j], vmax);
	}
      m_Vminmax.append(qMakePair(vmin, vmax));
      m_userVminmax.append(qMakePair((vmin+vmax)/2, vmax));
    }

  for(int i=0; i<m_edgeAttribute.count(); i++)
    {
      float emin = m_edgeAttribute[i].second[0];
      float emax = m_edgeAttribute[i].second[0];
      for(int j=1; j<m_edgeAttribute[i].second.count(); j++)
	{
	  emin = qMin((float)m_edgeAttribute[i].second[j], emin);
	  emax = qMax((float)m_edgeAttribute[i].second[j], emax);
	}
      m_Eminmax.append(qMakePair(emin, emax));
      m_userEminmax.append(qMakePair((emin+emax)/2, emax));
    }

//  QString str;
//  str += "Vertex\n";
//  for(int i=0; i<m_vertexAttribute.count(); i++)
//    {
//      str += m_vertexAttribute[i].first + QString("  %1 %2\n").\
//	arg(m_Vminmax[i].first).
//	arg(m_Vminmax[i].second);
//    }
//  str += "\n\nEdges\n";
//  for(int i=0; i<m_edgeAttribute.count(); i++)
//    {
//      str += m_edgeAttribute[i].first + QString("  %1 %2\n").\
//	arg(m_Eminmax[i].first).
//	arg(m_Eminmax[i].second);
//    }
//  QMessageBox::information(0, "", str);

  return true;
}

