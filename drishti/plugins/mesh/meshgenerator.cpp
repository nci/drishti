#include "staticfunctions.h"
#include "meshgenerator.h"

MeshGenerator::MeshGenerator() {}
MeshGenerator::~MeshGenerator() {}

QGradientStops
MeshGenerator::resampleGradientStops(QGradientStops stops)
{
  QColor colorMap[256];

  int startj, endj;
  for(int i=0; i<stops.size(); i++)
    {
      float pos = stops[i].first;
      QColor color = stops[i].second;
      endj = pos*255;
      colorMap[endj] = color;
      if (i > 0)
	{
	  QColor colStart, colEnd;
	  colStart = colorMap[startj];
	  colEnd = colorMap[endj];
	  float rb,gb,bb,ab, re,ge,be,ae;
	  rb = colStart.red();
	  gb = colStart.green();
	  bb = colStart.blue();
	  ab = colStart.alpha();
	  re = colEnd.red();
	  ge = colEnd.green();
	  be = colEnd.blue();
	  ae = colEnd.alpha();
	  for (int j=startj+1; j<endj; j++)
	    {
	      float frc = (float)(j-startj)/(float)(endj-startj);
	      float r,g,b,a;
	      r = rb + frc*(re-rb);
	      g = gb + frc*(ge-gb);
	      b = bb + frc*(be-bb);
	      a = ab + frc*(ae-ab);
	      colorMap[j] = QColor(r, g, b, a);
	    }
	}
      startj = endj;
    }

  QGradientStops newStops;
  for (int i=0; i<256; i++)
    {
      float pos = (float)i/255.0f;
      newStops << QGradientStop(pos, colorMap[i]);
    }

  return newStops;
}

