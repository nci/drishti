#include "vdbvolume.h"

#include "staticfunctions.h"
#include "meshgenerator.h"


MeshGenerator::MeshGenerator()
{
  QStringList ps;
  ps << "x";
  ps << "y";
  ps << "z";
  ps << "nx";
  ps << "ny";
  ps << "nz";
  ps << "red";
  ps << "green";
  ps << "blue";
  ps << "vertex_indices";
  ps << "vertex";
  ps << "face";

  for(int i=0; i<ps.count(); i++)
    {
      char *s;
      s = new char[ps[i].size()+1];
      strcpy(s, ps[i].toLatin1().data());
      plyStrings << s;
    }
}
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
			 bool &useOpacity,
			 int &smoothOpacity,
			 QGradientStops &stops,
			 bool doBorder,
			 int &chan,
			 bool &avgColor,
			 float &adaptivity)
{  
  chan = 0;
  isoval = 128;
  isovalf = 0.5;
  spread = 0;
  depth = 1;
  useColor = 1;
  fillValue = -1;
  checkForMore = true;
  useOpacity = true;
  smoothOpacity = 1;
  avgColor = true;
  adaptivity = 0;
  m_useTagColors = false;
  m_scaleModel = 1.0;
  QGradientStops vstops;
  vstops << QGradientStop(0.0, Qt::lightGray)
	 << QGradientStop(1.0, Qt::lightGray);

  if (doBorder) fillValue = 0;

  if (m_batchMode)
    return true;
  
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  if (m_voxelType == 0)
    {
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(useOpacity);
      plist["use opacity"] = vlist;

//      vlist.clear();
//      vlist << QVariant("checkbox");
//      vlist << QVariant(smoothOpacity);
//      plist["smooth opacity"] = vlist;
      vlist.clear();
      vlist << QVariant("int");
      vlist << QVariant(smoothOpacity);
      vlist << QVariant(0);
      vlist << QVariant(10);
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

  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(adaptivity);
  vlist << QVariant(0.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.01); // singlestep
  vlist << QVariant(3); // decimals
  plist["adaptivity"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(spread);
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["mesh smoothing"] = vlist;

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
  vlist << QVariant("float");
  vlist << QVariant(m_scaleModel);
  vlist << QVariant(0.001);
  vlist << QVariant(1.0);
  vlist << QVariant(0.005); // singlestep
  vlist << QVariant(3); // decimals
  plist["scale"] = vlist;

  vlist.clear();
  vlist << QVariant("color");
  vlist << QColor(Qt::white);
  plist["color"] = vlist;
  
  vlist.clear();
  vlist << QVariant("combobox");
  vlist << "1";
  vlist << "Fixed Color";
  vlist << "Lut Color";
  plist["color type"] = vlist;
  QStringList colortypes;
  colortypes << "Fixed Color";
  colortypes << "Lut Color";


  vlist.clear();
  QFile helpFile(":/meshgenerator.help");
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
  keys << "adaptivity";
  keys << "mesh smoothing";
  keys << "depth";
  keys << "fillvalue";
  keys << "scale";
  keys << "greater";
  keys << "color";
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
	    //smoothOpacity = pair.first.toBool();
	    smoothOpacity = pair.first.toInt();
	  else if (keys[ik] == "average color")
	    avgColor = pair.first.toBool();
	  else if (keys[ik] == "apply tag colors")
	    m_useTagColors = pair.first.toBool();
	  else if (keys[ik] == "mop channel")
	    chan = pair.first.toInt();
	  else if (keys[ik] == "color")
	    {
	      QColor col = pair.first.value<QColor>();
	      vstops.clear();
	      vstops << QGradientStop(0.0, col);
	      vstops << QGradientStop(1.0, col);
	    }
	  else if (keys[ik] == "isosurface value")
	    isovalf = pair.first.toFloat();
	  else if (keys[ik] == "adaptivity")
	    adaptivity = pair.first.toFloat();
	  else if (keys[ik] == "scale")
	    m_scaleModel = pair.first.toFloat();
	  else if (keys[ik] == "mesh smoothing")
	    spread = pair.first.toInt();
	  else if (keys[ik] == "depth")
	    depth = pair.first.toInt();
	  else if (keys[ik] == "fillvalue")
	    fillValue = pair.first.toInt();
	  else if (keys[ik] == "greater")
	    checkForMore = pair.first.toBool();
	  else if (keys[ik] == "color type")
	    {
	      useColor = pair.first.toInt();
	      useColor = qBound(0, useColor, 1);
	    }
	}
    }

  isoval = qRound(isovalf*255);
  if (m_voxelType > 0 && !useOpacity)
    isoval = qRound(isovalf*65535);

    
  stops = resampleGradientStops(vstops);

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
		     QVector<uchar> tagColors,
		     bool bm)
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

  m_batchMode = bm;
  
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
  bool useOpacity;
  int smoothOpacity;
  QGradientStops stops;
  int chan;
  bool avgColor;
  float adaptivity;
  if (! getValues(isoval, isovalf,
		  spread, depth, useColor, fillValue,
		  checkForMore,
		  useOpacity,
		  smoothOpacity,
		  stops,
		  doBorder,
		  chan,
		  avgColor,
		  adaptivity))
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


  float memGb = 8.0;
  if (!m_batchMode)
    memGb = QInputDialog::getDouble(0,
				    "Use memory",
				    "Max memory we can use (GB)", memGb, 0.1, 1000, 2);
  else
    memGb = 8.0; // for batch mode assume 5GB
  

  qint64 gb = 1024*1024*1024;
  qint64 memsize = memGb*gb; // max memory we can use (in GB)

  qint64 canhandle = memsize/11;
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
  QString flnm;
  if (!m_batchMode)
    flnm = QFileDialog::getSaveFileName(0,
					"Export mesh to file",
					prevDir,
					"*.ply ;; *.obj ;; *.stl");
  else
    {
      flnm = QDir(prevDir).filePath("mesh.ply");
    }
  
  if (flnm.size() == 0)
    {
      meshWindow->close();
      return "";
    }

  if (!StaticFunctions::checkExtension(flnm, ".ply") &&
      !StaticFunctions::checkExtension(flnm, ".obj") &&
      !StaticFunctions::checkExtension(flnm, ".stl"))
    flnm += ".ply";
  //----------------------------

  int nSlabs = 1;
  qint64 reqmem = m_nX;
  reqmem *= m_nY*m_nZ*6;
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
	       voxelScaling,
	       clipPos, clipNormal,
	       crops, paths,
	       useOpacity, smoothOpacity, lut,
	       chan,
	       avgColor,
	       adaptivity);


  meshWindow->close();

  return flnm;
}

