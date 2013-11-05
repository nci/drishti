#include "staticfunctions.h"
#include "meshgenerator.h"

MeshGenerator::MeshGenerator() {vcolor=0;}
MeshGenerator::~MeshGenerator() {if (vcolor) delete [] vcolor;}

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
MeshGenerator::getValues(int &depth,
			 int &fillValue,
			 bool &checkForMore,
			 bool &lookInside,
			 QGradientStops &stops,
			 bool doBorder,
			 int &chan,
			 bool &avgColor)
{
  chan = 0;
  depth = 1;
  fillValue = -1;
  checkForMore = true;
  lookInside = false;
  avgColor = false;
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

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(avgColor);
  plist["average color"] = vlist;

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
  keys << "average color";
  keys << "apply tag colors";
  keys << "mop channel";
  keys << "isosurface value";
  keys << "depth";
  keys << "fillvalue";
  keys << "scale";
  keys << "greater";
  keys << "look inside";
  keys << "color gradient";
  keys << "commandhelp";
  keys << "message";

  propertyEditor.set("Mesh Repainting Parameters", plist, keys);
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
	  if (keys[ik] == "average color")
	    avgColor = pair.first.toBool();
	  else if (keys[ik] == "apply tag colors")
	    m_useTagColors = pair.first.toBool();
	  else if (keys[ik] == "mop channel")
	    chan = pair.first.toInt();
	  else if (keys[ik] == "color gradient")
	    vstops = propertyEditor.getGradientStops(keys[ik]);
	  else if (keys[ik] == "scale")
	    m_scaleModel = pair.first.toFloat();
	  else if (keys[ik] == "depth")
	    depth = pair.first.toInt();
	  else if (keys[ik] == "fillvalue")
	    fillValue = pair.first.toInt();
	  else if (keys[ik] == "greater")
	    checkForMore = pair.first.toBool();
	  else if (keys[ik] == "look inside")
	    lookInside = pair.first.toBool();
	}
    }

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

  int depth, fillValue;
  bool checkForMore;
  bool lookInside;
  QGradientStops stops;
  int chan;
  bool avgColor;
  if (! getValues(depth, fillValue,
		  checkForMore,
		  lookInside,
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
  meshWindow->setWindowTitle("Drishti - Mesh Repainting Using Voxel Values");
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


  m_meshLog->insertPlainText(QString("Grid size : %1 %2 %3\n").\
			   arg(m_nX).arg(m_nY).arg(m_nZ));
  

  //---- import the mesh ---
  QString inflnm = QFileDialog::getOpenFileName(0,
						"Load mesh to repaint",
						prevDir,
						"*.ply");
  if (inflnm.size() == 0)
    {
      meshWindow->close();
      return "";
    }

  if (!loadPLY(inflnm))
    {
      meshWindow->close();
      return "";
    }
  //----------------------------

  //---- export the mesh ---
  QString outflnm = QFileDialog::getSaveFileName(0,
						 "Export mesh",
						 prevDir,
						 "*.ply");
  if (outflnm.size() == 0)
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

  QStringList volumeFiles;
#ifndef Q_OS_MACX
  volumeFiles = QFileDialog::getOpenFileNames(0,
					      "Load volume files for timeseries data, otherwise give CANCEL",
					      prevDir,
					      "NetCDF Files (*.pvl.nc)",
					      0,
					      QFileDialog::DontUseNativeDialog);
#else
  volumeFiles = QFileDialog::getOpenFileNames(0,
					      "Load volume files for timeseries data, otherwise give CANCEL",
					      prevDir,
					      "NetCDF Files (*.pvl.nc)",
					      0);
#endif

  int nvols = volumeFiles.count();      

  if (nvols == 0)
    volumeFiles << m_vfm->fileName();
  

  generateMesh(nSlabs,
	       volumeFiles,
	       outflnm,
	       depth,
	       stops,
	       fillValue,
	       checkForMore,
	       lookInside,
	       voxelScaling,
	       clipPos, clipNormal,
	       crops, paths,
	       lut,
	       chan,
	       avgColor);


  meshWindow->close();

  return outflnm;
}

QColor
MeshGenerator::getVRLutColor(uchar *volData,	  
			     int dlen,
			     int depth, int nextra,
			     QVector3D pos,
			     QVector3D normal,
			     uchar *lut,
			     bool lookInside,
			     QVector3D globalPos)
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

//	  QMessageBox::information(0, "", QString("vrlut : %1 %2 %3 : %4").\
//				   arg(i).arg(j).arg(k).arg(v));

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
	  
	  rgb = (1-a)*rgb + Vec(r,g,b)/255.0f;
	  tota = (1-a)*tota + a;

	  //rgb += Vec(r,g,b)/255.0f;
	  //tota += a;

	}

      vpos += normal;
      gpos += normal;
    }