bool
MeshGenerator::getValues(int &isoval, float &isovalf,
			 int &spread,
			 int &depth,
			 int &useColor,
			 int &fillValue,
			 bool &checkForMore,
			 bool &lookInside,
			 bool &useOpacity,
			 bool &smoothOpacity,
			 QGradientStops &stops,
			 bool doBorder,
			 int &chan,
			 bool &avgColor)
{
  chan = 0;
  isoval = 128;
  isovalf = 0.5;
  spread = 0;
  depth = 1;
  useColor = 5;
  fillValue = -1;
  checkForMore = true;
  lookInside = false;
  useOpacity = true;
  smoothOpacity = true;
  avgColor = true;
  m_useTagColors = false;
  m_scaleModel = 1.0;
  QGradientStops vstops;
  vstops << QGradientStop(0.0, QColor(50 ,50 ,50 ,255))
	 << QGradientStop(0.5, QColor(200,150,100,255))
	 << QGradientStop(1.0, QColor(255,255,255,255));

  if (doBorder) fillValue = 0;
  
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  if (m_voxelType == 0)
    {
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(useOpacity);
      plist["use opacity"] = vlist;

      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(smoothOpacity);
      plist["smooth opacity"] = vlist;

      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(avgColor);
      plist["average color"] = vlist;
    }

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_useTagColors);
  plist["apply tag colors"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(chan);
  vlist << QVariant(0);
  vlist << QVariant(2);
  plist["mop channel"] = vlist;

  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(isovalf);
  vlist << QVariant(0.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.004); // singlestep
  vlist << QVariant(3); // decimals
  plist["isosurface value"] = vlist;

//  vlist.clear();
//  vlist << QVariant("int");
//  vlist << QVariant(spread);
//  vlist << QVariant(0);
//  vlist << QVariant(50);
//  plist["spread"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(depth);
  vlist << QVariant(0);
  vlist << QVariant(200);
  plist["depth"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(fillValue);
  vlist << QVariant(-1);
  vlist << QVariant(255);
  plist["fillvalue"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(checkForMore);
  plist["greater"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(lookInside);
  plist["look inside"] = vlist;

  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(m_scaleModel);
  vlist << QVariant(0.001);
  vlist << QVariant(1.0);
  vlist << QVariant(0.005); // singlestep
  vlist << QVariant(3); // decimals
  plist["scale"] = vlist;

  vlist.clear();
  vlist << QVariant("colorgradient");
  for(int s=0; s<vstops.size(); s++)
    {
      float pos = vstops[s].first;
      QColor color = vstops[s].second;
      int r = color.red();
      int g = color.green();
      int b = color.blue();
      int a = color.alpha();
      vlist << QVariant(pos);
      vlist << QVariant(r);
      vlist << QVariant(g);
      vlist << QVariant(b);
      vlist << QVariant(a);
    }
  plist["color gradient"] = vlist;

  vlist.clear();
  vlist << QVariant("combobox");
  vlist << "5";
  vlist << "Fixed Color";
  vlist << "Normal Color";
  vlist << "Position Color";
  vlist << "Occlusion Color";
  vlist << "Lut Color";
  vlist << "VR Lut Color";
  plist["color type"] = vlist;
  QStringList colortypes;
  colortypes << "Fixed Color";
  colortypes << "Normal Color";
  colortypes << "Position Color";
  colortypes << "Occlusion Color";
  colortypes << "Lut Color";
  colortypes << "VR Lut Color";


  vlist.clear();
  QFile helpFile(":/mesh.help");
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

  vlist.clear();
  QString mesg;
  mesg += "File : "+m_vfm->fileName()+"\n";
  int d = m_vfm->depth();
  int w = m_vfm->width();
  int h = m_vfm->height();
  mesg += QString("Volume Size : %1 %2 %3\n").arg(h).arg(w).arg(d);
  mesg += QString("Data Min : %1 %2 %3\n").arg(m_dataMin.x).arg(m_dataMin.y).arg(m_dataMin.z);
  mesg += QString("Data Max : %1 %2 %3\n").arg(m_dataMax.x).arg(m_dataMax.y).arg(m_dataMax.z);

  if (m_voxelType > 0)
    mesg += "\n ** Only opacity based surface generation available for unsigned short data **\n";

  mesg += "\n* You can keep on working while this process is running.\n";
  vlist << mesg;
  plist["message"] = vlist;


  QStringList keys;
  keys << "use opacity";
  keys << "smooth opacity";
  keys << "average color";
  keys << "apply tag colors";
  keys << "mop channel";
  keys << "isosurface value";
  //keys << "spread"; // turn of spread for the time being
  keys << "depth";
  keys << "fillvalue";
  keys << "scale";
  keys << "greater";
  keys << "look inside";
  keys << "color gradient";
  keys << "color type";
  keys << "commandhelp";
  keys << "message";

  propertyEditor.set("Mesh Generation Parameters", plist, keys);
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return false;
  
  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);

      if (pair.second)
	{
	  if (keys[ik] == "use opacity")
	    useOpacity = pair.first.toBool();
	  else if (keys[ik] == "smooth opacity")
	    smoothOpacity = pair.first.toBool();
	  else if (keys[ik] == "average color")
	    avgColor = pair.first.toBool();
	  else if (keys[ik] == "apply tag colors")
	    m_useTagColors = pair.first.toBool();
	  else if (keys[ik] == "mop channel")
	    chan = pair.first.toInt();
	  else if (keys[ik] == "color gradient")
	    vstops = propertyEditor.getGradientStops(keys[ik]);
	  else if (keys[ik] == "isosurface value")
	    isovalf = pair.first.toFloat();
	  else if (keys[ik] == "scale")
	    m_scaleModel = pair.first.toFloat();
	  else if (keys[ik] == "spread")
	    spread = pair.first.toInt();
	  else if (keys[ik] == "depth")
	    depth = pair.first.toInt();
	  else if (keys[ik] == "fillvalue")
	    fillValue = pair.first.toInt();
	  else if (keys[ik] == "greater")
	    checkForMore = pair.first.toBool();
	  else if (keys[ik] == "look inside")
	    lookInside = pair.first.toBool();
	  else if (keys[ik] == "color type")
	    {
	      useColor = pair.first.toInt();
	      useColor = qBound(0, useColor, 4);
	    }
	}
    }

  isoval = qRound(isovalf*255);
  if (m_voxelType > 0 && !useOpacity)
    isoval = qRound(isovalf*65535);

    
  stops = resampleGradientStops(vstops);
  if (useColor < _OcclusionColor)
    spread = depth = 0;
  else if (useColor == _OcclusionColor)
    {
      if (spread == 0)
	{
	  QMessageBox::information(0, "", "For Occlusion Color spread should be greater than 0");
	  return false;
	}
    }

  return true;
}


QString
MeshGenerator::start(VolumeFileManager *vfm,
		     int nX, int nY, int nZ,
		     Vec dataMin, Vec dataMax,
		     QString prevDir,
		     Vec voxelScaling, int samplingLevel,
		     QList<Vec> clipPos,
		     QList<Vec> clipNormal,
		     QList<CropObject> crops,
		     QList<PathObject> paths,
		     uchar *lut,
		     int pruneLod, int pruneX, int pruneY, int pruneZ,
		     QVector<uchar> pruneData,
		     QVector<uchar> tagColors)
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
  m_tagColors = tagColors;
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

  bool doBorder = false;
  if (qRound(m_dataSize.x) < m_height ||
      qRound(m_dataSize.y) < m_width ||
      qRound(m_dataSize.z) < m_depth)
    doBorder = true;
  if (clipPos.count() > 0 || m_crops.count() > 0 || m_paths.count() > 0)
    doBorder = true;

  float isovalf;
  int isoval, spread, depth, useColor, fillValue;
  bool checkForMore;
  bool lookInside;
  bool useOpacity;
  bool smoothOpacity;
  QGradientStops stops;
  int chan;
  bool avgColor;
  if (! getValues(isoval, isovalf,
		  spread, depth, useColor, fillValue,
		  checkForMore,
		  lookInside,
		  useOpacity,
		  smoothOpacity,
		  stops,
		  doBorder,
		  chan,
		  avgColor))
    return "";


  m_meshLog = new QTextEdit;
  m_meshProgress = new QProgressBar;

  QVBoxLayout *meshLayout = new QVBoxLayout;
  meshLayout->addWidget(m_meshLog);
  meshLayout->addWidget(m_meshProgress);

  QWidget *meshWindow = new QWidget;
  if (useOpacity)
    meshWindow->setWindowTitle("Drishti - Mesh Generation Using Opacity Values");
  else
    meshWindow->setWindowTitle("Drishti - Mesh Generation Using Voxel Values");
  meshWindow->setLayout(meshLayout);
  meshWindow->show();
  meshWindow->resize(700, 300);


  float memGb = 0.5;
  memGb = QInputDialog::getDouble(0,
				  "Use memory",
				  "Max memory we can use (GB)", memGb, 0.1, 1000, 2);

  qint64 gb = 1024*1024*1024;
  qint64 memsize = memGb*gb; // max memory we can use (in GB)

  qint64 canhandle = memsize/15;
  qint64 gsize = qPow((double)canhandle, 0.333);

  m_meshLog->insertPlainText(QString("Can handle data with total grid size of %1 : typically %2^3\nOtherwise slabs method will be used.  Mesh is generated for each slab and then joined together.\n\n"). \
			   arg(canhandle).arg(gsize));


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
  m_meshLog->insertPlainText("\n\n");


  m_meshLog->insertPlainText(QString("Isosurface Value : %1 (%2)\nGrid size : %3 %4 %5\n").\
			   arg(isovalf).arg(isoval).\
			   arg(m_nX).arg(m_nY).arg(m_nZ));
  

  //---- export the grid ---
  QString flnm = QFileDialog::getSaveFileName(0,
					      "Export mesh to file",
					      prevDir,
					      "*.ply");
  if (flnm.size() == 0)
    {
      meshWindow->close();
      return "";
    }
  //----------------------------

  int nSlabs = 1;
  qint64 reqmem = m_nX;
  reqmem *= m_nY*m_nZ*20;
  nSlabs = qMax(qint64(1), reqmem/memsize + 1);
//  QMessageBox::information(0, "", QString("Number of Slabs : %1 : %2 %3").\
//			   arg(nSlabs).arg(reqmem).arg(memsize));

  generateMesh(nSlabs,
	       isoval,
	       flnm,
	       depth, spread,
	       stops,
	       useColor,
	       fillValue,
	       checkForMore,
	       lookInside,
	       voxelScaling,
	       clipPos, clipNormal,
	       crops, paths,
	       useOpacity, smoothOpacity, lut,
	       chan,
	       avgColor);


  meshWindow->close();

  return flnm;
}

