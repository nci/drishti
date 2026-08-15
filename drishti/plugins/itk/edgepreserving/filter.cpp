#include "staticfunctions.h"
#include "filter.h"
#include "../itkmemoryadmission.h"

#include <QSaveFile>

#include <limits>
#include <memory>
#include <stdexcept>
#include "propertyeditor.h"

#include "itkTextOutput.h"
#include "itkGradientAnisotropicDiffusionImageFilter.h"
#include "itkCurvatureAnisotropicDiffusionImageFilter.h"
#include "itkCurvatureFlowImageFilter.h"
#include "itkMinMaxCurvatureFlowImageFilter.h"
#include "itkBilateralImageFilter.h"

#include "itkCommand.h"

void
SmoothingFilter::next()
{
  int p = (m_prog*(100/m_niter))%100;
  m_meshProgress->setValue(p);
  m_prog++;
  qApp->processEvents();
}


SmoothingFilter::SmoothingFilter() {}
SmoothingFilter::~SmoothingFilter() {}

QString
SmoothingFilter::start(VolumeFileManager *vfm,
		       int nX, int nY, int nZ,
		       Vec dataMin, Vec dataMax,
		       QString prevDir,
		       int samplingLevel,
		       QList<Vec> clipPos,
		       QList<Vec> clipNormal,
		       QList<CropObject> crops,
		       QList<PathObject> paths,
		       uchar *lut,
		       int pruneLod, int pruneX, int pruneY, int pruneZ,
		       QVector<uchar> pruneData)
{ 
  itk::OutputWindow::SetInstance(itk::TextOutput::New().GetPointer());


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

  m_meshLog = new QTextEdit;
  m_meshProgress = new QProgressBar;

  QVBoxLayout *meshLayout = new QVBoxLayout;
  meshLayout->addWidget(m_meshLog);
  meshLayout->addWidget(m_meshProgress);

  QWidget *meshWindow = new QWidget;
  meshWindow->setWindowTitle("Drishti - Smoothing Using Voxel Values");
  meshWindow->setLayout(meshLayout);
  meshWindow->show();
  meshWindow->resize(700, 300);

  m_meshLog->insertPlainText("ITK Edge Preserving Smoothing.");

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
//  m_meshLog->insertPlainText(QString("Grid size : %1 %2 %3\n").\
//			   arg(m_nX).arg(m_nY).arg(m_nZ));
  

  int chan = 0; // mop channel

  int filterType = 0;
  bool usePruneData = false;

  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant("combobox");
  vlist << 0;
  vlist << QVariant("gradient anisotropic diffusion");
  vlist << QVariant("curvature anisotropic diffusion");
  vlist << QVariant("curvature flow");
  vlist << QVariant("minmax curvature flow");
  //vlist << QVariant("bilateral");  
  plist["filter type"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(usePruneData);
  plist["prune"] = vlist;

  vlist.clear();
  QString mesg;
  mesg += "File : "+m_vfm->fileName()+"\n";
  int d = m_vfm->depth();
  int w = m_vfm->width();
  int h = m_vfm->height();
  mesg += QString("Volume Size : %1 %2 %3\n").arg(h).arg(w).arg(d);
  mesg += QString("Data Min : %1 %2 %3\n").arg(m_dataMin.x).arg(m_dataMin.y).arg(m_dataMin.z);
  mesg += QString("Data Max : %1 %2 %3\n\n").arg(m_dataMax.x).arg(m_dataMax.y).arg(m_dataMax.z);
  mesg += "Raw values are used for the smoothing operation.\n";

  mesg += "Parameters for filtering operation :\n";
  mesg += "1. TimeStep - this parameter refers to the time step involved in solving the partial differential equation in the algorithm. The timestep should be restricted to be less than 0.0625.\n";
  mesg += "2. NumberOfIterations - this specifies the number of iterations that the solver must perform before returning a solution image. More the number of iterations, more smooth the image would be. A typical number for the number of iteration is 5.\n";
  mesg += "3. Conductance - this parameter controls conductance, which in turn controls the sensitivity of the algorithm in preserving the edges. If the value of the conductance is low, the algorithm preserves the image features to a larger extent. If the value of the conductance is high, the algorithm smooths (or diffuses) the features in the image. Typical values range from 0.5 to 2.0, but in general it will depend on the type of image and the number of iterations.\n";
  mesg += "4. StencilRadius - this parameter specifies the radius of the neighbourhood region of the pixel where the average intesity of the pixels is calculated to make a decision on the speed of the diffusion. Typical value of the radius would be 1. Note that if the value of the radius is large, not much smoothing is performed in the image.\n";

  vlist << mesg;
  plist["message"] = vlist;

  m_meshLog->insertPlainText(mesg);  
  
  QStringList keys;
  keys << "filter type";
  keys << "prune";
  keys << "message";

  propertyEditor.set("Smoothing Parameters", plist, keys);
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
  {  
    vmap = propertyEditor.get();

    for(int ik=0; ik<keys.count(); ik++)
      {
	QPair<QVariant, bool> pair = vmap.value(keys[ik]);
	
	if (pair.second)
	  {
	    if (keys[ik] == "filter type")
	      filterType = pair.first.toInt();
	    else if (keys[ik] == "prune")
	      usePruneData = pair.first.toBool();
	  }
      }
  }
  else
    {
      meshWindow->close();
      return "";
    }

  //----------------------------
  QString flnm = QFileDialog::getSaveFileName(0,
					      "Save filtered data to file",
					      prevDir,
					      "*.raw");
  if (flnm.size() == 0)
    {
      meshWindow->close();
      return "";
    }
  //----------------------------

  bool succeeded = false;
  try
    {
      applyFilter(flnm,
		  clipPos, clipNormal,
		  crops, paths,
		  lut,
		  chan,
		  filterType,
		  usePruneData);
      succeeded = true;
    }
  catch (const std::exception& error)
    {
      QMessageBox::critical(0, "Error",
                            QString("Cannot run the filter.\n%1")
                            .arg(QString::fromLocal8Bit(error.what())));
    }
  catch ( ... )
    {
      QMessageBox::critical(0, "Error", "Cannot run the filter because allocation or filter execution failed.");
    }

  if (!succeeded)
    {
      flnm.clear();
    }


  meshWindow->close();

  return flnm;
}

void
SmoothingFilter::applyFilter(QString flnm,
			     QList<Vec> clipPos,
			     QList<Vec> clipNormal,
			     QList<CropObject> crops,
			     QList<PathObject> paths,
			     uchar *lut,
			     int chan,
			     int filterType,
			     bool usePruneData)
{
  if (m_voxelType != VolumeFileManager::_UChar &&
      m_voxelType != VolumeFileManager::_UShort)
    throw std::runtime_error(
        "Edge-preserving filters support only unsigned 8-bit and 16-bit volumes");
  const int bpv = (m_voxelType == VolumeFileManager::_UShort ? 2 : 1);
  if (m_nX <= 0 || m_nY <= 0 || m_nZ <= 0)
    throw std::runtime_error("Filter volume dimensions must be positive");

  qint64 voxelCount = m_nX;
  if (voxelCount > std::numeric_limits<qint64>::max()/m_nY)
    throw std::runtime_error("Filter volume dimensions overflow addressable memory");
  voxelCount *= m_nY;
  if (voxelCount > std::numeric_limits<qint64>::max()/m_nZ)
    throw std::runtime_error("Filter volume dimensions overflow addressable memory");
  voxelCount *= m_nZ;

  const qint64 planeVoxels = static_cast<qint64>(m_nY)*m_nZ;
  if (planeVoxels > std::numeric_limits<qint64>::max()/bpv ||
      voxelCount > std::numeric_limits<qint64>::max()/bpv)
    throw std::runtime_error("Filter volume dimensions overflow addressable memory");
  const qint64 planeBytes64 = planeVoxels*bpv;
  const qint64 rawBytes64 = voxelCount*bpv;
  if (static_cast<quint64>(planeBytes64) > std::numeric_limits<size_t>::max() ||
      static_cast<quint64>(rawBytes64) > std::numeric_limits<size_t>::max())
    throw std::runtime_error("Filter volume exceeds addressable memory");

  const size_t nbytes = static_cast<size_t>(planeBytes64);

  if (m_width <= 0 || m_height <= 0)
    throw std::runtime_error("Source slice dimensions must be positive");
  const qint64 sourcePlaneVoxels = static_cast<qint64>(m_width)*m_height;
  if (sourcePlaneVoxels > std::numeric_limits<qint64>::max()/bpv)
    throw std::runtime_error("Source slice dimensions overflow addressable memory");
  const qint64 sourcePlaneBytes64 = sourcePlaneVoxels*bpv;
  if (planeBytes64 > sourcePlaneBytes64 ||
      static_cast<quint64>(sourcePlaneBytes64) >
      std::numeric_limits<size_t>::max())
    throw std::runtime_error("Source slice is smaller than the requested filter plane");

  const auto checkedPlaneIndex = [=](int row, int column) -> size_t
    {
      if (row < 0 || row >= m_nY || column < 0 || column >= m_nZ)
        throw std::runtime_error("Filter plane index is out of bounds");
      const qint64 index = static_cast<qint64>(row)*m_nZ + column;
      if (index < 0 || index >= planeVoxels)
        throw std::runtime_error("Filter plane index exceeds its buffer");
      return static_cast<size_t>(index);
    };
  const auto checkedSourceIndex = [=](qint64 row, qint64 column) -> size_t
    {
      if (row < 0 || row >= m_width || column < 0 || column >= m_height)
        throw std::runtime_error("Requested source voxel is outside its slice");
      const qint64 index = row*m_height + column;
      if (index < 0 || index >= sourcePlaneVoxels)
        throw std::runtime_error("Source voxel index exceeds its slice buffer");
      return static_cast<size_t>(index);
    };

  qint64 pruneVoxelCount = 0;
  if (usePruneData)
    {
      if (m_pruneX <= 0 || m_pruneY <= 0 || m_pruneZ <= 0 ||
          m_pruneLod <= 0 || chan < 0 || chan >= 3)
        throw std::runtime_error("Prune data geometry or channel is invalid");
      const qint64 prunePlane = static_cast<qint64>(m_pruneY)*m_pruneX;
      if (prunePlane > std::numeric_limits<qint64>::max()/m_pruneZ)
        throw std::runtime_error("Prune data dimensions overflow addressable memory");
      pruneVoxelCount = prunePlane*m_pruneZ;
      if (pruneVoxelCount > std::numeric_limits<qint64>::max()/3 ||
          3*pruneVoxelCount > static_cast<qint64>(m_pruneData.size()))
        throw std::runtime_error("Prune data buffer is smaller than its geometry");
    }

  requireItkMemoryAdmission(
    ItkMemoryWorkload::EdgePreserving,
    static_cast<std::uint64_t>(m_nX),
    static_cast<std::uint64_t>(m_nY),
    static_cast<std::uint64_t>(m_nZ));

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

  m_meshLog->moveCursor(QTextCursor::End);
  int d0 = 0;
  int d1 = m_nX-1;
  int d0z = d0 + qRound(m_dataMin.z);
  int d1z = d1 + qRound(m_dataMin.z);

  std::unique_ptr<uchar[]> rawVolOwner(
      new uchar[static_cast<size_t>(rawBytes64)]);
  uchar *rawVol = rawVolOwner.get();

  std::unique_ptr<uchar[]> tmpOwner(new uchar[nbytes]);
  uchar *tmp = tmpOwner.get();

  int i0 = 0;
  for(int i=d0z; i<=d1z; i++)
    {
      m_meshProgress->setValue((int)(100.0*(float)i0/(float)m_nX));
      qApp->processEvents();

      int iv = qBound(0, i, m_depth-1);
      uchar *vslice = m_vfm->getSlice(iv);
      if (!vslice)
        throw std::runtime_error(
            QString("Cannot read source slice %1: %2")
            .arg(iv).arg(m_vfm->lastError()).toStdString());

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
		  {
		    const size_t targetIndex = checkedPlaneIndex(w, h);
		    const size_t sourceIndex = checkedSourceIndex(
		        static_cast<qint64>(wmin)+w,
		        static_cast<qint64>(hmin)+h);
		    tmp[targetIndex] = vslice[sourceIndex];
		  }
	    }
	  else
	    {
	      for(int w=0; w<m_nY; w++)
		for(int h=0; h<m_nZ; h++)
		  {
		    const size_t targetIndex = checkedPlaneIndex(w, h);
		    const size_t sourceIndex = checkedSourceIndex(
		        static_cast<qint64>(wmin)+w,
		        static_cast<qint64>(hmin)+h);
		    reinterpret_cast<ushort*>(tmp)[targetIndex] =
		        reinterpret_cast<const ushort*>(vslice)[sourceIndex];
		  }
	    }
	}

      if (usePruneData)
	{
	  for(int j=0; j<m_nY; j++)
	    for(int k=0; k<m_nZ; k++)
	      {
		const size_t planeIndex = checkedPlaneIndex(j, k);
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
		  const qint64 mopidx =
		      (static_cast<qint64>(ppk)*m_pruneY + ppj)*m_pruneX + ppi;
		  const qint64 mopOffset = 3*mopidx + chan;
		  if (mopidx < 0 || mopidx >= pruneVoxelCount ||
		      mopOffset < 0 ||
		      mopOffset >= static_cast<qint64>(m_pruneData.size()) ||
		      mopOffset > std::numeric_limits<int>::max())
		    throw std::runtime_error("Prune data index is out of bounds");
		  mop = m_pruneData.at(static_cast<int>(mopOffset));
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
		      v = tmp[planeIndex];
		    else
		      v = reinterpret_cast<const ushort*>(tmp)[planeIndex];
		    ok = checkBlend(po, v, lut);
		  }
		
		if (ok && m_pathBlendPresent)
		  {
		    ushort v;
		    if (m_voxelType == 0)
		      v = tmp[planeIndex];
		    else
		      v = reinterpret_cast<const ushort*>(tmp)[planeIndex];
		    ok = checkPathBlend(po, v, lut);
		  }
		
		if (!ok)
		  {
		    if (m_voxelType == 0)
		      tmp[planeIndex] = 0;
		    else
		      reinterpret_cast<ushort*>(tmp)[planeIndex] = 0;
		  }
	      }
	}

      memcpy(rawVol + static_cast<size_t>(i0)*nbytes, tmp, nbytes);
      
      i0++;
    }
  m_meshProgress->setValue(100);
  qApp->processEvents();

 
  float *floatVol = 0;
  std::unique_ptr<float[]> floatVolOwner;
  if (filterType < 4)
    {
      if (static_cast<quint64>(voxelCount) >
          std::numeric_limits<size_t>::max()/sizeof(float))
        throw std::runtime_error("Filtered float volume exceeds addressable memory");
      floatVolOwner.reset(new float[static_cast<size_t>(voxelCount)]);
      floatVol = floatVolOwner.get();

      if (bpv == 1)
        {
          for(qint64 i=0; i<voxelCount; i++)
            floatVol[i] = static_cast<float>(rawVol[i])/255.0f;
        }
      else
        {
          const ushort *rawVol16 = reinterpret_cast<const ushort*>(rawVol);
          for(qint64 i=0; i<voxelCount; i++)
            floatVol[i] = static_cast<float>(rawVol16[i])/65535.0f;
        }

      if (filterType == 0)
	GradientAnisotropicDiffusionFilter(floatVol); 
      else if (filterType == 1)
	CurvatureAnisotropicDiffusionFilter(floatVol); 
      else if (filterType == 2)
	CurvatureFlowFilter(floatVol);
      else if (filterType == 3)
	MinMaxCurvatureFlowFilter(floatVol);

      m_meshLog->insertPlainText(" done.\n");

      for(qint64 i=0; i<voxelCount; i++)
        rawVol[i] = static_cast<uchar>(
            qBound(0.0f, floatVol[i]*255.0f, 255.0f));

    }      
