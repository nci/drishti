#include "vdbvolume.h"

#include "staticfunctions.h"
#include "meshgenerator.h"
#include "meshtools.h"

#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QSaveFile>

#include <memory>
#include <limits>
#include <stdexcept>

namespace
{
bool writeAll(QIODevice& device, const char *data, qint64 bytes)
{
  qint64 written = 0;
  while (written < bytes)
    {
      const qint64 count = device.write(data+written, bytes-written);
      if (count <= 0)
        return false;
      written += count;
    }
  return true;
}

bool checkedMultiply(qint64 left, qint64 right, qint64& result)
{
  if (left < 0 || right < 0 ||
      (right != 0 && left > std::numeric_limits<qint64>::max()/right))
    return false;
  result = left*right;
  return true;
}
}


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
			 int &smoothOpacity,
			 QGradientStops &stops,
			 int &chan,
			 bool &avgColor,
			 float &adaptivity,
			 bool tetMesh)
{  
  chan = 0;
  isoval = 128;
  isovalf = 0.5;
  spread = 0;
  depth = 2;
  useColor = 1;
  smoothOpacity = 1;
  avgColor = true;
  adaptivity = 0.1;
  m_useTagColors = false;
  m_scaleModel = 1.0;
  QGradientStops vstops;
  vstops << QGradientStop(0.0, Qt::lightGray)
	 << QGradientStop(1.0, Qt::lightGray);


  if (m_batchMode)
    return true;
  
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  if (m_voxelType == 0)
    {
      vlist.clear();
      vlist << QVariant("int");
      vlist << QVariant(smoothOpacity);
      vlist << QVariant(0);
      vlist << QVariant(10);
      plist["smooth opacity"] = vlist;

      if (!tetMesh)
	{
	  vlist.clear();
	  vlist << QVariant("checkbox");
	  vlist << QVariant(avgColor);
	  plist["average color"] = vlist;
	}
    }

  if (!tetMesh)
    {
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(m_useTagColors);
      plist["apply tag colors"] = vlist;
    }

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

  if (!tetMesh)
    {
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
    }
  
  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(m_scaleModel);
  vlist << QVariant(0.001);
  vlist << QVariant(1.0);
  vlist << QVariant(0.005); // singlestep
  vlist << QVariant(3); // decimals
  plist["scale"] = vlist;

  if (!tetMesh)
    {
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
    }

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
  if (!tetMesh)
    keys << "average color";
  keys << "apply tag colors";
  keys << "mop channel";
  keys << "isosurface value";
  if (!tetMesh)
    {
      keys << "adaptivity";
      keys << "mesh smoothing";
      keys << "depth";
    }
  keys << "scale";
  if (!tetMesh)
    {
      keys << "color";
      keys << "color type";
    }
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
	  if (keys[ik] == "smooth opacity")
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
	  else if (keys[ik] == "color type")
	    {
	      useColor = pair.first.toInt();
	      useColor = qBound(0, useColor, 1);
	    }
	}
    }

  isoval = qRound(isovalf*255);
    
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

  
  m_meshLog = new QTextEdit;
  m_meshProgress = new QProgressBar;

  QVBoxLayout *meshLayout = new QVBoxLayout;
  meshLayout->addWidget(m_meshLog);
  meshLayout->addWidget(m_meshProgress);

  QWidget *meshWindow = new QWidget;
  meshWindow->setWindowTitle("Drishti - Mesh Generation Using Opacity Values");
  meshWindow->setLayout(meshLayout);
  meshWindow->show();
  meshWindow->resize(700, 300);


  //----------------------------
  //---- export the grid ---
  QString flnm;
  if (!m_batchMode)
    flnm = QFileDialog::getSaveFileName(0,
					"Export mesh to file",
					prevDir,
					"Surface Mesh (*.ply *.obj *.stl) ;; Tetrahedral Mesh (*.msh)");
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
      !StaticFunctions::checkExtension(flnm, ".stl") &&
      !StaticFunctions::checkExtension(flnm, ".msh"))
    flnm += ".ply";

  bool tetMesh = false;
  if (StaticFunctions::checkExtension(flnm, ".msh"))
    tetMesh = true;    
  //----------------------------



  //----------------------------
  float isovalf;
  int isoval, spread, depth, useColor;
  int smoothOpacity;
  QGradientStops stops;
  int chan;
  bool avgColor;
  float adaptivity;
  if (! getValues(isoval, isovalf,
		  spread, depth, useColor,
		  smoothOpacity,
		  stops,
		  chan,
		  avgColor,
		  adaptivity,
		  tetMesh))
    return "";
  //----------------------------



  float memGb = 8.0;
  if (!m_batchMode)
    memGb = QInputDialog::getDouble(0,
				    "Use memory",
				    "Max memory we can use (GB)", memGb, 0.1, 1000, 2);
  else
    memGb = 8.0; // for batch mode assume 5GB
  

  qint64 gb = 1024*1024*1024;
  qint64 memsize = memGb*gb; // max memory we can use (in GB)

  if (m_nX <= 0 || m_nY <= 0 || m_nZ <= 0 || memsize <= 0)
    {
      QMessageBox::critical(0, "Mesh generation failed",
                            "The selected mesh grid or memory budget is invalid.");
      meshWindow->close();
      return QString();
    }

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
  


  qint64 planeVoxels = 0;
  qint64 volumeVoxels = 0;
  qint64 reqmem = 0;
  if (!checkedMultiply(m_nY, m_nZ, planeVoxels) ||
      !checkedMultiply(m_nX, planeVoxels, volumeVoxels) ||
      !checkedMultiply(volumeVoxels, 11, reqmem))
    {
      QMessageBox::critical(0, "Mesh generation failed",
                            "The mesh memory estimate overflows the supported range.");
      meshWindow->close();
      return QString();
    }
  const qint64 requestedSlabs = reqmem/memsize +
                                (reqmem%memsize == 0 ? 0 : 1);
  const int nSlabs = static_cast<int>(
    qBound<qint64>(1, requestedSlabs, m_nX));