float
MeshGenerator::getOcclusionFractionSAT(int *oData,
				       int dlen,
				       int spread, int nextra,
				       QVector3D pos,
				       QVector3D normal,
				       bool lookInside)
{
  if (lookInside)
    normal = -normal;

  float op = 0;
  float tnf = 0;
  for(int a=1; a<spread; a++)
    {
      //float a2 = qMax(1.0f, sqrtf(a));
      //float a2 = a;
      float a2 = qMax(1.0f, a/2.0f);
      QVector3D r = pos + a*normal;
      int xmin = r.x() - a2;
      int xmax = r.x() + a2;
      int ymin = r.y() - a2;
      int ymax = r.y() + a2;
      int zmin = r.z() - a2;
      int zmax = r.z() + a2;
      int nf = (xmax-xmin+1)*(ymax-ymin+1)*(zmax-zmin+1);
      xmin = qMax(0, xmin);
      ymin = qMax(0, ymin);
      zmin = qMax(0, zmin);
      xmax = qMin(m_nZ-1, xmax);
      ymax = qMin(m_nY-1, ymax);
      zmax = qMin(dlen+2*nextra-1, zmax);
      
      float frc = (spread-a);
      op += (frc*(oData[zmax*m_nY*m_nZ + ymax*m_nZ + xmax] +
		  oData[zmax*m_nY*m_nZ + ymin*m_nZ + xmin] +
		  oData[zmin*m_nY*m_nZ + ymax*m_nZ + xmin] +
		  oData[zmin*m_nY*m_nZ + ymin*m_nZ + xmax] -
		  oData[zmin*m_nY*m_nZ + ymin*m_nZ + xmin] -
		  oData[zmin*m_nY*m_nZ + ymax*m_nZ + xmax] -
		  oData[zmax*m_nY*m_nZ + ymax*m_nZ + xmin] -
		  oData[zmax*m_nY*m_nZ + ymin*m_nZ + xmax]))/nf;
      tnf += frc;
    }
  op /= tnf;
  op = qBound(0.0f, op, 1.0f);
  return op;
}

float
MeshGenerator::getOcclusionFraction(uchar *oData,
				    int dlen,
				    int spread, int nextra,
				    uchar isoval,
				    QVector3D pos,
				    QVector3D normal,
				    bool checkForMore)
{
  QVector3D pvec, qvec;
  pvec = QVector3D(63.97, 13.47, 79.33);
  pvec.normalize();
  qvec = QVector3D::normal(pvec, normal);
  pvec = QVector3D::normal(normal, qvec);
  
  float frc = 0;
  float nf = 0;
  for(int a=1; a<=spread; a++)
    {
      int ae = qRound(sqrtf(spread*spread-a*a));
      int stp = 1+a/2;
      for(int b=-ae; b<=ae; b+=stp)
	for(int c=-ae; c<=ae; c+=stp)
	  {
	    nf ++;
	    QVector3D r = pos + a*normal + b*pvec + c*qvec;
	    int i = r.x();
	    int j = r.y();
	    int k = r.z();
	    if (i > m_nZ-1 || j > m_nY-1 || k > m_nX-1 ||
		i < 0 || j < 0 || k < 0) // outside so not occluded
	      frc++;
	    else
	      {
		i = qBound(0, i, m_nZ-1);
		j = qBound(0, j, m_nY-1);
		k = qBound(0, k, dlen+2*nextra-1);
		uchar e = oData[k*m_nY*m_nZ + j*m_nZ + i];
		if (checkForMore)
		  {
		    if (isoval >= e)
		      frc++;
		  }
		else
		  {
		    if (isoval < e)
		      frc++;
		  }
	      }
	  }
      QVector3D tv;
      tv = pvec;
      pvec += qvec;
      qvec -= tv;
      pvec.normalize();
      qvec.normalize();
    }

  frc /= nf;
  frc = qBound(0.0f, frc, 1.0f);

  return frc;
}

QColor
MeshGenerator::getOcclusionColor(int *oData,
				 int dlen,
				 int spread, int nextra,
				 uchar isoval,
				 QVector3D pos,
				 QVector3D normal,
				 QGradientStops vstops,
				 bool lookInside)
{
  float frc = getOcclusionFractionSAT(oData,
				      dlen,
				      spread, nextra,
				      pos, normal,
				      lookInside);
  if (frc > 0.0)
    frc = sqrt(frc); // shift towards brightness
      
  int stopsCount = vstops.count()-1;
  QColor col = vstops[frc*stopsCount].second;

  return col;
}

QColor
MeshGenerator::getLutColor(uchar *volData,	  
			   int *oData,
			   int dlen,
			   int depth, int spread, int nextra,
			   uchar isoval,
			   QVector3D pos,
			   QVector3D normal,
			   QGradientStops vstops,
			   bool lookInside,
			   bool avgColor)
{
  // go a bit deeper and start
  QVector3D vpos = pos + normal;

  // -- find how far deep we can go
  int nd = 0;
  for(int n=0; n<=depth; n++)
    {
      int i = vpos.x();
      int j = vpos.y();
      int k = vpos.z();
      if (i > m_nZ-1 || j > m_nY-1 || k > dlen+2*nextra-1 ||
	  i < 0 || j < 0 || k < 0) // gone out
	break;
      nd ++;
      vpos += normal;
    }

  // now start collecting the samples
  vpos = pos + normal;

  Vec rgb = Vec(0,0,0);  
  float tota = 0;
  for(int n=0; n<=nd; n++)
    {
      int i = vpos.x();
      int j = vpos.y();
      int k = vpos.z();
      i = qBound(0, i, m_nZ-1);
      j = qBound(0, j, m_nY-1);
      k = qBound(0, k, dlen+2*nextra-1);
      uchar v;
      if (m_voxelType == 0)
	v = volData[k*m_nY*m_nZ + j*m_nZ + i];
      else
	v = ((ushort*)volData)[k*m_nY*m_nZ + j*m_nZ + i] / 256;
      QColor col0 = vstops[v].second;
      float a = col0.alphaF()/255.0f;
      float r = a*col0.red();
      float g = a*col0.green();
      float b = a*col0.blue();

      Vec vcol;

      if (avgColor)
	{
	  rgb += Vec(r,g,b)/255.0f;
	  tota += a;
	}
      else
	{
	  r /= 255.0f;
	  g /= 255.0f;
	  b /= 255.0f;
	  
	  rgb += Vec(r,g,b)*(1-tota);
	  tota += a*(1-tota);
	}
      
      vpos += normal;
    }

  if (tota < 0.01) tota = 1.0;
  rgb /= tota;

  //-------------------
  // apply ambient occlusion
  if (spread > 0)
    {
      float frc = getOcclusionFractionSAT(oData,
					  dlen,
					  spread, nextra,
					  pos, normal,
					  lookInside);
      if (frc > 0.0)
	frc = sqrt(frc); // shift towards brightness
      frc = qMax(0.1f, frc);
      rgb *= frc;
    }
  //-------------------

  rgb *= 255;
  QColor col = QColor(rgb.x, rgb.y, rgb.z);

  return col;
}