//  else if (filterType == 4)
//    BilateralFilter(rawVol);


  QSaveFile fp(flnm);
  fp.setDirectWriteFallback(false);
  if (!fp.open(QFile::WriteOnly))
    throw std::runtime_error(
        QString("Cannot open output file: %1").arg(fp.errorString()).toStdString());
  uchar vt = 0;
  if (fp.write((char*)&vt, 1) != 1 ||
      fp.write((char*)&m_nX, 4) != 4 ||
      fp.write((char*)&m_nY, 4) != 4 ||
      fp.write((char*)&m_nZ, 4) != 4 ||
      fp.write((char*)rawVol, voxelCount) != voxelCount)
    {
      const QString detail = fp.errorString();
      fp.cancelWriting();
      throw std::runtime_error(
          QString("Cannot write complete output volume: %1")
          .arg(detail).toStdString());
    }
  if (!fp.commit())
    throw std::runtime_error(
        QString("Cannot commit output volume: %1")
        .arg(fp.errorString()).toStdString());

  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("SmoothingFilter data saved in "+flnm);

  QMessageBox::information(0, "", QString("SmoothingFilter data saved in "+flnm));

  savePvl(flnm);
}


void
SmoothingFilter::savePvl(QString flnm)
{
  bool ok = false;
  QStringList slevels;
  slevels << "Yes";  
  slevels << "No";
  QString option = QInputDialog::getItem(0,
		   "Save pvl.nc",
		   QString("Save .pvl.nc file (containing reference to %1) ?").arg(flnm),
		    slevels,
			  0,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "Yes")
	{
	  QFileInfo fi(flnm);
	  QString pvlflnm = QFileDialog::getSaveFileName(0,
					      "Save to .pvl.nc file",
					      fi.absolutePath(),
					      "*.pvl.nc");
	  if (pvlflnm.size() == 0)
	    return;

	  QSaveFile fp(pvlflnm);
	  fp.setDirectWriteFallback(false);
	  if (!fp.open(QFile::WriteOnly))
	    {
	      QMessageBox::warning(
	          0, "PVL sidecar not saved",
	          QString("The filtered volume was saved, but the PVL sidecar could not be opened.\n%1")
	          .arg(fp.errorString()));
	      return;
	    }

	  QDir pdir(QFileInfo(pvlflnm).absolutePath());
	  QTextStream out(&fp);
	  out << "<!DOCTYPE Drishti_Header>\n";
	  out << "<PvlDotNcFileHeader>\n";
	  out << "  <pvlnames><name>" << pdir.relativeFilePath(flnm) << "</name></pvlnames>\n";
	  out << "  <pvlheadersize>13</pvlheadersize>\n";
	  out << "  <rawheadersize>13</rawheadersize>\n";
	  out << "  <voxeltype>unsigned char</voxeltype>\n";
	  out << "  <pvlvoxeltype>unsigned char</pvlvoxeltype>\n";
	  out << QString("  <gridsize>%1 %2 %3</gridsize>\n").arg(m_nX).arg(m_nY).arg(m_nZ);
	  out << "  <voxelunit>nounit</voxelunit>\n";
	  out << "  <voxelsize>1 1 1</voxelsize>\n";
	  out << "  <description></description>\n";
	  out << QString("  <slabsize>%1</slabsize>\n").arg(m_nX+1);
	  out << "  <rawmap>0 255 </rawmap>\n";
	  out << "  <pvlmap>0 255 </pvlmap>\n";
	  out << "</PvlDotNcFileHeader>\n";
	  out.flush();
	  if (out.status() != QTextStream::Ok)
	    {
	      const QString detail = fp.errorString();
	      fp.cancelWriting();
	      QMessageBox::warning(
	          0, "PVL sidecar not saved",
	          QString("The filtered volume was saved, but the PVL sidecar could not be written completely.\n%1")
	          .arg(detail));
	      return;
	    }
	  if (!fp.commit())
	    {
	      QMessageBox::warning(
	          0, "PVL sidecar not saved",
	          QString("The filtered volume was saved, but the PVL sidecar could not be committed.\n%1")
	          .arg(fp.errorString()));
	      return;
	    }

	  QMessageBox::information(0, "", QString("pvl.nc information saved in "+pvlflnm));
	}
    }

}