//  if (tota < 0.01) tota = 1.0;
//  rgb /= tota;
  rgb *= 255;
  QColor col = QColor(rgb.x, rgb.y, rgb.z, 255*tota);

  return col;
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
			    QStringList volumeFiles,
			    QString flnm,
			    int depth,
			    QGradientStops vstops,
			    int fillValue,
			    bool checkForMore,
			    bool lookInside,
			    Vec voxelScaling,
			    QList<Vec> clipPos,
			    QList<Vec> clipNormal,
			    QList<CropObject> crops,
			    QList<PathObject> paths,
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

  int nextra = depth;
  int blockStep = m_nX/nSlabs;


  //-----------------------------
  int nvols = volumeFiles.count();      
  //-----------------------------

  for (int volnum=0; volnum < nvols; volnum++)
    {
      //if (nvols > 1)
	{
	  m_vfm->setBaseFilename(volumeFiles[volnum]);
	  uchar *vslice = m_vfm->getSlice(0);
	}

      m_meshLog->moveCursor(QTextCursor::End);
      m_meshLog->insertPlainText(QString("\nProcessing file %1 of %2 : %3\n").\
				 arg(volnum+1).arg(nvols).arg(m_vfm->fileName()));

      for (int nb=0; nb<nSlabs; nb++)
	{
	  m_meshLog->moveCursor(QTextCursor::End);
	  m_meshLog->insertPlainText(QString("  Processing slab %1 of %2\n").arg(nb+1).arg(nSlabs));
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
		    // no mop pruning

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
		      //cropped[jk] = mop; // nop mop pruning
		      cropped[jk] = 255;
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
	      
	      delete [] data0;
	    }
	  //------------
	  
	  
	  //--------------------------------
	  // ---- set border voxels to fillValue
	  if (fillValue >= 0)
	    {
	      uchar *v = extData;
	      
	      i0 = 0;
	      for(int i=d0z-nextra; i<=d1z+nextra; i++)
		{
		  int iv = qBound(0, i, m_depth-1);
		  int i0dx = i0*m_nY*m_nZ;
		  if (iv <= qRound(m_dataMin.z) || iv >= qRound(m_dataMax.z))
		    {
		      if (m_voxelType == 0)
			memset(v + i0dx, fillValue, m_nY*m_nZ);
		      else
			{
			  for(int fi=0; fi<m_nY*m_nZ; fi++)
			    ((ushort*)v)[i0*m_nY*m_nZ + fi] = fillValue;
			}
		    }
		  else
		    {
		      if (m_voxelType == 0)
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
	  
	  m_meshLog->moveCursor(QTextCursor::End);
	  m_meshLog->insertPlainText("  Generating Color ...\n");
	  
	  for(int ni=0; ni<m_nverts; ni++)
	    {
	      m_meshProgress->setValue((int)(100.0*(float)ni/(float)m_nverts));
	      qApp->processEvents();
	      
	      float v[3];
	      v[0] = m_vlist[ni]->x/voxelScaling.x/m_scaleModel;
	      v[1] = m_vlist[ni]->y/voxelScaling.y/m_scaleModel;
	      v[2] = m_vlist[ni]->z/voxelScaling.z/m_scaleModel;

	      if (v[2] > d0 && v[2] <= d1)
		{
//		  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5 %6\n%7 %8"). \
//					   arg(v[0]).arg(v[1]).arg(v[2]). \
//					   arg(m_vlist[ni]->x).arg(m_vlist[ni]->y).arg(m_vlist[ni]->z). \
//					   arg(d0).arg(d1));
		  		  
		  uchar *volData = extData;
		  QColor col;
		  QVector3D pos, normal;
		  pos = QVector3D(v[0], v[1], v[2]-d0 + nextra);
		  normal = QVector3D(-m_vlist[ni]->nx,
				     -m_vlist[ni]->ny,
				     -m_vlist[ni]->nz);
		  col = getVRLutColor(volData,
				      dlen,
				      depth, nextra,
				      pos, normal,
				      lut,
				      lookInside,
				      QVector3D(v[0], v[1], v[2]));
		      
		  if (col.alphaF() > 0)
		    {
		      float r = col.red()/255.0f;
		      float g = col.green()/255.0f;
		      float b = col.blue()/255.0f;
		      float a = col.alphaF();

		      // tinge with lutcolor
		      QColor col0 = vstops[255*(float)(volnum+1)/(float)nvols].second;
		      float aa = col0.alphaF();
		      float tr = col0.red()/255.0f;
		      float tg = col0.green()/255.0f;
		      float tb = col0.blue()/255.0f;
		      
		      r = (1.0-aa)*r + aa*a*tr;
		      g = (1.0-aa)*g + aa*a*tg;
		      b = (1.0-aa)*b + aa*a*tb;

		      vcolor[3*ni+0] = (1-a)*vcolor[3*ni+0] + r;
		      vcolor[3*ni+1] = (1-a)*vcolor[3*ni+1] + g;
		      vcolor[3*ni+2] = (1-a)*vcolor[3*ni+2] + b;
		    }
		}
	    }
	  m_meshProgress->setValue(100);
	  
	  delete [] extData;
	} // loop over slabs

//      if (nvols > 1) // save intermediate files
//	{
//	  QString plyflnm = flnm;
//	  plyflnm.chop(3);
//	  plyflnm += QString("%1.ply").arg((int)volnum, (int)nvols/10+2, 10, QChar('0'));
//
//	  for(int ni=0; ni<m_nverts; ni++)
//	    {
//	      m_vlist[ni]->r = 255*vcolor[3*ni+0];
//	      m_vlist[ni]->g = 255*vcolor[3*ni+1];
//	      m_vlist[ni]->b = 255*vcolor[3*ni+2];
//	    }
//	  savePLY(plyflnm);
//	}

    }// loop over files

  for(int ni=0; ni<m_nverts; ni++)
    {
      m_vlist[ni]->r = 255*vcolor[3*ni+0];
      m_vlist[ni]->g = 255*vcolor[3*ni+1];
      m_vlist[ni]->b = 255*vcolor[3*ni+2];
    }

  savePLY(flnm);

  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Mesh saved in "+flnm);

  QMessageBox::information(0, "", QString("Mesh saved in "+flnm));


}
void
MeshGenerator::savePLY(QString flnm)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Saving Mesh");

  PlyProperty vert_props[]  = { /* list of property information for a PlyVertex */
    {"x", Float32, Float32,  offsetof( Vertex,x ), 0, 0, 0, 0},
    {"y", Float32, Float32,  offsetof( Vertex,y ), 0, 0, 0, 0},
    {"z", Float32, Float32,  offsetof( Vertex,z ), 0, 0, 0, 0},
    {"nx", Float32, Float32, offsetof( Vertex,nx ), 0, 0, 0, 0},
    {"ny", Float32, Float32, offsetof( Vertex,ny ), 0, 0, 0, 0},
    {"nz", Float32, Float32, offsetof( Vertex,nz ), 0, 0, 0, 0},
    {"red", Uint8, Uint8,    offsetof( Vertex,r ), 0, 0, 0, 0},
    {"green", Uint8, Uint8,  offsetof( Vertex,g ), 0, 0, 0, 0},
    {"blue", Uint8, Uint8,   offsetof( Vertex,b ), 0, 0, 0, 0}
  };

  PlyProperty face_props[]  = { /* list of property information for a PlyFace */
    {"vertex_indices", Int32, Int32, offsetof( Face,verts ),
      1, Uint8, Uint8, offsetof( Face,nverts )},
  };


  PlyFile    *ply;
  FILE    *fp = fopen(flnm.toAscii().data(), "wb");

  Face     face ;
  int      verts[3] ;
  char    *elem_names[]  = { "vertex", "face" };
  ply = write_ply (fp,
		   2,
		   elem_names,
		   PLY_BINARY_LE );

  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, "vertex", m_nverts );
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
  describe_element_ply ( ply, "face", m_nfaces );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );

  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, "vertex" );
  for(int ni=0; ni<m_nverts; ni++)
    put_element_ply ( ply, ( void * ) m_vlist[ni] );

  /* set up and write the PlyFace elements */
  put_element_setup_ply ( ply, "face" );
  for(int ni=0; ni<m_nfaces; ni++)
    put_element_ply ( ply, ( void * ) m_flist[ni] );

  close_ply ( ply );
  free_ply ( ply );

  m_meshProgress->setValue(100);
}