QColor
MeshGenerator::getLutColor(uchar *volData,	  
			   int *oData,
			   int dlen,
			   int depth, int spread, int nextra,
			   uchar isoval,
			   QVector3D pos,
			   QVector3D normal,
			   uchar *lut,
			   bool lookInside,
			   QVector3D globalPos,
			   bool avgColor)			   
{
  // go a bit deeper and start
  QVector3D vpos = pos + normal;

  // -- find how far deep we can go
  int nd = 0;
  for(int n=0; n<=depth; n++)
    {
      int i = vpos.x();
      int j = vpos.y();
      int k = vpos.z();
      if (i > m_nZ-1 || j > m_nY-1 || k > dlen+2*nextra-1 ||
	  i < 0 || j < 0 || k < 0) // gone out
	break;
      nd ++;
      vpos += normal;
    }

  // now start collecting the samples
  vpos = pos + normal;
  QVector3D gpos = globalPos + normal;

  Vec rgb = Vec(0,0,0);
  float tota = 0;
  for(int ns=0; ns<=nd; ns++)
    {
      int i = vpos.x();
      int j = vpos.y();
      int k = vpos.z();
      
      i = qBound(0, i, m_nZ-1);
      j = qBound(0, j, m_nY-1);
      k = qBound(0, k, dlen+2*nextra-1);

      Vec po0 = Vec(m_dataMin.x+gpos.x(), m_dataMin.y+gpos.y(), gpos.z());
      Vec po = po0*m_samplingLevel;

      bool ok=true;

      if (ok)
	{	  
	  ushort v, gr;
	  if (m_voxelType == 0)
	    {
	      v = volData[k*m_nY*m_nZ + j*m_nZ + i];
	      gr = 0;
	    }
	  else
	    {
	      v = ((ushort*)volData)[k*m_nY*m_nZ + j*m_nZ + i];
	      gr = v%256;
	      v = v/256;
	    }

	  float a = lut[4*(256*gr + v)+3]/255.0f;
	  float r = lut[4*(256*gr + v)+0]*a;
	  float g = lut[4*(256*gr + v)+1]*a;
	  float b = lut[4*(256*gr + v)+2]*a;
      
	  if (m_blendPresent)
	    {
	      for(int ci=0; ci<m_crops.count(); ci++)
		{
		  if (m_crops[ci].cropType() > CropObject::Displace_Displace &&
		      m_crops[ci].cropType() < CropObject::Glow_Ball)
		    {
		      float viewMix = m_crops[ci].checkBlend(po);
		      
		      int tfSet = m_crops[ci].tfset();
		      tfSet *= 256*256*4;
		      float a1 = lut[tfSet+4*(256*gr + v)+3]/255.0f;
		      float r1 = lut[tfSet+4*(256*gr + v)+0]*a1;
		      float g1 = lut[tfSet+4*(256*gr + v)+1]*a1;
		      float b1 = lut[tfSet+4*(256*gr + v)+2]*a1;

		      r = (1-viewMix)*r + viewMix*r1;
		      g = (1-viewMix)*g + viewMix*g1;
		      b = (1-viewMix)*b + viewMix*b1;
		      a = (1-viewMix)*a + viewMix*a1;
		    }
		}
	    }
	  if (m_pathBlendPresent)
	    {
	      for(int ci=0; ci<m_paths.count(); ci++)
		{
		  if (m_paths[ci].blend())
		    {
		      float viewMix = m_paths[ci].checkBlend(po);
		      
		      int tfSet = m_paths[ci].blendTF();
		      tfSet *= 256*256*4;
		      float a1 = lut[tfSet+4*(256*gr + v)+3]/255.0f;
		      float r1 = lut[tfSet+4*(256*gr + v)+0]*a1;
		      float g1 = lut[tfSet+4*(256*gr + v)+1]*a1;
		      float b1 = lut[tfSet+4*(256*gr + v)+2]*a1;

		      r = (1-viewMix)*r + viewMix*r1;
		      g = (1-viewMix)*g + viewMix*g1;
		      b = (1-viewMix)*b + viewMix*b1;
		      a = (1-viewMix)*a + viewMix*a1;
		    }
		}
	    }

	  // apply tag colors
	  if (m_useTagColors)
	    {
	      Vec pp = po0 - m_dataMin;
	      int ppi = pp.x/m_pruneLod;
	      int ppj = pp.y/m_pruneLod;
	      int ppk = pp.z/m_pruneLod;
	      ppi = qBound(0, ppi, m_pruneX-1);
	      ppj = qBound(0, ppj, m_pruneY-1);
	      ppk = qBound(0, ppk, m_pruneZ-1);
	      int mopidx = ppk*m_pruneY*m_pruneX + ppj*m_pruneX + ppi;
	      int tag = m_pruneData[3*mopidx + 2]; // channel 2 has tag information
	      if (tag > 0)
		{
		  Vec tc = Vec(m_tagColors[4*tag+0],
			       m_tagColors[4*tag+1],
			       m_tagColors[4*tag+2]);
		  float tagOp = m_tagColors[4*tag+3]/255.0f;
		  tc *= tagOp;

		  r = (1-tagOp)*r + tagOp*tc.x;
		  g = (1-tagOp)*g + tagOp*tc.y;
		  b = (1-tagOp)*b + tagOp*tc.z;		  

		  a = qMax(tagOp, a);
		}
	    }
	  
	  Vec vcol;

	  if (avgColor)
	    {
	      rgb += Vec(r,g,b)/255.0f;
	      tota += a;
	    }
	  else
	    {
	      r /= 255.0f;
	      g /= 255.0f;
	      b /= 255.0f;

	      rgb += Vec(r,g,b)*(1-tota);
	      tota += a*(1-tota);
	    }

	}

      vpos += normal;
      gpos += normal;
    }

  if (tota < 0.01) tota = 1.0;
  rgb /= tota;    

  //-------------------
  // apply ambient occlusion
  if (spread > 0)
    {
      float frc = getOcclusionFractionSAT(oData,
					  dlen,
					  spread, nextra,
					  pos, normal,
					  lookInside);
      if (frc > 0.0)
	frc = sqrt(frc); // shift towards brightness
      frc = qMax(0.1f, frc);
      rgb *= frc;
    }
  //-------------------

  rgb *= 255;
  QColor col = QColor(rgb.x, rgb.y, rgb.z);

  return col;
}