void
SmoothingFilter::GradientAnisotropicDiffusionFilter(float *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("This filter performs anisotropic diffusion on a scalar volume using the classic Perona-Malik.  The conductance term (which controls the sensitivity of the process to edge contrast) is chosen as a function of the gradient magnitude.\n");
  m_meshLog->insertPlainText("This filter requires 3 parameters : number of iterations to be performed, time step and conductance parameter used in the computation of the level set evolution.\n");
  m_meshLog->moveCursor(QTextCursor::End);


  typedef itk::Image<float, 3> ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = m_nZ;
  size[1] = m_nY;
  size[2] = m_nX;

  ImageType::RegionType region(start, size);

  m_niter = 5;
  float timeStep = 0.0625;
  float conductance = 1.0;

  bool ok;
  QString text = QInputDialog::getText(0, "Parameters",
				       "Iterations, time step, conductance",
				       QLineEdit::Normal,
				       "5 0.0625 1.0",
				       &ok);
  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	m_niter = list[0].toInt();
      if (list.count() > 1)
	timeStep = list[1].toFloat();
      if (list.count() > 2)
	conductance = list[2].toFloat();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  float *iptr = (float*)image->GetBufferPointer();
  memcpy(iptr, inVol,
         sizeof(float)*static_cast<size_t>(m_nX)*m_nY*m_nZ);

  typedef itk::GradientAnisotropicDiffusionImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetNumberOfIterations(m_niter);
  filter->SetTimeStep(timeStep);
  filter->SetConductanceParameter(conductance);

#ifdef Q_OS_MAC
      QMessageBox::information(0, "", "Applying gradient anisotropic diffusion filter");
#endif

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  float *outVol = (float*)(dimg->GetBufferPointer());
  
  memcpy(inVol, outVol,
         sizeof(float)*static_cast<size_t>(m_nX)*m_nY*m_nZ);
}