//  QMessageBox::information(0, "", QString("Number of Slabs : %1 : %2 %3").\
//			   arg(nSlabs).arg(reqmem).arg(memsize));

  const bool outputExisted = QFileInfo::exists(flnm);
  QStringList preexistingSlabArtifacts;
  for (int slab=0; slab<nSlabs; ++slab)
    {
      const QString triFile = flnm + QString(".%1.tri").arg(slab);
      const QString vertFile = flnm + QString(".%1.vert").arg(slab);
      if (QFileInfo::exists(triFile)) preexistingSlabArtifacts << triFile;
      if (QFileInfo::exists(vertFile)) preexistingSlabArtifacts << vertFile;
    }
  if (!preexistingSlabArtifacts.isEmpty())
    {
      QMessageBox::critical(0, "Mesh generation failed",
        "Temporary mesh files already exist beside the selected output. "
        "Move or remove those .tri/.vert files before retrying so they are "
        "not overwritten.");
      meshWindow->close();
      return QString();
    }

  bool succeeded = false;
  try
    {
      succeeded = generateMesh(nSlabs,
		   isoval,
		   flnm,
		   depth, spread,
		   stops,
		   useColor,
		   voxelScaling,
		   clipPos, clipNormal,
		   crops, paths,
		   smoothOpacity, lut,
		   chan,
		   avgColor,
		   adaptivity,
		   tetMesh);
      if (!succeeded)
        QMessageBox::critical(0, "Mesh generation failed",
                              "The mesh output or a temporary slab file "
                              "could not be written completely.");
    }
  catch (const std::exception& error)
    {
      QMessageBox::critical(0, "Mesh generation failed",
                            QString::fromLocal8Bit(error.what()));
    }
  catch (...)
    {
      QMessageBox::critical(0, "Mesh generation failed",
                            "Allocation or mesh processing failed.");
    }

  if (!succeeded)
    {
      if (!outputExisted)
        QFile::remove(flnm);
      for (int slab=0; slab<nSlabs; ++slab)
        {
          const QString triFile = flnm + QString(".%1.tri").arg(slab);
          const QString vertFile = flnm + QString(".%1.vert").arg(slab);
          if (!preexistingSlabArtifacts.contains(triFile)) QFile::remove(triFile);
          if (!preexistingSlabArtifacts.contains(vertFile)) QFile::remove(vertFile);
        }
      flnm.clear();
    }


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

	      gr = qMin(255, int(qSqrt(a*a+b*b+c*c)));
	      //gr = 0;
	    }
	  else
	    {
	      v = ((ushort*)volData)[k*m_nY*m_nZ + j*m_nZ + i];
	      gr = v%255;
	      v = v/255;
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
		int a = tmp1[qMin(m_nY-1,j+1)*m_nZ + k] -
		        tmp1[qMax(0, j-1)*m_nZ + k];

		int b = tmp1[j*m_nZ + qMin(m_nZ-1,k+1)] -
		        tmp1[j*m_nZ + qMax(0, k-1)];

		int c = tmp2[j*m_nZ + k] -
		        tmp0[j*m_nZ + k];

		uchar gr = qMin(255, int(qSqrt(a*a + b*b + c*c)));
		  
		opac = mop*lut[tfSet + 4*(256*gr + v) + 3];

		// ignoring gradient to calculate opacity
		//opac = mop*lut[tfSet + 4*v + 3];
	      }
	    else
	      {
		int a = v%255;
		int b = v/255;
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
		  a = v%255;
		  b = v/255;
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
		  a = v%255;
		  b = v/255;
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

bool
MeshGenerator::generateMesh(int nSlabs, int isoval,
			    QString flnm,
			    int depth, int spread,
			    QGradientStops vstops,
			    int useColor,
			    Vec voxelScaling,
			    QList<Vec> clipPos,
			    QList<Vec> clipNormal,
			    QList<CropObject> crops,
			    QList<PathObject> paths,
			    int smoothOpacity, 
			    uchar *lut,
			    int chan, bool avgColor, float adaptivity,
			    bool tetMesh)
{

  VdbVolume vdb;

  int saveType = 0; // default .ply
  if (StaticFunctions::checkExtension(flnm, ".obj")) saveType = 1;
  if (StaticFunctions::checkExtension(flnm, ".stl")) saveType = 2;

  bool saveIntermediate = false;

  int bpv = 1;
  if (m_voxelType > 0) bpv = 2;
  if (nSlabs <= 0 || m_nX <= 0 || m_nY <= 0 || m_nZ <= 0)
    return false;

  qint64 planeVoxels = 0;
  qint64 planeBytes = 0;
  if (!checkedMultiply(m_nY, m_nZ, planeVoxels) ||
      !checkedMultiply(planeVoxels, bpv, planeBytes) ||
      planeVoxels > std::numeric_limits<int>::max() ||
      static_cast<quint64>(planeBytes) >
        static_cast<quint64>(std::numeric_limits<std::size_t>::max()))
    {
      qWarning() << "Mesh slice size exceeds the supported address range";
      return false;
    }
  const std::size_t nbytes = static_cast<std::size_t>(planeBytes);

  
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

  int ntriangles = 0;
  int nvertices = 0;
  for (int nb=0; nb<nSlabs; nb++)
    {
      m_meshLog->moveCursor(QTextCursor::End);
      m_meshLog->insertPlainText(QString("Processing slab %1 of %2\n").arg(nb+1).arg(nSlabs));

      const int d0 = static_cast<int>(
        static_cast<qint64>(nb)*m_nX/nSlabs);
      const int d1 = qMin(m_nX-1, static_cast<int>(
        static_cast<qint64>(nb+1)*m_nX/nSlabs));
      int dlen = d1-d0+1;
      
      int d0z = d0 + qRound(m_dataMin.z);
      int d1z = d1 + qRound(m_dataMin.z);

      qint64 dataSize = 0;
      qint64 rawDataBytes = 0;
      if (dlen <= 0 ||
          !checkedMultiply(planeVoxels, dlen, dataSize) ||
          !checkedMultiply(dataSize, bpv, rawDataBytes) ||
          static_cast<quint64>(dataSize) >
            static_cast<quint64>(std::numeric_limits<std::size_t>::max()) ||
          static_cast<quint64>(rawDataBytes) >
            static_cast<quint64>(std::numeric_limits<std::size_t>::max()))
        {
          qWarning() << "Mesh slab size exceeds the supported address range";
          return false;
        }
      const std::size_t dataBytes = static_cast<std::size_t>(dataSize);
      const std::size_t sourceDataBytes =
        static_cast<std::size_t>(rawDataBytes);
      std::unique_ptr<uchar[]> extDataOwner(
        new uchar[sourceDataBytes]);
      std::unique_ptr<uchar[]> gDataOwner(new uchar[dataBytes]);
      std::unique_ptr<uchar[]> croppedOwner(new uchar[nbytes]);
      std::unique_ptr<uchar[]> tmpOwner(new uchar[nbytes]);
      std::unique_ptr<uchar[]> tmp0Owner(new uchar[nbytes]);
      std::unique_ptr<uchar[]> tmp1Owner(new uchar[nbytes]);
      std::unique_ptr<uchar[]> tmp2Owner(new uchar[nbytes]);
      uchar *extData = extDataOwner.get();
      uchar *gData = gDataOwner.get();
      uchar *cropped = croppedOwner.get();
      uchar *tmp = tmpOwner.get();
      uchar *tmp0 = tmp0Owner.get();
      uchar *tmp1 = tmp1Owner.get();
      uchar *tmp2 = tmp2Owner.get();
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
	  if (!vslice)
	    {
	      const QString detail = m_vfm->lastError();
	      throw std::runtime_error(
	          QString("Cannot read source slice %1: %2")
	          .arg(iv).arg(detail).toStdString());
	    }

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
	      for(qint64 j=0; j<planeVoxels; j++)
		{
		  if (cropped[j] == 0)
		    tmp[j] = 0;
		}
	    }
	  else
	    {
	      for(qint64 j=0; j<planeVoxels; j++)
		{
		  if (cropped[j] == 0)
		    ((ushort*)tmp)[j] = 0;
		}
	    }

	  // tmp now clipped and contains raw data
	  memcpy(extData + bpv*i0*planeVoxels, tmp, nbytes);

	  // apply opacity
	  {
	    memcpy(tmp0, tmp1, nbytes);
	    memcpy(tmp1, tmp2, nbytes);
	    memcpy(tmp2, tmp, nbytes);

	    if (i0 == 1)
	      {
		applyOpacity(iv, cropped, lut, tmp,
			     tmp1, tmp2, tmp2); // i0 == 0
		memcpy(gData + i0*planeVoxels, tmp,
		       static_cast<std::size_t>(planeVoxels));
	      }
	    if (i0 > 1)
	      {
		applyOpacity(iv, cropped, lut, tmp,
			     tmp0, tmp1, tmp2); // i0 == 1
		memcpy(gData + i0*planeVoxels, tmp,
		       static_cast<std::size_t>(planeVoxels));
		if (i0 == i0end)
		  {
		    applyOpacity(iv, cropped, lut, tmp,
				 tmp1, tmp2, tmp2); // i0 == 1
		    memcpy(gData + i0*planeVoxels, tmp,
		           static_cast<std::size_t>(planeVoxels));
		  }
	      }
	    }

	  i0++;
	} // i loop
      
      m_meshProgress->setValue(100);
      qApp->processEvents();

      //------------
      if (m_tearPresent)
	{
	  std::unique_ptr<uchar[]> data0Owner(new uchar[dataBytes]);
	  uchar *data0 = data0Owner.get();

	  // applyTear once for coloring volume
	  uchar *data1 = extData;
	  memcpy(data0, data1, dataBytes);
	  applyTear(d0, d1,
		    data0, data1, true);

	  // applyTear once for opacity volume
	  data1 = gData;
	  memcpy(data0, data1, dataBytes);
	  applyTear(d0, d1,
		    data0, data1, false);

	}
      //------------


      //--------------------------------
      //--------------------------------
      m_meshLog->moveCursor(QTextCursor::End);
      m_meshLog->insertPlainText("Generating VDB ...\n");
      vdb.generateVDB(gData,
		      dlen, m_nY, m_nZ,
		      -1, 1, 0, // values less than 1 are background
		      m_meshProgress);

      
      // convert to levelset
      vdb.convertToLevelSet(128, 0);
      
      if (smoothOpacity > 0)
	{
	  m_meshLog->moveCursor(QTextCursor::End);
	  m_meshLog->insertPlainText("Smoothing volume ...\n");
	  vdb.offset(0.1);
	  vdb.gaussian(1, smoothOpacity);
	}
      
      m_meshLog->moveCursor(QTextCursor::End);
      m_meshLog->insertPlainText(QString("Generating mesh with adaptivity %1 ...\n").arg(adaptivity));
      QVector<QVector3D> V;
      QVector<QVector3D> VN;
      QVector<int> T;
      //vdb.generateMesh(0, 3*(128.0-isoval)/255.0, adaptivity, V, VN, T);
      if (!tetMesh)
	vdb.generateMesh(0, 3*(128.0-isoval)/255.0, adaptivity, V, VN, T);
      else // set adaptivity to 0.0 for tetrahedral mesh generation
	vdb.generateMesh(0, 3*(128.0-isoval)/255.0, 0.0, V, VN, T);
      
      int nverts = V.count();
      if (T.count()%3 != 0 || VN.count() != nverts)
        {
          qWarning() << "VDB produced inconsistent mesh arrays";
          return false;
        }
      int ntrigs = T.count()/3;
      if (nvertices > std::numeric_limits<int>::max()-nverts ||
          ntriangles > std::numeric_limits<int>::max()-ntrigs)
        {
          qWarning() << "Combined mesh exceeds the supported index range";
          return false;
        }
	
      //--------------------------------
      //--------------------------------


      // mesh smoothing
      if (spread > 0)
	MeshTools::smoothMesh(V, VN, T, 5*spread);


      // save coordinates and connectivity
      {
	m_meshLog->moveCursor(QTextCursor::End);
	m_meshLog->insertPlainText("Saving triangle coordinates ...\n");
	QString mflnm = flnm + QString(".%1.tri").arg(nb);
	QSaveFile fout(mflnm);
	if (!fout.open(QFile::WriteOnly) ||
	    !writeAll(fout, reinterpret_cast<const char*>(&ntrigs), 4))
	  {
	    qWarning() << "Cannot write triangle slab" << mflnm
	               << fout.errorString();
	    fout.cancelWriting();
	    return false;
	  }
	if (saveType < 2) // save modified triangle vertex numbers only for PLY and OBJ files
	  {
	    for(int ni=0; ni<ntrigs; ni++)
	      {
		int v[3];
		v[0] = T[3*ni+0] + nvertices;
		v[1] = T[3*ni+1] + nvertices;
		v[2] = T[3*ni+2] + nvertices;
		if (!writeAll(fout, reinterpret_cast<const char*>(v), 12))
		  {
		    qWarning() << "Cannot complete triangle slab" << mflnm
		               << fout.errorString();
		    fout.cancelWriting();
		    return false;
		  }
	      }
	  }
	else // saving stl formatted output
	  {
	    for(int ni=0; ni<ntrigs; ni++)
	      {
		int v[3];
		v[0] = T[3*ni+0];
		v[1] = T[3*ni+1];
		v[2] = T[3*ni+2];
		if (!writeAll(fout, reinterpret_cast<const char*>(v), 12))
		  {
		    qWarning() << "Cannot complete triangle slab" << mflnm
		               << fout.errorString();
		    fout.cancelWriting();
		    return false;
		  }
	      }
	  }
	if (fout.error() != QFileDevice::NoError || !fout.commit())
	  {
	    qWarning() << "Cannot commit triangle slab" << mflnm
	               << fout.errorString();
	    fout.cancelWriting();
	    return false;
	  }
	ntriangles += ntrigs;
      }      
  
	{
	QString mflnm = flnm + QString(".%1.vert").arg(nb);
	QSaveFile fout(mflnm);
	if (!fout.open(QFile::WriteOnly) ||
	    !writeAll(fout, reinterpret_cast<const char*>(&nverts), 4))
	  {
	    qWarning() << "Cannot write vertex slab" << mflnm
	               << fout.errorString();
	    fout.cancelWriting();
	    return false;
	  }
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
	    if (!writeAll(fout, reinterpret_cast<const char*>(v), 24))
	      {
		qWarning() << "Cannot complete vertex slab" << mflnm
		           << fout.errorString();
		fout.cancelWriting();
		return false;
	      }
	    
	    if (saveType < 2) // do colour calculations for PLY/OBJ files
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
		if (!writeAll(fout, reinterpret_cast<const char*>(c), 3))
		  {
		    qWarning() << "Cannot complete vertex colors" << mflnm
		               << fout.errorString();
		    fout.cancelWriting();
		    return false;
		  }
	      } // if (saveType == 0)
	  }
	if (fout.error() != QFileDevice::NoError || !fout.commit())
	  {
	    qWarning() << "Cannot commit vertex slab" << mflnm
	               << fout.errorString();
	    fout.cancelWriting();
	    return false;
	  }
	nvertices += nverts;
	m_meshProgress->setValue(100);
      }
      
    } // loop over slabs



  bool ok = true;
  if (tetMesh)
    {
      ok = MeshTools::saveToTetrahedralMesh(flnm,
					    nSlabs,
					    nvertices, ntriangles);
    }
  else
    {
      if (saveType == 0) // PLY
	ok = MeshTools::saveToPLY(flnm,
			          nSlabs,
			          nvertices, ntriangles,
			          true);
      else if (saveType == 1) // OBJ	
	ok = MeshTools::saveToOBJ(flnm,
			          nSlabs,
			          nvertices, ntriangles);
      else // STL
	ok = MeshTools::saveToSTL(flnm,
			          nSlabs,
			          nvertices, ntriangles);
    }
  
  if (ok)
    {
      m_meshLog->moveCursor(QTextCursor::End);
      m_meshLog->insertPlainText("Mesh saved in "+flnm);
      
      if (!m_batchMode)
	{
	  QMessageBox dlg(QMessageBox::Information, "Surface Mesh Saved", QString("Mesh saved in "+flnm));
	  dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint);
	  dlg.exec();
	}
    }

  return ok;
}