QColor
MeshGenerator::getLutColor(uchar *volData,	  
			   int dlen, int depth, int spread,
			   uchar isoval,
			   QVector3D pos,
			   QVector3D normal,
			   uchar *lut,
			   QVector3D globalPos,
			   bool avgColor)			   
{
  // go a bit deeper and start
  QVector3D vpos = pos - normal;

  // -- find how far deep we can go
  int nd = 0;
  for(int n=0; n<=depth; n++)
    {
      qint64 i = vpos.x();
      qint64 j = vpos.y();
      qint64 k = vpos.z();
      if (i > m_nZ-1 || j > m_nY-1 || k > dlen-1 ||
	  i < 0 || j < 0 || k < 0) // gone out
	break;
      nd ++;
      vpos -= normal;
    }

  // now start collecting the samples
  vpos = pos - normal;
  QVector3D gpos = globalPos - normal;

  Vec rgb = Vec(0,0,0);
  float tota = 0;
  for(int ns=0; ns<=nd; ns++)
    {
      qint64 i = vpos.x();
      qint64 j = vpos.y();
      qint64 k = vpos.z();
      
      i = qBound((qint64)0, i, (qint64)(m_nZ-1));
      j = qBound((qint64)0, j, (qint64)(m_nY-1));
      k = qBound((qint64)0, k, (qint64)(dlen-1));

      Vec po0 = Vec(m_dataMin.x+gpos.x(), m_dataMin.y+gpos.y(), gpos.z());
      Vec po = po0*m_samplingLevel;

      bool ok=true;

      if (ok)
	{	  
	  ushort v, gr;
	  if (m_voxelType == 0)
	    {
	      v = volData[k*m_nY*m_nZ + j*m_nZ + i];

	      int a = volData[k*m_nY*m_nZ + j*m_nZ + qMin((qint64)m_nZ-1,i+1)] -
		      volData[k*m_nY*m_nZ + j*m_nZ + qMax((qint64)0,i-1)];
	      int b = volData[k*m_nY*m_nZ + qMin((qint64)m_nY-1,j+1)*m_nZ + i] -
		      volData[k*m_nY*m_nZ + qMax((qint64)0,j-1)*m_nZ + i];
	      int c = volData[qMin((qint64)(dlen-1),k+1)*m_nY*m_nZ + j*m_nZ + i] -
		      volData[qMax((qint64)0,k-1)*m_nY*m_nZ + j*m_nZ + i];

	      gr = qMin(256, int(qSqrt(a*a+b*b+c*c)));
	      //gr = 0;
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

      vpos -= normal;
      gpos -= normal;
    }

  
  if (tota < 0.01) // if total opacity is 0, set white color 
    rgb = Vec(1,1,1);

  if (tota < 0.01) tota = 1.0;
  rgb /= tota;    
  rgb *= 255;
  QColor col = QColor(rgb.x, rgb.y, rgb.z);

  return col;
}

//void
//MeshGenerator::smoothData(uchar *gData,
//			  int dlen, int nY, int nZ,
//			  int spread)
//{
//  m_meshLog->moveCursor(QTextCursor::End);
//  m_meshLog->insertPlainText("Smoothing data ...\n");
//
//  uchar *tmp = new uchar[qMax(dlen, qMax(nY, nZ))];
//
//  for(qint64 k=0; k<dlen; k++)
//    {
//      m_meshProgress->setValue((int)(100.0*(float)k/(float)(dlen)));
//      qApp->processEvents();
//
//      for(qint64 j=0; j<nY; j++)
//	{
//	  memset(tmp, 0, nZ);
//	  for(qint64 i=0; i<nZ; i++)
//	    {
//	      float v = 0.0f;
//	      int nt = 0;
//	      for(qint64 i0=qMax(0,i-spread); i0<=qMin((qint64)nZ-1,i+spread); i0++)
//		{
//		  nt++;
//		  v += gData[k*nY*nZ + j*nZ + i0];
//		}
//	      tmp[i] = v/nt;
//	    }
//	  
//	  for(qint64 i=0; i<nZ; i++)
//	    gData[k*nY*nZ + j*nZ + i] = tmp[i];
//	}
//    }
//
//
//  for(int k=0; k<dlen; k++)
//    {
//      m_meshProgress->setValue((int)(100.0*(float)k/(float)(dlen)));
//      qApp->processEvents();
//
//      for(int i=0; i<nZ; i++)
//	{
//	  memset(tmp, 0, nY);
//	  for(qint64 j=0; j<nY; j++)
//	    {
//	      float v = 0.0f;
//	      int nt = 0;
//	      for(qint64 j0=qMax(0,j-spread); j0<=qMin(nY-1,j+spread); j0++)
//		{
//		  nt++;
//		  v += gData[k*nY*nZ + j0*nZ + i];
//		}
//	      tmp[j] = v/nt;
//	    }	  
//	  for(qint64 j=0; j<nY; j++)
//	    gData[k*nY*nZ + j*nZ + i] = tmp[j];
//	}
//    }
//  
//  for(qint64 j=0; j<nY; j++)
//    {
//      m_meshProgress->setValue((int)(100.0*(float)j/(float)(nY)));
//      qApp->processEvents();
//
//      for(qint64 i=0; i<nZ; i++)
//	{
//	  memset(tmp, 0, dlen);
//	  for(qint64 k=0; k<dlen; k++)
//	    {
//	      float v = 0.0f;
//	      int nt = 0;
//	      for(int k0=qMax(0,k-spread); k0<=qMin(dlen-1,k+spread); k0++)
//		{
//		  nt++;
//		  v += gData[k0*nY*nZ + j*nZ + i];
//		}
//	      tmp[k] = v/nt;
//	    }
//	  for(int k=0; k<dlen; k++)
//	    gData[k*nY*nZ + j*nZ + i] = tmp[k];
//	}
//    }
//  
//  delete [] tmp;
//
//  m_meshProgress->setValue(100);
//}

void
MeshGenerator::applyTear(int d0, int d1,
			 uchar *data0, uchar *data1,
			 bool flag)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("apply dissection ...\n");

  int dlen = d1-d0+1;
  for(qint64 i0=d0; i0<=d1; i0++)
    {
      qint64 i = i0 - d0;
      m_meshProgress->setValue((int)(100.0*(float)((i0-d0)/(float)(dlen))));
      qApp->processEvents();

      int iv = qBound(0, (int)i0, m_depth-1);
      for(qint64 j=0; j<m_nY; j++)
	for(qint64 k=0; k<m_nZ; k++)
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
			  data1[(i0-d0)*m_nY*m_nZ + j*m_nZ + k] = 0;
			else 
			  ((ushort*)data1)[(i0-d0)*m_nY*m_nZ + j*m_nZ + k] = 0;
		      }
		    else
		      {
			newPo -= Vec(m_dataMin.x, m_dataMin.y, m_dataMin.z);
			int newi = ceil(newPo.z);
			int newj = ceil(newPo.y);
			int newk = ceil(newPo.x);
			newi = qBound(d0, newi, d1);
			newj = qBound(0, newj, m_nY-1);
			newk = qBound(0, newk, m_nZ-1);
			if (!flag || m_voxelType == 0)
			  data1[(i0-d0)*m_nY*m_nZ + j*m_nZ + k] = 
			    data0[(newi-d0)*m_nY*m_nZ + newj*m_nZ + newk];
			else
			  ((ushort*)data1)[(i0-d0)*m_nY*m_nZ + j*m_nZ + k] = 
			    ((ushort*)data0)[(newi-d0)*m_nY*m_nZ + newj*m_nZ + newk];
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
			    uchar* tmp,
			    uchar* tmp0, uchar* tmp1, uchar* tmp2)
{
  int jk = 0;
  for(int j=0; j<m_nY; j++)
    for(int k=0; k<m_nZ; k++)
      {
	if (cropped[jk] > 0)
	  {
	    int v;
	    if (m_voxelType == 0)
	      v = tmp1[jk];
	    else
	      v = ((ushort*)tmp1)[jk];

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
	      {
		// using the gradient to calculate the opacity
		int a = tmp1[qMin(m_nY,j+1)*m_nZ]-tmp1[qMax(0, j-1)*m_nZ];
		int b = tmp1[j*m_nZ + qMin(m_nZ,k+1)]-tmp1[j*m_nZ + qMax(0, k-1)];
		int c = tmp2[j*m_nZ + k]-tmp0[j*m_nZ + k];
		int gr = qMin(256,int(qSqrt(a*a + b*b + c*c)));
		  
		opac = mop*lut[tfSet + 4*(256*gr + v) + 3];

		// ignoring gradient to calculate opacity
		//opac = mop*lut[tfSet + 4*v + 3];
	      }
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
    }  return cropped;
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
			    Vec voxelScaling,
			    QList<Vec> clipPos,
			    QList<Vec> clipNormal,
			    QList<CropObject> crops,
			    QList<PathObject> paths,
			    bool useOpacity, int smoothOpacity, 
			    uchar *lut,
			    int chan,
			    bool avgColor,
			    float adaptivity)
{
  int saveType = 0; // default .ply
  if (StaticFunctions::checkExtension(flnm, ".obj")) saveType = 1;
  if (StaticFunctions::checkExtension(flnm, ".stl")) saveType = 2;

  bool saveIntermediate = false;

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

      qint64 dataSize = m_nY*m_nZ;
      dataSize *= dlen;
      uchar *extData;
      if (m_voxelType == 0)
	extData = new uchar[dataSize];
      else
	extData = new uchar[2*dataSize]; // ushort

      uchar *gData=0;
      if (useOpacity)
	gData = new uchar[dataSize];


      uchar *cropped = new uchar[nbytes];
      uchar *tmp = new uchar[nbytes];
      uchar *tmp0 = new uchar[nbytes];
      uchar *tmp1 = new uchar[nbytes];
      uchar *tmp2 = new uchar[nbytes];
      memset(tmp,  0, nbytes);
      memset(tmp0, 0, nbytes);
      memset(tmp1, 0, nbytes);
      memset(tmp2, 0, nbytes);
      
      int i0 = 0;
      int i0end = d1z-d0z;
      for(int i=d0z; i<=d1z; i++)
	{
	  m_meshProgress->setValue((int)(100.0*(float)(i0/(float)dlen)));
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
	      } // j,k loop

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
	  memcpy(extData + bpv*i0*qint64(m_nY*m_nZ), tmp, nbytes);

	  if (useOpacity)
	    {
	      memcpy(tmp0, tmp1, nbytes);
	      memcpy(tmp1, tmp2, nbytes);
	      memcpy(tmp2, tmp, nbytes);

	      if (i0 == 1)
		{
		  applyOpacity(iv, cropped, lut, tmp,
			       tmp1, tmp2, tmp2); // i0 == 0
		  memcpy(gData + i0*qint64(m_nY*m_nZ), tmp, m_nY*m_nZ);
		}
	      if (i0 > 1)
		{
		  applyOpacity(iv, cropped, lut, tmp,
			       tmp0, tmp1, tmp2); // i0 == 1
		  memcpy(gData + i0*qint64(m_nY*m_nZ), tmp, m_nY*m_nZ);
		  if (i0 == i0end)
		    {
		      applyOpacity(iv, cropped, lut, tmp,
				   tmp1, tmp2, tmp2); // i0 == 1
		      memcpy(gData + i0*qint64(m_nY*m_nZ), tmp, m_nY*m_nZ);
		    }
		}
	    }

	  i0++;
	} // i loop
      
      delete [] tmp;
      delete [] tmp0;
      delete [] tmp1;
      delete [] tmp2;
      delete [] cropped;
      m_meshProgress->setValue(100);
      qApp->processEvents();

      //------------
      if (m_tearPresent)
	{
	  uchar *data0 = new uchar[dataSize];

	  uchar *data1 = extData;
	  memcpy(data0, data1, dataSize);
	  applyTear(d0, d1,
		    data0, data1, true);

	  if (useOpacity)
	    {
	      data1 = gData;
	      memcpy(data0, data1, dataSize);
	      applyTear(d0, d1,
			data0, data1, false);
	    }

	  delete [] data0;
	}
      //------------


      //--------------------------------
      // ---- set border voxels to fillValue
      if (fillValue >= 0)
	{
	  uchar *v = extData;
	  if (useOpacity) v = gData;

	  i0 = 0;
	  for(int i=d0z; i<=d1z; i++)
	    {
	      int iv = qBound(0, i, m_depth-1);
	      qint64 i0dx = i0*qint64(m_nY*m_nZ);
	      if (iv <= qRound(m_dataMin.z) || iv >= qRound(m_dataMax.z))
		{
		  if (useOpacity || m_voxelType == 0)
		    memset(v + i0dx, fillValue, m_nY*m_nZ);
		  else
		    {
		      for(int fi=0; fi<m_nY*m_nZ; fi++)
			((ushort*)v)[i0dx + fi] = fillValue;
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
      //--------------------------------
      m_meshLog->moveCursor(QTextCursor::End);
      m_meshLog->insertPlainText("Generating VDB ...\n");
      VdbVolume vdb;
      if (!useOpacity)
	vdb.generateVDB(extData,
			dlen, m_nY, m_nZ,
			-1, 1,  // values less than 1 are background
			m_meshProgress);
      else
      	vdb.generateVDB(gData,
			dlen, m_nY, m_nZ,
			-1, 1,  // values less than 1 are background
			m_meshProgress);
      
      if (smoothOpacity > 0)
	{
	  m_meshLog->moveCursor(QTextCursor::End);
	  m_meshLog->insertPlainText("Smoothing volume ...\n");
	  vdb.dilate(1);
	  vdb.mean(1, smoothOpacity); // width, iterations
	}

      m_meshLog->moveCursor(QTextCursor::End);
      m_meshLog->insertPlainText(QString("Generating mesh with adaptivity %1 ...\n").arg(adaptivity));
      QVector<QVector3D> V;
      QVector<QVector3D> VN;
      QVector<int> T;
      vdb.generateMesh(isoval, adaptivity, V, VN, T);
      
      QVector<Vec> E;
      for(int i=0; i<T.count()/3; i++)
	E << Vec(T[3*i+0], T[3*i+1], T[3*i+2]);

      int nverts = V.count();
      int ntrigs = E.count();
	
      //--------------------------------
      //--------------------------------


      // mesh smoothing
      if (spread > 0)
	smoothMesh(V, VN, E, 5*spread);
 
      {
	m_meshLog->moveCursor(QTextCursor::End);
	m_meshLog->insertPlainText("Saving triangle coordinates ...\n");
	QString mflnm = flnm + QString(".%1.tri").arg(nb);
	QFile fout(mflnm);
	fout.open(QFile::WriteOnly);
	fout.write((char*)&ntrigs, 4);
	if (saveType < 2) // save modified triangle vertex numbers only for PLY and OBJ files
	  {
	    for(int ni=0; ni<ntrigs; ni++)
	      {
		int v[3];
		v[0] = E[ni].x + nvertices;
		v[1] = E[ni].y + nvertices;
		v[2] = E[ni].z + nvertices;
		fout.write((char*)v, 12);
	      }
	  }
	else // saving stl formatted output
	  {
	    for(int ni=0; ni<ntrigs; ni++)
	      {
		int v[3];
		v[0] = E[ni].x;
		v[1] = E[ni].y;
		v[2] = E[ni].z;
		fout.write((char*)v, 12);
	      }
	  }
	fout.close();
	ntriangles += ntrigs;
      }      
  
      {
	QString mflnm = flnm + QString(".%1.vert").arg(nb);
	QFile fout(mflnm);
	fout.open(QFile::WriteOnly);
	fout.write((char*)&nverts, 4);
	for(int ni=0; ni<nverts; ni++)
	  {
	    m_meshProgress->setValue((int)(100.0*(float)ni/(float)nverts));
	    qApp->processEvents();
	    
	    float v[6];
	    v[0] = V[ni].x();
	    v[1] = V[ni].y();
	    v[2] = V[ni].z() + d0;
	    v[3] = VN[ni].x();
	    v[4] = VN[ni].y();
	    v[5] = VN[ni].z();
	    
	    // apply voxelscaling
	    v[0] *= voxelScaling.x;
	    v[1] *= voxelScaling.y;
	    v[2] *= voxelScaling.z;
	    v[0] *= m_scaleModel;
	    v[1] *= m_scaleModel;
	    v[2] *= m_scaleModel;
	    fout.write((char*)v, 24);
	    
	    if (saveType == 0) // do colour calculations only for PLY files
	      {
		//m_meshLog->moveCursor(QTextCursor::End);
		//m_meshLog->insertPlainText("Generating Color ...\n");
		uchar c[3];
		float r,g,b;
		if (useColor == _FixedColor)
		  {
		    QColor col = vstops[isoval].second;
		    r = col.red()/255.0;
		    g = col.green()/255.0;
		    b = col.blue()/255.0;
		  }
		else
		  {
		    uchar *volData = extData;
		    QColor col = Qt::white;
		    QVector3D pos = V[ni];
		    QVector3D normal = VN[ni];
		    col = getLutColor(volData,
				      dlen, depth, 0,
				      isoval,
				      pos, normal, // position within slab
				      lut,
				      V[ni]+QVector3D(0,0,d0), // global position
				      avgColor);
		    
		    r = col.red()/255.0;
		    g = col.green()/255.0;
		    b = col.blue()/255.0;
		  }
		
		c[0] = r*255;
		c[1] = g*255;
		c[2] = b*255;
		fout.write((char*)c, 3);
	      } // if (saveType == 0)
	  }
	fout.close();
	nvertices += nverts;
	m_meshProgress->setValue(100);
      }
      
      delete [] extData;
      if (gData) delete [] gData;
    } // loop over slabs


  
  if (saveType == 0) // PLY
    saveMeshToPLY(flnm,
		  nSlabs,
		  nvertices, ntriangles,
		  true);
  else if (saveType == 1) // OBJ	
    saveMeshToOBJ(flnm,
		  nSlabs,
		  nvertices, ntriangles);
  else // STL
    saveMeshToSTL(flnm,
		  nSlabs,
		  nvertices, ntriangles);

  
  
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Mesh saved in "+flnm);

  if (!m_batchMode)
    {
      QMessageBox dlg(QMessageBox::Information, "Surface Mesh Saved", QString("Mesh saved in "+flnm));
      dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint);
      dlg.exec();
    }


}
void
MeshGenerator::saveMeshToPLY(QString flnm,
			     int nSlabs,
			     int nvertices, int ntriangles,
			     bool bin)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Saving Mesh " + flnm);

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

  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(myVertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(myVertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(myVertex,z), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(myVertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(myVertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(myVertex,nz), 0, 0, 0, 0},
    {plyStrings[6], Uint8, Uint8, offsetof(myVertex,r), 0, 0, 0, 0},
    {plyStrings[7], Uint8, Uint8, offsetof(myVertex,g), 0, 0, 0, 0},
    {plyStrings[8], Uint8, Uint8, offsetof(myVertex,b), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(PlyFace,verts),
     1, Uint8, Uint8, offsetof(PlyFace,nverts)},
  };

  PlyFile    *ply;
  FILE       *fp = fopen(flnm.toLatin1().data(),
			 bin ? "wb" : "w");

  PlyFace     face ;
  int         verts[3] ;
  char       *elem_names[]  = {plyStrings[10], plyStrings[11]};
  ply = write_ply (fp,
		   2,
		   elem_names,
		   bin? PLY_BINARY_LE : PLY_ASCII );

  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, plyStrings[10], nvertices );
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
  describe_element_ply ( ply, plyStrings[11], ntriangles );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );


  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, plyStrings[10] );
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
	  vertex.nx = v[3];
	  vertex.ny = v[4];
	  vertex.nz = v[5];
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
  put_element_setup_ply ( ply, plyStrings[11] );
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

	  face.verts[0] = v[0];
	  face.verts[1] = v[1];
	  face.verts[2] = v[2];

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

void
MeshGenerator::saveMeshToOBJ(QString objflnm,
			     int nSlabs,
			     int nvertices, int ntriangles)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Saving Mesh " + objflnm);

//  // calculate number of triangles
//  int ntri = 0;
//  for (int nb=0; nb<nSlabs; nb++)
//    {
//      QString tflnm = objflnm + QString(".%1.tri").arg(nb);
//      QFile tfin(tflnm);
//      tfin.open(QFile::ReadOnly);
//      int ntrigs;
//      tfin.read((char*)&ntrigs, 4);
//      tfin.close();
//      ntri += ntrigs;
//    }

  QFile fobj(objflnm);
  fobj.open(QFile::WriteOnly);
  QTextStream out(&fobj);
  out << "#\n";
  out << "#  Wavefront OBJ generated by Drishti\n";
  out << "#\n";
  out << "#  https://github.com/nci/drishti\n";
  out << "#\n";
  out << QString("# %1 vertices\n").arg(nvertices);
  out << QString("# %1 normals\n").arg(nvertices);
  out << QString("# %1 triangles\n").arg(ntriangles);
  out << "g\n";
  for (int nb=0; nb<nSlabs; nb++)
    {
      m_meshProgress->setValue((int)(100.0*(float)nb/(float)nSlabs));
      qApp->processEvents();

      int nverts;
      QString mflnm = objflnm + QString(".%1.vert").arg(nb);

      QFile fin(mflnm);
      fin.open(QFile::ReadOnly);
      fin.read((char*)&nverts, 4);
      for(int ni=0; ni<nverts; ni++)
	{
	  float v[6];
	  fin.read((char*)v, 24);

	  out << "v " << QString("%1 %2 %3\n").arg(v[0]).arg(v[1]).arg(v[2]);
	  out << "vn "<< QString("%1 %2 %3\n").arg(v[3]).arg(v[4]).arg(v[5]);
	}
      fin.close();
      fin.remove();
    }
  out << "g\n";
  for (int nb=0; nb<nSlabs; nb++)
    {
      m_meshProgress->setValue((int)(100.0*(float)nb/(float)nSlabs));
      qApp->processEvents();

      int ntrigs;
      QString mflnm = objflnm + QString(".%1.tri").arg(nb);

      QFile fin(mflnm);
      fin.open(QFile::ReadOnly);
      fin.read((char*)&ntrigs, 4);      
      for(int ni=0; ni<ntrigs; ni++)
	{
	  int v[3];
	  fin.read((char*)v, 12);
	  out << "f " << QString("%1 %2 %3\n").arg(v[0]+1).arg(v[1]+1).arg(v[2]+1);
	}
      fin.close();
      fin.remove();
    }

  m_meshProgress->setValue(100);
}


void
MeshGenerator::saveMeshToSTL(QString flnm,
			     int nSlabs,
			     int nvertices, int ntriangles)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Saving Mesh " + flnm);

  // calculate number of triangles
  int ntri = 0;
  for (int nb=0; nb<nSlabs; nb++)
    {
      QString tflnm = flnm + QString(".%1.tri").arg(nb);
      QFile tfin(tflnm);
      tfin.open(QFile::ReadOnly);
      int ntrigs;
      tfin.read((char*)&ntrigs, 4);
      tfin.close();
      ntri += ntrigs;
    }

  char header[80];
  sprintf(header, "Drishti generated STL file.");
  QFile fstl(flnm);
  fstl.open(QFile::WriteOnly);
  fstl.write((char*)&header, 80); // 80 byte header
  fstl.write((char*)&ntri, 4); // number of triangles

  for (int nb=0; nb<nSlabs; nb++)
    {
      m_meshProgress->setValue((int)(100.0*(float)nb/(float)nSlabs));
      qApp->processEvents();

      int ntrigs;
      QString tflnm = flnm + QString(".%1.tri").arg(nb);
      QFile tfin(tflnm);
      tfin.open(QFile::ReadOnly);
      tfin.read((char*)&ntrigs, 4);      
      int *tri;
      tri = new int[3*ntrigs];
      tfin.read((char*)tri, 4*3*ntrigs);

      int nverts;
      QString vflnm = flnm + QString(".%1.vert").arg(nb);
      QFile vfin(vflnm);
      vfin.open(QFile::ReadOnly);
      vfin.read((char*)&nverts, 4);

      float *vert;
      vert = new float[6*nverts];
      vfin.read((char*)vert, 4*6*nverts);

      for(int ni=0; ni<ntrigs; ni++)
	{
	  float v[12];
	  int k = tri[3*ni+0];
	  int j = tri[3*ni+1];
	  int i = tri[3*ni+2];

	  v[0] = vert[6*i+3];
	  v[1] = vert[6*i+4];
	  v[2] = vert[6*i+5];
	  v[0] += vert[6*j+3];
	  v[1] += vert[6*j+4];
	  v[2] += vert[6*j+5];
	  v[0] += vert[6*k+3];
	  v[1] += vert[6*k+4];
	  v[2] += vert[6*k+5];
	  float mag = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	  v[0]/=mag;
	  v[1]/=mag;
	  v[2]/=mag;
	  v[0] = -v[0];
	  v[1] = -v[1];
	  v[2] = -v[2];

	  v[3] = vert[6*i+0];
	  v[4] = vert[6*i+1];
	  v[5] = vert[6*i+2];

	  v[6] = vert[6*j+0];
	  v[7] = vert[6*j+1];
	  v[8] = vert[6*j+2];

	  v[9]  = vert[6*k+0];
	  v[10] = vert[6*k+1];
	  v[11] = vert[6*k+2];

	  fstl.write((char*)&v, 12*4);

	  ushort abc = 0; // attribute byte count
	  fstl.write((char*)&abc, 2);
	}

      vfin.close();
      tfin.close();
      vfin.remove();
      tfin.remove();

      delete [] vert;
      delete [] tri;
    }

  fstl.close();
  m_meshProgress->setValue(100);
}

void
MeshGenerator::smoothMesh(QVector<QVector3D>& V,
			  QVector<QVector3D>& N,
			  QVector<Vec>& E,
			  int ntimes)
{  
  QVector<QVector3D> newV;
  newV = V;

  int nv = V.count();

  //----------------------------
  // create incidence matrix
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Smoothing mesh\n   Generate incidence matrix ...");
  QMultiMap<int, int> imat;
  int ntri = E.count();
  for(int i=0; i<ntri; i++)
    {
      if (i%10000 == 0)
	{
	  m_meshProgress->setValue((int)(100.0*(float)i/(float)(ntri)));
	  qApp->processEvents();
	}

      int a = E[i].x;
      int b = E[i].y;
      int c = E[i].z;

      imat.insert(a, b);
      imat.insert(b, a);
      imat.insert(a, c);
      imat.insert(c, a);
      imat.insert(b, c);
      imat.insert(c, b);
    }
  //----------------------------

  //----------------------------
  // smooth vertices
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("   Smoothing vertices ...");
  for(int nt=0; nt<ntimes; nt++)
    {
      m_meshProgress->setValue((int)(100.0*(float)nt/(float)(ntimes)));
      qApp->processEvents();

      // deflation step
      for(int i=0; i<nv; i++)
	{
	  QList<int> idx = imat.values(i);
	  QVector3D v0 = V[i];
	  QVector3D v = QVector3D(0,0,0);
	  float sum = 0;
	  for(int j=0; j<idx.count(); j++)
	    {
	      QVector3D vj = V[idx[j]];
	      float ln = (v0-vj).length();
	      if (ln > 0)
		{
		  sum += 1.0/ln;
		  v = v + vj/ln;
		}
	    }
	  if (sum > 0)
	    v0 = v0 + 0.9*(v/sum - v0);
	  newV[i] = v0;
	}

      //inflation step
      for(int i=0; i<nv; i++)
	{
	  QList<int> idx = imat.values(i);
	  QVector3D v0 = newV[i];
	  QVector3D v = QVector3D(0,0,0);
	  float sum = 0;
	  for(int j=0; j<idx.count(); j++)
	    {
	      QVector3D vj = newV[idx[j]];
	      float ln = (v0-vj).length();
	      if (ln > 0)
		{
		  sum += 1.0/ln;
		  v = v + vj/ln;
		}
	    }
	  if (sum > 0)
	    v0 = v0 - 0.5*(v/sum - v0);

	  V[i] = v0;
	}
    }
  //----------------------------


  //----------------------------
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("   Calculate normals ...");
  // now calculate normals
  for(int i=0; i<nv; i++)
    newV[i] = QVector3D(0,0,0);

  QVector<int> nvs;
  nvs.resize(nv);
  nvs.fill(0);

  for(int i=0; i<ntri; i++)
    {
      if (i%10000 == 0)
	{
	  m_meshProgress->setValue((int)(100.0*(float)i/(float)(ntri)));
	  qApp->processEvents();
	}

      int a = E[i].x;
      int b = E[i].y;
      int c = E[i].z;

      QVector3D va = V[a];
      QVector3D vb = V[b];
      QVector3D vc = V[c];
      QVector3D v0 = (vb-va).normalized();
      QVector3D v1 = (vc-va).normalized();      
      QVector3D vn = QVector3D::crossProduct(v1,v0);
      
      newV[a] += vn;
      newV[b] += vn;
      newV[c] += vn;

      nvs[a]++;
      nvs[b]++;
      nvs[c]++;
    }

  for(int i=0; i<nv; i++)
      N[i] = newV[i]/nvs[i];
  //----------------------------

  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("\n");
  m_meshProgress->setValue(100);
}