void
SmoothingFilter::CurvatureAnisotropicDiffusionFilter(float *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("This filter performs anisotropic diffusion on a scalar volume using modified curvature diffusion equation.\n");
  m_meshLog->insertPlainText("This filter requires 3 parameters : number of iterations to be performed, time step and conductance parameter used in the computation of the level set evolution.\n");
  m_meshLog->moveCursor(QTextCursor::End);

  typedef itk::Image<float, 3> ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = m_nZ;
  size[1] = m_nY;
  size[2] = m_nX;

  ImageType::RegionType region(start, size);

  m_niter = 5;
  float timeStep = 0.0625;
  float conductance = 1.0;

  bool ok;
  QString text = QInputDialog::getText(0, "Parameters",
				       "Iterations, time step, conductance",
				       QLineEdit::Normal,
				       "5 0.0625 1.0",
				       &ok);
  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	m_niter = list[0].toInt();
      if (list.count() > 1)
	timeStep = list[1].toFloat();
      if (list.count() > 2)
	conductance = list[2].toFloat();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  float *iptr = (float*)image->GetBufferPointer();
  memcpy(iptr, inVol,
         sizeof(float)*static_cast<size_t>(m_nX)*m_nY*m_nZ);

  typedef itk::CurvatureAnisotropicDiffusionImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetNumberOfIterations(m_niter);
  filter->SetTimeStep(timeStep);
  filter->SetConductanceParameter(conductance);

#ifdef Q_OS_MAC
      QMessageBox::information(0, "", "Applying curvature anisotropic diffusion filter");
#endif

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  float *outVol = (float*)(dimg->GetBufferPointer());
  
  memcpy(inVol, outVol,
         sizeof(float)*static_cast<size_t>(m_nX)*m_nY*m_nZ);
}