void
MeshGenerator::smoothData(uchar *gData,
			  int dlen, int nY, int nZ,
			  int spread)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Smoothing data for color generation ...\n");

  uchar *tmp = new uchar[qMax(dlen, qMax(nY, nZ))];

  for(int k=0; k<dlen; k++)
    {
      m_meshProgress->setValue((int)(100.0*(float)k/(float)(dlen)));
      qApp->processEvents();

      for(int j=0; j<nY; j++)
	{
	  memset(tmp, 0, nZ);
	  for(int i=0; i<nZ; i++)
	    {
	      float v = 0.0f;
	      int nt = 0;
	      for(int i0=qMax(0,i-spread); i0<=qMin(nZ-1,i+spread); i0++)
		{
		  nt++;
		  v += gData[k*nY*nZ + j*nZ + i0];
		}
	      tmp[i] = v/nt;
	    }
	  
	  for(int i=0; i<nZ; i++)
	    gData[k*nY*nZ + j*nZ + i] = tmp[i];
	}
    }


  for(int k=0; k<dlen; k++)
    {
      m_meshProgress->setValue((int)(100.0*(float)k/(float)(dlen)));
      qApp->processEvents();

      for(int i=0; i<nZ; i++)
	{
	  memset(tmp, 0, nY);
	  for(int j=0; j<nY; j++)
	    {
	      float v = 0.0f;
	      int nt = 0;
	      for(int j0=qMax(0,j-spread); j0<=qMin(nY-1,j+spread); j0++)
		{
		  nt++;
		  v += gData[k*nY*nZ + j0*nZ + i];
		}
	      tmp[j] = v/nt;
	    }	  
	  for(int j=0; j<nY; j++)
	    gData[k*nY*nZ + j*nZ + i] = tmp[j];
	}
    }
  
  for(int j=0; j<nY; j++)
    {
      m_meshProgress->setValue((int)(100.0*(float)j/(float)(nY)));
      qApp->processEvents();

      for(int i=0; i<nZ; i++)
	{
	  memset(tmp, 0, dlen);
	  for(int k=0; k<dlen; k++)
	    {
	      float v = 0.0f;
	      int nt = 0;
	      for(int k0=qMax(0,k-spread); k0<=qMin(dlen-1,k+spread); k0++)
		{
		  nt++;
		  v += gData[k0*nY*nZ + j*nZ + i];
		}
	      tmp[k] = v/nt;
	    }
	  for(int k=0; k<dlen; k++)
	    gData[k*nY*nZ + j*nZ + i] = tmp[k];
	}
    }
  
  delete [] tmp;

  m_meshProgress->setValue(100);
}

void
MeshGenerator::genSAT(int *oData,
		      int dlen, int nY, int nZ,
		      int spread)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Generating summed area table for ambient occlusion ...\n");

  for(int k=0; k<dlen; k++)
    {
      m_meshProgress->setValue((int)(100.0*(float)k/(float)(dlen)));
      qApp->processEvents();

      for(int j=0; j<nY; j++)
	for(int i=1; i<nZ; i++)
	  oData[k*nY*nZ + j*nZ + i] += oData[k*nY*nZ + j*nZ + (i-1)];
    }


  for(int k=0; k<dlen; k++)
    {
      m_meshProgress->setValue((int)(100.0*(float)k/(float)(dlen)));
      qApp->processEvents();

      for(int i=0; i<nZ; i++)
	for(int j=1; j<nY; j++)
	  oData[k*nY*nZ + j*nZ + i] += oData[k*nY*nZ + (j-1)*nZ + i];
    }
  
  for(int j=0; j<nY; j++)
    {
      m_meshProgress->setValue((int)(100.0*(float)j/(float)(nY)));
      qApp->processEvents();

      for(int i=0; i<nZ; i++)
	for(int k=1; k<dlen; k++)
	  oData[k*nY*nZ + j*nZ + i] += oData[(k-1)*nY*nZ + j*nZ + i];
    }

  m_meshProgress->setValue(100);
}