bool
MeshGenerator::loadPLY(QString flnm)
{
  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {"x", Float32, Float32, offsetof(Vertex,x), 0, 0, 0, 0},
    {"y", Float32, Float32, offsetof(Vertex,y), 0, 0, 0, 0},
    {"z", Float32, Float32, offsetof(Vertex,z), 0, 0, 0, 0},
    {"nx", Float32, Float32, offsetof(Vertex,nx), 0, 0, 0, 0},
    {"ny", Float32, Float32, offsetof(Vertex,ny), 0, 0, 0, 0},
    {"nz", Float32, Float32, offsetof(Vertex,nz), 0, 0, 0, 0},
    {"red", Uint8, Uint8, offsetof(Vertex,r), 0, 0, 0, 0},
    {"green", Uint8, Uint8, offsetof(Vertex,g), 0, 0, 0, 0},
    {"blue", Uint8, Uint8, offsetof(Vertex,b), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {"vertex_indices", Int32, Int32, offsetof(Face,verts),
     1, Uint8, Uint8, offsetof(Face,nverts)},
  };


  /*** the PLY object ***/
  PlyOtherProp *vert_other,*face_other;

  bool per_vertex_color = false;
  bool has_normals = false;

  int i,j;
  int elem_count;
  char *elem_name;
  PlyFile *in_ply;


  /*** Read in the original PLY object ***/
  FILE *fp = fopen(flnm.toAscii().data(), "rb");

  in_ply  = read_ply (fp);

  for (i = 0; i < in_ply->num_elem_types; i++) {

    /* prepare to read the i'th list of elements */
    elem_name = setup_element_read_ply (in_ply, i, &elem_count);


    if (equal_strings ("vertex", elem_name)) {

      /* create a vertex list to hold all the vertices */
      m_vlist = (Vertex **) malloc (sizeof (Vertex *) * elem_count);
      m_nverts = elem_count;

      /* set up for getting vertex elements */

      setup_property_ply (in_ply, &vert_props[0]);
      setup_property_ply (in_ply, &vert_props[1]);
      setup_property_ply (in_ply, &vert_props[2]);

      for (j = 0; j < in_ply->elems[i]->nprops; j++) {
	PlyProperty *prop;
	prop = in_ply->elems[i]->props[j];
	if (equal_strings ("r", prop->name) ||
	    equal_strings ("red", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[3]);
	  per_vertex_color = true;
	}
	if (equal_strings ("g", prop->name) ||
	    equal_strings ("green", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[4]);
	  per_vertex_color = true;
	}
	if (equal_strings ("b", prop->name) ||
	    equal_strings ("blue", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[5]);
	  per_vertex_color = true;
	}
	if (equal_strings ("nx", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[6]);
	  has_normals = true;
	}
	if (equal_strings ("ny", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[7]);
	  has_normals = true;
	}
	if (equal_strings ("nz", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[8]);
	  has_normals = true;
	}
      }

      /* grab all the vertex elements */
      for (j = 0; j < elem_count; j++) {
        m_vlist[j] = (Vertex *) malloc (sizeof (Vertex));
        get_element_ply (in_ply, (void *) m_vlist[j]);
      }
    }
    else if (equal_strings ("face", elem_name)) {

      /* create a list to hold all the face elements */
      m_flist = (Face **) malloc (sizeof (Face *) * elem_count);
      m_nfaces = elem_count;

      /* set up for getting face elements */

      setup_property_ply (in_ply, &face_props[0]);

      /* grab all the face elements */
      for (j = 0; j < elem_count; j++) {
        m_flist[j] = (Face *) malloc (sizeof (Face));
        get_element_ply (in_ply, (void *) m_flist[j]);
      }
    }
    else
      get_other_element_ply (in_ply);
  }

  close_ply (in_ply);
  free_ply (in_ply);
  

  vcolor = new float[m_nverts*3];
  for(int i=0; i<m_nverts; i++)
    {
      vcolor[3*i+0] = m_vlist[i]->r/255.0f;
      vcolor[3*i+1] = m_vlist[i]->g/255.0f;
      vcolor[3*i+2] = m_vlist[i]->b/255.0f;
    }

  QMessageBox::information(0, "", QString("Loaded input mesh : vertices %1  faces %2").\
			   arg(m_nverts).\
			   arg(m_nfaces));

  return true;
}