void
SmoothingFilter::CurvatureFlowFilter(float *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("This filter implements a curvature driven image denoising algorithm. Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is then evolved using a curvature-based speed function.  Areas of high curvature will diffuse faster than the areas of low curvature.  Hence, small jagged noise artefacts will disappear quickly, while large scale artefacts will be slow to evolve, thereby preserving sharp boundaries between objects.\n");
  m_meshLog->insertPlainText("This filter requires 2 parameters : number of iterations to be performed and time step.\n");
  m_meshLog->moveCursor(QTextCursor::End);

  typedef itk::Image<float, 3> ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = m_nZ;
  size[1] = m_nY;
  size[2] = m_nX;

  ImageType::RegionType region(start, size);

  m_niter = 5;
  float timeStep = 0.0625;

  bool ok;
  QString text = QInputDialog::getText(0, "Parameters",
				       "Iterations, time step",
				       QLineEdit::Normal,
				       "5 0.0625",
				       &ok);
  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	m_niter = list[0].toInt();
      if (list.count() > 1)
	timeStep = list[1].toFloat();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  float *iptr = (float*)image->GetBufferPointer();
  memcpy(iptr, inVol,
         sizeof(float)*static_cast<size_t>(m_nX)*m_nY*m_nZ);

  typedef itk::CurvatureFlowImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetNumberOfIterations(m_niter);
  filter->SetTimeStep(timeStep);

#ifdef Q_OS_MAC
      QMessageBox::information(0, "", "Applying curvature flow filter");
#endif

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  float *outVol = (float*)(dimg->GetBufferPointer());
  
  memcpy(inVol, outVol,
         sizeof(float)*static_cast<size_t>(m_nX)*m_nY*m_nZ);
}