void
MeshGenerator::applyTear(int d0, int d1, int nextra,
			 uchar *data0, uchar *data1,
			 bool flag)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("apply dissection ...\n");

  int dlen = d1-d0+1+2*nextra;
  for(int i0=d0-nextra; i0<=d1+nextra; i0++)
    {
      int i = i0 - d0;
      m_meshProgress->setValue((int)(100.0*(float)((i0-d0+nextra)/(float)(dlen))));
      qApp->processEvents();

      int iv = qBound(0, i0, m_depth-1);
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
MeshGenerator::applyOpacity(int iv,
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
MeshGenerator::checkBlend(Vec po, ushort v, uchar* lut)
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
MeshGenerator::checkCrop(Vec po)
{
  bool cropped = false;
  for(int ci=0; ci<m_crops.count(); ci++)
    {
      if (m_crops[ci].cropType() < CropObject::Tear_Tear)
	{
	  // take union
	  cropped |= m_crops[ci].checkCropped(po);
	}
    }
  return cropped;
}

bool
MeshGenerator::checkPathBlend(Vec po, ushort v, uchar* lut)
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
MeshGenerator::checkPathCrop(Vec po)
{
  bool cropped = false;
  for(int ci=0; ci<m_paths.count(); ci++)
    {
      if (m_paths[ci].crop())
	{
	  // take union
	  cropped |= m_paths[ci].checkCropped(po);
	}
    }
  return cropped;
}

void
MeshGenerator::generateMesh(int nSlabs,
			    int isoval,
			    QString flnm,
			    int depth, int spread,
			    QGradientStops vstops,
			    int useColor,
			    int fillValue,
			    bool checkForMore,
			    bool lookInside,
			    Vec voxelScaling,
			    QList<Vec> clipPos,
			    QList<Vec> clipNormal,
			    QList<CropObject> crops,
			    QList<PathObject> paths,
			    bool useOpacity, bool smoothOpacity, 
			    uchar *lut,
			    int chan,
			    bool avgColor)
{
  bool saveIntermediate = false;

  int bpv = 1;
  if (m_voxelType > 0) bpv = 2;
  int nbytes = bpv*m_nY*m_nZ;

//  if (nSlabs > 1)
//    {
//      QStringList sl;
//      sl << "No";
//      sl << "Yes";
//      bool ok;
//      QString okstr = QInputDialog::getItem(0, "Save slab files",
//		      "Save slab files in .ply format.\nFiles will not be collated together to create a unified mesh for the whole sample.",
//					    sl, 0, false, &ok);
//      if (ok && okstr == "Yes")
//	saveIntermediate = true;
//    }

  QGradientStops lutstops;
  for(int i=0; i<255; i++)
    {
      QColor col(lut[4*i+0], lut[4*i+1], lut[4*i+2], lut[4*i+3]);
      lutstops << QGradientStop((float)i/(float)255.0f, col);
    }
  
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

  int nextra = qMax(spread, depth);
  if (useOpacity && smoothOpacity)
    nextra = qMax(5, nextra); // using 11x11x11 box kernel
  int blockStep = m_nX/nSlabs;
  int ntriangles = 0;
  int nvertices = 0;
  for (int nb=0; nb<nSlabs; nb++)
    {
      m_meshLog->moveCursor(QTextCursor::End);
      m_meshLog->insertPlainText(QString("Processing slab %1 of %2\n").arg(nb+1).arg(nSlabs));
      int d0 = nb*blockStep;
      int d1 = qMin(m_nX-1, (nb+1)*blockStep);
      int dlen = d1-d0+1;
      
      int d0z = d0 + qRound(m_dataMin.z);
      int d1z = d1 + qRound(m_dataMin.z);

      uchar *extData;
      if (m_voxelType == 0)
	extData = new uchar[(dlen+2*nextra)*m_nY*m_nZ];
      else
	extData = new uchar[2*(dlen+2*nextra)*m_nY*m_nZ]; // ushort

      uchar *gData=0;
      if (useOpacity)
	gData = new uchar[(dlen+2*nextra)*m_nY*m_nZ];

      int *oData=0;
      if (spread > 0 && useColor >= _OcclusionColor)
	oData = new int[(dlen+2*nextra)*m_nY*m_nZ];

      uchar *cropped = new uchar[nbytes];
      uchar *tmp = new uchar[nbytes];

      int i0 = 0;
      for(int i=d0z-nextra; i<=d1z+nextra; i++)
	{
	  m_meshProgress->setValue((int)(100.0*(float)(i0/(float)(dlen+2*nextra))));
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

	  // tmp now clipped and contains raw data
	  memcpy(extData + bpv*i0*m_nY*m_nZ, tmp, nbytes);

	  if (useOpacity)
	    {
	      applyOpacity(iv, cropped, lut, tmp);
	      memcpy(gData + i0*m_nY*m_nZ, tmp, m_nY*m_nZ);
	    }

	  i0++;
	}
      delete [] tmp;
      delete [] cropped;
      m_meshProgress->setValue(100);
      qApp->processEvents();

      //------------
      if (m_tearPresent)
	{
	  uchar *data0 = new uchar[(dlen+2*nextra)*m_nY*m_nZ];

	  uchar *data1 = extData;
	  memcpy(data0, data1, (dlen+2*nextra)*m_nY*m_nZ);
	  applyTear(d0, d1, nextra,
		    data0, data1, true);

	  if (useOpacity)
	    {
	      data1 = gData;
	      memcpy(data0, data1, (dlen+2*nextra)*m_nY*m_nZ);
	      applyTear(d0, d1, nextra,
			data0, data1, false);
	    }

	  delete [] data0;
	}
      //------------

      if (useOpacity && smoothOpacity)
	smoothData(gData,
		   dlen+2*nextra, m_nY, m_nZ,
		   qMin(2, nextra));


      //--------------------------------
      // ---- set border voxels to fillValue
      if (fillValue >= 0)
	{
	  uchar *v = extData;
	  if (useOpacity) v = gData;

	  i0 = 0;
	  for(int i=d0z-nextra; i<=d1z+nextra; i++)
	    {
	      int iv = qBound(0, i, m_depth-1);
	      int i0dx = i0*m_nY*m_nZ;
	      if (iv <= qRound(m_dataMin.z) || iv >= qRound(m_dataMax.z))
		{
		  if (useOpacity || m_voxelType == 0)
		    memset(v + i0dx, fillValue, m_nY*m_nZ);
		  else
		    {
		      for(int fi=0; fi<m_nY*m_nZ; fi++)
			((ushort*)v)[i0*m_nY*m_nZ + fi] = fillValue;
		    }
		}
	      else
		{
		  if (useOpacity || m_voxelType == 0)
		    {
		      for(int j=0; j<m_nY; j++)
			v[i0dx + j*m_nZ] = fillValue;
		      for(int j=0; j<m_nY; j++)
			v[i0dx + j*m_nZ + m_nZ-1] = fillValue;
		      for(int k=0; k<m_nZ; k++)
			v[i0dx + k] = fillValue;
		      for(int k=0; k<m_nZ; k++)
			v[i0dx + (m_nY-1)*m_nZ + k] = fillValue;
		    }
		  else
		    {
		      for(int j=0; j<m_nY; j++)
			((ushort*)v)[i0dx + j*m_nZ] = fillValue;
		      for(int j=0; j<m_nY; j++)
			((ushort*)v)[i0dx + j*m_nZ + m_nZ-1] = fillValue;
		      for(int k=0; k<m_nZ; k++)
			((ushort*)v)[i0dx + k] = fillValue;
		      for(int k=0; k<m_nZ; k++)
			((ushort*)v)[i0dx + (m_nY-1)*m_nZ + k] = fillValue;
		    }
		}
	      i0++;
	    }
	}
      //--------------------------------

      //--------------------------------
      if (oData)
	{
	  memset(oData, 0, 4*(dlen+2*nextra)*m_nY*m_nZ);
	  uchar *vData = extData;
	  if (useOpacity) vData = gData;

	  if (useColor == _OcclusionColor)
	    {
	      for(int j=0; j<(dlen+2*nextra)*m_nY*m_nZ; j++)
		{
		  ushort v;
		  if (useOpacity || m_voxelType == 0)
		    v = vData[j];
		  else
		    v = ((ushort*)vData)[j];

		  if (checkForMore)
		    {
		      if (isoval >= v) oData[j] = 1;
		    }
		  else
		    {
		      if (isoval < v) oData[j] = 1;
		    }
		}
	    }
	  else
	    {
	      for(int j=0; j<(dlen+2*nextra)*m_nY*m_nZ; j++)
		{
		  ushort v;
		  if (useOpacity || m_voxelType == 0)
		    v = vData[j];
		  else
		    v = ((ushort*)vData)[j];

		  if (checkForMore)
		    {
		      if (lut[4*v + 3] > 0) oData[j] = 1;
		    }
		  else
		    {
		      if (lut[4*v + 3] == 0) oData[j] = 1;
		    }
		}
	    }
	    
	  genSAT(oData,
		 dlen+2*nextra, m_nY, m_nZ,
		 qMin(2, nextra));
	}
      //--------------------------------



      MarchingCubes mc ;
      mc.setLogger(m_meshLog, m_meshProgress);
      mc.set_resolution(m_nZ, m_nY, dlen ) ;      
      if (!useOpacity)
	mc.set_ext_data((extData + nextra*nbytes));
      else
	mc.set_ext_data((gData + nextra*m_nY*m_nZ)); 
      mc.init_all() ;
      mc.run(isoval) ;

      // save part .ply file
      if (saveIntermediate)
	{
	  QString plyfl = flnm;
	  plyfl.chop(4);
	  plyfl += QString(".%1.ply").arg(nb);
	  mc.writePLY(plyfl.toAscii().data(), true);
	}
      else
	{
	  {
	    m_meshLog->moveCursor(QTextCursor::End);
	    m_meshLog->insertPlainText("Saving triangle coordinates ...\n");
	    QString mflnm = flnm + QString(".%1.tri").arg(nb);
	    int ntrigs = mc.ntrigs();
	    Triangle *triangles = mc.triangles();
	    QFile fout(mflnm);
	    fout.open(QFile::WriteOnly);
	    fout.write((char*)&ntrigs, 4);
	    for(int ni=0; ni<ntrigs; ni++)
	      {
		int v[3];
		v[0] = triangles[ni].v1 + nvertices;
		v[1] = triangles[ni].v2 + nvertices;
		v[2] = triangles[ni].v3 + nvertices;
		fout.write((char*)v, 12);
	      }
	    fout.close();
	    ntriangles += ntrigs;
	  }
	  
	  {
	    m_meshLog->moveCursor(QTextCursor::End);
	    m_meshLog->insertPlainText("Generating Color ...\n");
	    QString mflnm = flnm + QString(".%1.vert").arg(nb);
	    int nverts = mc.nverts();
	    Vertex *vertices = mc.vertices();
	    QFile fout(mflnm);
	    fout.open(QFile::WriteOnly);
	    fout.write((char*)&nverts, 4);
	    for(int ni=0; ni<nverts; ni++)
	      {
		m_meshProgress->setValue((int)(100.0*(float)ni/(float)nverts));
		qApp->processEvents();

		float v[6];
		v[0] = vertices[ni].x;
		v[1] = vertices[ni].y;
		v[2] = vertices[ni].z + d0;
		v[3] = vertices[ni].nx;
		v[4] = vertices[ni].ny;
		v[5] = vertices[ni].nz;

		// apply voxelscaling
		v[0] *= voxelScaling.x;
		v[1] *= voxelScaling.y;
		v[2] *= voxelScaling.z;
		v[0] *= m_scaleModel;
		v[1] *= m_scaleModel;
		v[2] *= m_scaleModel;
		fout.write((char*)v, 24);

		v[0] = vertices[ni].x;
		v[1] = vertices[ni].y;
		v[2] = vertices[ni].z + d0;

		uchar c[3];
		float r,g,b;
		if (useColor == _FixedColor)
		  {
		    QColor col = vstops[isoval].second;
		    r = col.red()/255.0;
		    g = col.green()/255.0;
		    b = col.blue()/255.0;
		  }
		else if (useColor == _NormalColor)
		  {
		    r = v[3];
		    g = v[4];
		    b = v[5];
		  }
		else if (useColor == _PositionColor)
		  {
		    r = v[0]/m_nZ;
		    g = v[1]/m_nY;
		    b = v[2]/m_nX;
		  }
		else if (useColor >= _OcclusionColor)
		  {
		    uchar *volData = extData;
		    if (useOpacity && useColor < _LutColor)
		      volData = gData;
		    QColor col;
		    QVector3D pos, normal;
		    pos = QVector3D(vertices[ni].x,
				    vertices[ni].y,
				    vertices[ni].z + nextra);
		    normal = QVector3D(vertices[ni].nx,
				       vertices[ni].ny,
				       vertices[ni].nz);
		    if (useColor == _OcclusionColor)
		      col = getOcclusionColor(oData,
					      dlen,
					      spread, nextra,
					      isoval,
					      pos, normal,
					      vstops,
					      lookInside);
		    else if (useColor == _LutColor)
		      col = getLutColor(volData,
					oData,
					dlen,
					depth, spread, nextra,
					isoval,
					pos, normal,
					vstops,
					lookInside,
					avgColor);
		    else if (useColor == _VRLutColor)
		      col = getLutColor(volData,
					oData,
					dlen,
					depth, spread, nextra,
					isoval,
					pos, normal,
					lut,
					lookInside,
					QVector3D(vertices[ni].x,
						  vertices[ni].y,
						  vertices[ni].z+d0),
					avgColor);

		    r = col.red()/255.0;
		    g = col.green()/255.0;
		    b = col.blue()/255.0;
		  }

		c[0] = r*255;
		c[1] = g*255;
		c[2] = b*255;
		fout.write((char*)c, 3);
	      }
	    fout.close();
	    nvertices += nverts;
	    m_meshProgress->setValue(100);
	  }
	}

      mc.clean_all();
      delete [] extData;
      if (gData) delete [] gData;
      if (oData) delete [] oData;
    } // loop over slabs

  // Files are not collated together to create
  // a unified mesh for the whole sample
  if (!saveIntermediate)
    saveMesh(flnm,
	     nSlabs,
	     nvertices, ntriangles,
	     true);

  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Mesh saved in "+flnm);

  QMessageBox::information(0, "", QString("Mesh saved in "+flnm));


}
void
MeshGenerator::saveMesh(QString flnm,
			int nSlabs,
			int nvertices, int ntriangles,
			bool bin)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Saving Mesh");

  typedef struct PlyFace
  {
    unsigned char nverts;    /* number of Vertex indices in list */
    int *verts;              /* Vertex index list */
  } PlyFace;

  typedef struct
  {
    real  x,  y,  z ;  /**< Vertex coordinates */
    real nx, ny, nz ;  /**< Vertex normal */
    uchar r, g, b;
  } myVertex ;


  PlyProperty vert_props[]  = { /* list of property information for a PlyVertex */
    {"x", Float32, Float32,  offsetof( myVertex,x ), 0, 0, 0, 0},
    {"y", Float32, Float32,  offsetof( myVertex,y ), 0, 0, 0, 0},
    {"z", Float32, Float32,  offsetof( myVertex,z ), 0, 0, 0, 0},
    {"nx", Float32, Float32, offsetof( myVertex,nx ), 0, 0, 0, 0},
    {"ny", Float32, Float32, offsetof( myVertex,ny ), 0, 0, 0, 0},
    {"nz", Float32, Float32, offsetof( myVertex,nz ), 0, 0, 0, 0},
    {"red", Uint8, Uint8,    offsetof( myVertex,r ), 0, 0, 0, 0},
    {"green", Uint8, Uint8,  offsetof( myVertex,g ), 0, 0, 0, 0},
    {"blue", Uint8, Uint8,   offsetof( myVertex,b ), 0, 0, 0, 0}
  };

  PlyProperty face_props[]  = { /* list of property information for a PlyFace */
    {"vertex_indices", Int32, Int32, offsetof( PlyFace,verts ),
      1, Uint8, Uint8, offsetof( PlyFace,nverts )},
  };


  PlyFile    *ply;
  FILE       *fp = fopen(flnm.toAscii().data(),
			 bin ? "wb" : "w");

  PlyFace     face ;
  int         verts[3] ;
  char       *elem_names[]  = { "vertex", "face" };
  ply = write_ply (fp,
		   2,
		   elem_names,
		   bin? PLY_BINARY_LE : PLY_ASCII );

  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, "vertex", nvertices );
  describe_property_ply ( ply, &vert_props[0] );
  describe_property_ply ( ply, &vert_props[1] );
  describe_property_ply ( ply, &vert_props[2] );
  describe_property_ply ( ply, &vert_props[3] );
  describe_property_ply ( ply, &vert_props[4] );
  describe_property_ply ( ply, &vert_props[5] );
  describe_property_ply ( ply, &vert_props[6] );
  describe_property_ply ( ply, &vert_props[7] );
  describe_property_ply ( ply, &vert_props[8] );

  /* describe PlyFace properties (just list of PlyVertex indices) */
  describe_element_ply ( ply, "face", ntriangles );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );


  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, "vertex" );
  for (int nb=0; nb<nSlabs; nb++)
    {
      m_meshProgress->setValue((int)(100.0*(float)nb/(float)nSlabs));
      qApp->processEvents();

      int nverts;
      QString mflnm = flnm + QString(".%1.vert").arg(nb);

      QFile fin(mflnm);
      fin.open(QFile::ReadOnly);
      fin.read((char*)&nverts, 4);
      for(int ni=0; ni<nverts; ni++)
	{
	  myVertex vertex;
	  float v[6];
	  fin.read((char*)v, 24);
	  vertex.x = v[0];
	  vertex.y = v[1];
	  vertex.z = v[2];
	  vertex.nx = -v[3];
	  vertex.ny = -v[4];
	  vertex.nz = -v[5];
	  uchar c[3];
 	  fin.read((char*)c, 3);
	  vertex.r = c[0];
	  vertex.g = c[1];
	  vertex.b = c[2];
	  
	  put_element_ply ( ply, ( void * ) &vertex );
	}
      fin.close();
      fin.remove();
    }

  /* set up and write the PlyFace elements */
  put_element_setup_ply ( ply, "face" );
  face.nverts = 3 ;
  face.verts  = verts ;
  for (int nb=0; nb<nSlabs; nb++)
    {
      m_meshProgress->setValue((int)(100.0*(float)nb/(float)nSlabs));
      qApp->processEvents();

      int ntrigs;
      QString mflnm = flnm + QString(".%1.tri").arg(nb);

      QFile fin(mflnm);
      fin.open(QFile::ReadOnly);
      fin.read((char*)&ntrigs, 4);      
      for(int ni=0; ni<ntrigs; ni++)
	{
	  int v[3];
	  fin.read((char*)v, 12);

	  face.verts[0] = v[2];
	  face.verts[1] = v[1];
	  face.verts[2] = v[0];

	  put_element_ply ( ply, ( void * ) &face );
	}
      fin.close();
      fin.remove();
    }

  close_ply ( ply );
  free_ply ( ply );

  //fclose( fp ) ; // done with close_ply(ply);
  m_meshProgress->setValue(100);
}