void
SmoothingFilter::MinMaxCurvatureFlowFilter(float *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("This filter implements a curvature driven image denoising algorithm. Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is then evolved using a curvature-based speed function. In min/max curvature flow, movement is turned on or off depending on the scale of the noise one wants to remove. Switching depends on the average image value of a region of radius R around each point. The choice of this stencil radius, governs the scale of the noise to be removed.\nThe threshold value is the average intensity obtained in the direction perpendicular to the gradient at the extrema of the local neighborhood.\n");
  m_meshLog->insertPlainText("This filter requires 3 parameters : number of iterations to be performed, timestep and stencil radius.\n");
  m_meshLog->moveCursor(QTextCursor::End);

  typedef itk::Image< float, 3 > ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = m_nZ;
  size[1] = m_nY;
  size[2] = m_nX;

  ImageType::RegionType region(start, size);

  m_niter = 10;
  float timestep = 0.0625;
  int radius = 1;

  bool ok;
  QString text = QInputDialog::getText(0, "Parameters",
				       "Iterations, timestep, radius",
				       QLineEdit::Normal,
				       "5 0.0625 2",
				       &ok);
  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	m_niter = list[0].toInt();
      if (list.count() > 1)
	radius = list[1].toInt();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  float *iptr = (float*)image->GetBufferPointer();
  memcpy(iptr, inVol,
         sizeof(float)*static_cast<size_t>(m_nX)*m_nY*m_nZ);

  typedef itk::MinMaxCurvatureFlowImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetNumberOfIterations(m_niter);
  filter->SetTimeStep(timestep);
  filter->SetStencilRadius(radius);

#ifdef Q_OS_MAC
      QMessageBox::information(0, "", "Applying minmax curvature flow filter");
#endif

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  float *outVol = (float*)(dimg->GetBufferPointer());
  
  memcpy(inVol, outVol,
         sizeof(float)*static_cast<size_t>(m_nX)*m_nY*m_nZ);
}

void
SmoothingFilter::BilateralFilter(uchar *inVol)
{
  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText("This filter uses bilateral filtering to blur an image using both domain and range neighborhoods. Pixels that are close to a pixel in the image domain and similar to a pixel in the image range are used to calculate the filtered value. Two gaussian kernels (one in the image domain and one in the image range) are used to smooth the image. The result is an image that is smoothed in homogeneous regions yet has edges preserved. The result is similar to anisotropic diffusion but the implementation in non-iterative.\n");
  m_meshLog->insertPlainText("This filter requires 2 parameters : domain sigma, range sigma for respective gaussian kernels.\n");
  m_meshLog->moveCursor(QTextCursor::End);

  typedef itk::Image<uchar, 3> ImageType;

  ImageType::IndexType start;
  start.Fill(0);

  ImageType::SizeType size;
  size[0] = m_nZ;
  size[1] = m_nY;
  size[2] = m_nX;

  ImageType::RegionType region(start, size);

  double domainSigma = 1.0;
  double rangeSigma = 1.0;

  bool ok;
  QString text = QInputDialog::getText(0, "Parameters",
				       "Domain Sigma, Range Sigma",
				       QLineEdit::Normal,
				       "0.5 1.0",
				       &ok);

  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() > 0)
	domainSigma = list[0].toFloat();
      if (list.count() > 1)
	rangeSigma = list[1].toFloat();
    }

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
  uchar *iptr = (uchar*)image->GetBufferPointer();
  memcpy(iptr, inVol, static_cast<size_t>(m_nX)*m_nY*m_nZ);

  typedef itk::BilateralImageFilter<ImageType, ImageType> Filter;
  Filter::Pointer filter = Filter::New();
  filter->SetInput( image );
  filter->SetDomainSigma(domainSigma);
  filter->SetRangeSigma(rangeSigma);

  // set up progress update
  m_prog = 0;
  typedef itk::SimpleMemberCommand<SmoothingFilter> CommandProgress;
  CommandProgress::Pointer progressbar = CommandProgress::New();
  progressbar->SetCallbackFunction(this, &SmoothingFilter::next);
  filter->AddObserver(itk::ProgressEvent(), progressbar);

  filter->Update();
 
  ImageType *dimg = filter->GetOutput();
  uchar *outVol = (uchar*)(dimg->GetBufferPointer());
  
  memcpy(inVol, outVol, static_cast<size_t>(m_nX)*m_nY*m_nZ);
}

bool
SmoothingFilter::checkBlend(Vec po, ushort v, uchar* lut)
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
SmoothingFilter::checkCrop(Vec po)
{
  for(int ci=0; ci<m_crops.count(); ci++)
    {
      if (m_crops[ci].cropType() < CropObject::Tear_Tear)
	{
	  if (m_crops[ci].checkCropped(po) == false)
	    return false;
	}
    }
  return true;
}

bool
SmoothingFilter::checkPathBlend(Vec po, ushort v, uchar* lut)
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
SmoothingFilter::checkPathCrop(Vec po)
{
  for(int ci=0; ci<m_paths.count(); ci++)
    {
      if (m_paths[ci].crop())
	{
	  if (m_paths[ci].checkCropped(po) == false)
	    return false;
	}
    }
  return true;
}

