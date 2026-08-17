#include "remapwidget.h"

#include <QSignalBlocker>
#include "global.h"
#include "staticfunctions.h"
#include "common.h"
#include "raw2pvl.h"
#include "savepvldialog.h"
#include "fileslistdialog.h"
#include "importmemoryadmission.h"
#include "../../common/src/recoveryjournal.h"
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>

#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
class SaveImagesRecovery
{
public:
  SaveImagesRecovery() : m_active(false), m_commitAttempted(false),
                         m_committed(false) {}

  bool begin(const QString& directory, QString *error)
  {
    m_active = RecoveryJournal::beginDirect(
      directory, QStringLiteral("save-images"), m_state, error);
    return m_active;
  }

  bool addTarget(const QString& target, QString *error)
  {
    if (!m_active)
      return false;
    return RecoveryJournal::addDirectTarget(m_state, target, error);
  }

  bool commit(QString *error)
  {
    if (!m_active)
      return false;
    // If commit fails after PREPARED/COMMITTED was recorded, leave the
    // journal for the next operation to recover instead of guessing which
    // part of cleanup completed.
    m_commitAttempted = true;
    const bool ok = RecoveryJournal::commitDirect(m_state, error);
    if (ok)
      {
        m_committed = true;
        m_active = false;
      }
    return ok;
  }

  ~SaveImagesRecovery()
  {
    if (m_active && !m_commitAttempted)
      {
        QString ignored;
        (void)RecoveryJournal::rollbackDirect(m_state, &ignored);
      }
  }

private:
  RecoveryJournalState m_state;
  bool m_active;
  bool m_commitAttempted;
  bool m_committed;
};
}

void RemapWidget::setPvlMapMax(int pmm) { m_histogramWidget->setPvlMapMax(pmm); }

RemapWidget::RemapWidget(QWidget *parent) :
  QWidget(parent),
  m_volData(new VolumeData),
  m_quickRawRequested(false)
{
  ui.setupUi(this);

  QVBoxLayout *layout1 = new QVBoxLayout();
  layout1->setContentsMargins(0,0,0,0);
  ui.histogramFrame->setLayout(layout1);

  QVBoxLayout *layout2 = new QVBoxLayout();
  layout2->setContentsMargins(0,0,0,0);
  ui.imageFrame->setLayout(layout2);

  QVBoxLayout *layout3 = new QVBoxLayout();
  layout3->setContentsMargins(0,0,0,0);
  ui.colorFrame->setLayout(layout3);

  QVBoxLayout *layout4 = new QVBoxLayout();
  layout4->setContentsMargins(0,0,0,0);
  ui.sliderFrame->setLayout(layout4);

  m_scrollArea = new QScrollArea;
  m_scrollArea->setBackgroundRole(QPalette::Dark);
  ui.imageFrame->layout()->addWidget(m_scrollArea);


  m_histogramWidget = 0;
  m_imageWidget = 0;
  m_gradientWidget = 0;
  m_slider = 0;


  hideWidgets();

  m_fileNames.clear();
  m_timeseriesFiles.clear();
  m_mergeVolumes = false;
}

void
RemapWidget::hideWidgets()
{
  ui.checkBox->hide();
  ui.butX->hide();
  ui.butY->hide();
  ui.butZ->hide();
}

void
RemapWidget::showWidgets()
{
  m_histogramWidget->show();
  m_gradientWidget->show();

  if (m_volData->voxelType() == _Rgb ||
      m_volData->voxelType() == _Rgba)
    {
      ui.checkBox->hide();
      ui.checkBox->setCheckState(Qt::Unchecked);
      m_gradientWidget->hide();
      m_histogramWidget->hide();
    }
  else
    {
      ui.checkBox->show();
      ui.checkBox->setCheckState(Qt::Checked);
      m_histogramWidget->show();
      m_gradientWidget->show();
    }

////disabling slice selection
//  ui.butX->show();
//  ui.butY->show();
//  ui.butZ->show();
//  ui.butZ->setChecked(true);
}

void
RemapWidget::saveVoxelInfo()
{
  m_vu = m_volData->voxelUnit();
  m_volData->voxelSize(m_vx, m_vy, m_vz);
//  QMessageBox::information(0, "", QString("%1 : %2 %3 %4").
//			   arg(m_vu).arg(m_vx).arg(m_vy).arg(m_vz));
}

bool
RemapWidget::setFile(QList<QString> flnm,
		     QString plugin,
		     bool vol4d,
		     bool skipRawDialog)
{
  std::unique_ptr<VolumeData> candidate(new VolumeData);
  if (!candidate->setFile(flnm, plugin, vol4d, skipRawDialog))
    {
      if (!candidate->lastOperationCanceled() &&
	  !candidate->lastError().isEmpty())
	QMessageBox::critical(0, "Volume Load Error", candidate->lastError());
      return false;
    }

  m_mergeVolumes = false;
  m_fileNames = flnm;
  m_timeseriesFiles.clear();
  Global::statusBar()->clearMessage();

  hideWidgets();
  delete m_histogramWidget;
  delete m_imageWidget;
  delete m_gradientWidget;
  delete m_slider;
  m_histogramWidget = 0;
  m_imageWidget = 0;
  m_gradientWidget = 0;
  m_slider = 0;

  m_volumeFile = flnm;
  m_volData.swap(candidate);

  m_histogramWidget = new RemapHistogramWidget();
  m_histogramWidget->setMinimumSize(100, 300);
  m_histogramWidget->setSizePolicy(QSizePolicy::Expanding,
				   QSizePolicy::Fixed);

  m_gradientWidget = new GradientEditorWidget();
  m_gradientWidget->setDrawBox(false);
  m_gradientWidget->setMinimumSize(200, 40);
  m_gradientWidget->setGeneralLock(GradientEditor::LockToHalfHeight);
  
  int d, w, h;
  m_volData->gridSize(d, w, h);

  m_slider = new MySlider();
  m_slider->set(0, d-1, 0, d-1, 0);
  
  ui.histogramFrame->layout()->addWidget(m_histogramWidget);
  ui.colorFrame->layout()->addWidget(m_gradientWidget);
  ui.sliderFrame->layout()->addWidget(m_slider);

  m_imageWidget = new RemapImage();
  m_imageWidget->setGridSize(d, w, h);
  m_scrollArea->setWidget(m_imageWidget);

  m_currSlice = 0;

  connect(m_histogramWidget, SIGNAL(getHistogram()),
	  this, SLOT(getHistogram()));  

  connect(m_histogramWidget, SIGNAL(newMapping()),
	  this, SLOT(newMapping()));  

  connect(m_histogramWidget, SIGNAL(newMinMax(float, float)),
	  this, SLOT(newMinMax(float, float)));

  connect(m_imageWidget, SIGNAL(getSlice(int)),
	  this, SLOT(getSlice(int)));  

  connect(m_imageWidget, SIGNAL(getRawValue(int, int, int)),
	  this, SLOT(getRawValue(int, int, int)));

  connect(m_imageWidget, SIGNAL(newMinMax(float, float)),
	  this, SLOT(newMinMax(float, float)));

  connect(m_imageWidget, SIGNAL(saveTrimmed(int, int, int,
					    int, int, int)),
	  this, SLOT(saveTrimmed(int, int, int,
				 int, int, int)));
  
  connect(m_imageWidget, SIGNAL(saveIsosurface(int, int, int,
					       int, int, int)),
	  this, SLOT(saveIsosurface(int, int, int,
				    int, int, int)));
  
  connect(m_imageWidget, SIGNAL(saveTrimmedImages(int, int, int,
						  int, int, int)),
	  this, SLOT(saveTrimmedImages(int, int, int,
				       int, int, int)));
  
  connect(m_imageWidget, SIGNAL(extractRawVolume()),
	  this, SLOT(extractRawVolume()));
  
  connect(m_gradientWidget, SIGNAL(gradientChanged(QGradientStops)),
	  m_imageWidget, SLOT(setGradientStops(QGradientStops)));

  connect(m_gradientWidget, SIGNAL(gradientChanged(QGradientStops)),
	  m_histogramWidget, SLOT(setGradientStops(QGradientStops)));


  connect(m_slider, SIGNAL(valueChanged(int)),
	  m_imageWidget, SLOT(sliceChanged(int)));

  connect(m_slider, SIGNAL(userRangeChanged(int, int)),
	  m_imageWidget, SLOT(userRangeChanged(int, int)));


  QGradientStops stops;
  stops << QGradientStop(0, Qt::black)
	<< QGradientStop(1, Qt::white);
  m_gradientWidget->setColorGradient(stops);
  m_imageWidget->setGradientStops(stops);
  m_histogramWidget->setGradientStops(stops);

  setRawMinMax();
  
  showWidgets();

  return true;
}

void
RemapWidget::on_butZ_clicked()
{
  m_imageWidget->setSliceType(RemapImage::DSlice);

  int d, w, h, u0, u1;
  m_volData->gridSize(d, w, h);
  m_imageWidget->depthUserRange(u0, u1);
  m_slider->set(0, d-1, u0, u1, 0);
}
void
RemapWidget::on_butY_clicked()
{
  m_imageWidget->setSliceType(RemapImage::WSlice);

  int d, w, h, u0, u1;
  m_volData->gridSize(d, w, h);
  m_imageWidget->widthUserRange(u0, u1);
  m_slider->set(0, w-1, u0, u1, 0);
}
void
RemapWidget::on_butX_clicked()
{
  m_imageWidget->setSliceType(RemapImage::HSlice);

  int d, w, h, u0, u1;
  m_volData->gridSize(d, w, h);
  m_imageWidget->heightUserRange(u0, u1);
  m_slider->set(0, h-1, u0, u1, 0);
}

void
RemapWidget::newMinMax(float rmin, float rmax)
{
  if (!m_volData->setMinMax(rmin, rmax))
    {
      QMessageBox::warning(0, "Histogram Range", m_volData->lastError());
      return;
    }
  setRawMinMax();
}

void
RemapWidget::getRawValue(int d, int w, int h)
{
  m_imageWidget->setRawValue(m_volData->rawValue(d, w, h));
}

void
RemapWidget::setRawMinMax()
{
  float rawMin, rawMax;
  rawMin = m_volData->rawMin();
  rawMax = m_volData->rawMax();

  int minRaw, maxRaw;
  minRaw = maxRaw = -1;
  if (m_volData->voxelType() == _UChar) { minRaw=0; maxRaw=255; }
  if (m_volData->voxelType() == _Char) { minRaw=-128; maxRaw=127; }
  if (m_volData->voxelType() == _UShort) { minRaw=0; maxRaw=65535; }
  if (m_volData->voxelType() == _Short) { minRaw=-32768; maxRaw=32767; }

  // Rendering must never trigger synchronous volume I/O.  Initialize the
  // histogram and mapping before the widget is painted, while blocking the
  // normal mapping-changed signal that would otherwise decode a preview
  // before the slice orientation has been configured.
  {
    const QSignalBlocker blocker(m_histogramWidget);
    m_histogramWidget->setRawMinMax(rawMin, rawMax, minRaw, maxRaw);
    m_histogramWidget->setHistogram(m_volData->histogram());
  }
  m_volData->setMap(m_histogramWidget->rawMap(),
                    m_histogramWidget->pvlMap());

  if (ui.butZ->isChecked())
    m_imageWidget->setSliceType(RemapImage::DSlice);
  else if (ui.butY->isChecked())
    m_imageWidget->setSliceType(RemapImage::WSlice);
  else if (ui.butX->isChecked())
    m_imageWidget->setSliceType(RemapImage::HSlice);
}

void
RemapWidget::getSlice(int slc)
{
  m_currSlice = slc;

  if (ui.butZ->isChecked())
    m_imageWidget->setImage(m_volData->getDepthSliceImage(m_currSlice));
  else if (ui.butY->isChecked())
    m_imageWidget->setImage(m_volData->getWidthSliceImage(m_currSlice));
  else if (ui.butX->isChecked())
    m_imageWidget->setImage(m_volData->getHeightSliceImage(m_currSlice));

  m_slider->setValue(slc);
}

void
RemapWidget::newMapping()
{
  QList<float> rawMap;
  QList<int> pvlMap;

  rawMap = m_histogramWidget->rawMap();
  pvlMap = m_histogramWidget->pvlMap();

  m_volData->setMap(rawMap, pvlMap);
  
  if (ui.butZ->isChecked())
    m_imageWidget->setImage(m_volData->getDepthSliceImage(m_currSlice));
  else if (ui.butY->isChecked())
    m_imageWidget->setImage(m_volData->getWidthSliceImage(m_currSlice));
  else if (ui.butX->isChecked())
    m_imageWidget->setImage(m_volData->getHeightSliceImage(m_currSlice));
}

void
RemapWidget::getHistogram()
{
  // will be called only once
  m_histogramWidget->setHistogram(m_volData->histogram());
}

void
RemapWidget::loadLimits()
{
  if (m_imageWidget)
    {
      m_imageWidget->loadLimits();
      if (ui.butZ->isChecked())
	on_butZ_clicked();
      else if (ui.butY->isChecked())
	on_butY_clicked();
      else if (ui.butX->isChecked())
	on_butX_clicked();
    }
  else
    QMessageBox::information(0, "Error", "Load what ???  Load a volume first !!"); 
}

void
RemapWidget::saveLimits()
{
  if (m_imageWidget)
    m_imageWidget->saveLimits();
  else
    QMessageBox::information(0, "Error", "Save what ???  Load a volume first !!"); 
}

void
RemapWidget::saveImage()
{
  if (m_imageWidget)
    m_imageWidget->saveImage();
  else
    QMessageBox::information(0, "Error", "Save what ???  Load a volume first !!"); 
}

bool
RemapWidget::saveAs()
{
  if (!m_imageWidget)
    {
      QMessageBox::information(0, "Error", "Save what ???  Load a volume first !!");
      return false;
    }

  int dmin = 0;
  int dmax = 0;
  int wmin = 0;
  int wmax = 0;
  int hmin = 0;
  int hmax = 0;
  m_imageWidget->depthUserRange(dmin, dmax);
  m_imageWidget->widthUserRange(wmin, wmax);
  m_imageWidget->heightUserRange(hmin, hmax);
  m_quickRawRequested = false;
  return saveTrimmedResult(dmin, dmax, wmin, wmax, hmin, hmax);
}

void
RemapWidget::saveIsosurfaceAs()
{
  if (m_imageWidget)
    m_imageWidget->emitSaveIsosurface();
  else
    QMessageBox::information(0, "Error", "Save what ???  Load a volume first !!"); 
}

void
RemapWidget::saveImages()
{
  if (m_imageWidget)
    m_imageWidget->emitSaveTrimmedImages();
  else
    QMessageBox::information(0, "Error", "Save what ???  Load a volume first !!"); 
}

void
RemapWidget::extractRawVolume()
{
  QMessageBox::information(0, "Error", "Sorry not implemented");
}

void
RemapWidget::saveTrimmedImages(int dmin, int dmax,
			       int wmin, int wmax,
			       int hmin, int hmax)
{
  QString imgflnm;
  imgflnm = QFileDialog::getSaveFileName(0,
					 "Save images with basename as",
					 Global::previousDirectory(),
					 "Image Files (*.png *.tif *.bmp *.jpg *.raw)");
//					 0,
//					 QFileDialog::DontUseNativeDialog);

  if (imgflnm.isEmpty())
    return;

  QFileInfo f(imgflnm);
  // Keep staging on the target volume so the final renames remain atomic and
  // do not fail as cross-volume moves when the output is on another drive.
  QTemporaryDir staging(QDir(f.absolutePath()).filePath(
    QStringLiteral(".drishti-save-images-XXXXXX")));
  staging.setAutoRemove(true);
  if (!staging.isValid())
    {
      QMessageBox::critical(0, "Save Images",
                            "Cannot create a temporary batch directory.");
      return;
    }
  SaveImagesRecovery recovery;
  QString recoveryError;
  if (!recovery.begin(f.absolutePath(), &recoveryError))
    {
      QMessageBox::critical(0, "Save Images",
                            QString("Cannot recover the previous image batch: %1")
                            .arg(recoveryError));
      return;
    }
  QStringList stagedFiles;
  QStringList finalFiles;
  auto stagePathFor = [&](const QString& finalPath)
    {
      if (!recovery.addTarget(finalPath, &recoveryError))
        return QString();
      const QString stagePath = QDir(staging.path()).filePath(
        QString::number(finalFiles.size()) + "-" + QFileInfo(finalPath).fileName());
      stagedFiles.append(stagePath);
      finalFiles.append(finalPath);
      return stagePath;
    };
  QChar fillChar = '0';

  QProgressDialog progress("Saving images",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  int dsz = dmax-dmin+1;
  int wsz = wmax-wmin+1;
  int hsz = hmax-hmin+1;

  if (StaticFunctions::checkExtension(imgflnm, ".raw"))
    {
      int nX,nY,nZ, bps;
      m_volData->gridSize(nX,nY,nZ);
      bps = m_volData->bytesPerVoxel();
      int vtype = m_volData->voxelType();
      if (vtype == _Rgb || vtype == _Rgba)
        {
          QMessageBox::warning(
            0, "Save Images",
            "Packed RAW image export supports scalar volumes only. "
            "Use an image format or the RGB/RGBA volume export instead.");
          return;
        }

      uchar vt = 0;
      if (vtype == _UChar) vt = 0; // unsigned byte
      if (vtype == _Char) vt = 1; // signed byte
      if (vtype == _UShort) vt = 2; // unsigned short
      if (vtype == _Short) vt = 3; // signed short
      if (vtype == _Int) vt = 4; // int
      if (vtype == _Float) vt = 8; // float
      std::uint64_t sourcePixels = 0;
      std::uint64_t sourceBytes = 0;
      std::uint64_t outputPixels = 0;
      std::uint64_t outputBytes = 0;
      if (!checkedImportMultiply(static_cast<std::uint64_t>(nY),
				 static_cast<std::uint64_t>(nZ),
				 sourcePixels) ||
	  !checkedImportMultiply(sourcePixels,
				 static_cast<std::uint64_t>(bps),
				 sourceBytes) ||
	  !checkedImportMultiply(static_cast<std::uint64_t>(wsz),
				 static_cast<std::uint64_t>(hsz),
				 outputPixels) ||
	  !checkedImportMultiply(outputPixels,
				 static_cast<std::uint64_t>(bps),
				 outputBytes) ||
	  sourceBytes > static_cast<std::uint64_t>(
			      std::numeric_limits<std::size_t>::max()) ||
	  outputBytes > static_cast<std::uint64_t>(
			      std::numeric_limits<qint64>::max()))
	{
	  QMessageBox::critical(0, "Save Images",
				"The RAW slice size is unsupported.");
	  return;
	}
      const ImportMemoryAdmission admission =
	evaluateImportMemoryAdmission(sourceBytes);
      if (!admission.approved)
	{
	  QMessageBox::critical(0, "Save Images",
				"The RAW export was stopped because there is not "
				"enough physical-memory or Commit headroom.");
	  return;
	}
      std::unique_ptr<uchar[]> sliceStorage(
	new (std::nothrow) uchar[static_cast<std::size_t>(sourceBytes)]);
      if (!sliceStorage)
	{
	  QMessageBox::critical(0, "Save Images",
				"Cannot allocate the admitted RAW slice buffer.");
	  return;
	}
      uchar *slc = sliceStorage.get();
      for(int d=dmin; d<=dmax; d++)
	{
	  if (progress.wasCanceled())
	    {
	      QMessageBox::information(0, "Save Images", "-----Aborted-----");
	      return;
	    }
	  progress.setValue((int)(100*(float)(d-dmin)/(float)dsz));

	  if (!m_volData->getDepthSlice(d, slc))
	    {
	      QMessageBox::critical(0, "Save Images", m_volData->lastError());
	      return;
	    }
	  for(int j=wmin; j<=wmax; j++)
	    {
	      const qint64 destinationOffset =
		static_cast<qint64>(j-wmin)*hsz*bps;
	      const qint64 sourceOffset =
		(static_cast<qint64>(j)*nZ+hmin)*bps;
	      std::memmove(slc+destinationOffset, slc+sourceOffset,
			   static_cast<std::size_t>(hsz)*bps);
	    }

	  QString flname = f.absolutePath() +
	                   QDir::separator() +
	                   f.baseName();
	  flname += QString("%1").arg((int)d, 5, 10, fillChar);
	  flname += ".";
	  flname += f.completeSuffix();
	  const QString stageName = stagePathFor(flname);
	  if (stageName.isEmpty())
	    {
	      QMessageBox::critical(0, "Save Images", recoveryError);
	      return;
	    }
	  QSaveFile fout(stageName);
	  if (!fout.open(QFile::WriteOnly) ||
	      fout.write((char*)&vt, 1) != 1 ||
	      fout.write((char*)&wsz, 4) != 4 ||
	      fout.write((char*)&hsz, 4) != 4 ||
	      fout.write((char*)slc, static_cast<qint64>(outputBytes)) !=
	        static_cast<qint64>(outputBytes) ||
	      !fout.commit())
	    {
	      fout.cancelWriting();
	      QMessageBox::critical(0, "Save Images",
		QString("Cannot write '%1': %2").arg(flname, fout.errorString()));
	      return;
	    }
	}
    }      
  else
    {
      QImage timage;
      QVector<QRgb> colorMap = m_imageWidget->colorMap();
      for(int d=dmin; d<=dmax; d++)
	{
	  if (progress.wasCanceled())
	    {
	      QMessageBox::information(0, "Save Images", "-----Aborted-----");
	      return;
	    }
	  timage = m_volData->getDepthSliceImage(d);
	  if (timage.format() == QImage::Format_Indexed8)
	    timage.setColorTable(colorMap);  
	  timage = timage.copy(hmin, wmin, hsz, wsz);
	  
	  QString flname = f.absolutePath() + QDir::separator() +
	    f.baseName();
	  flname += QString("%1").arg((int)d, 5, 10, fillChar);
	  flname += ".";
	  flname += f.completeSuffix();
	  
	  const QString stageName = stagePathFor(flname);
	  if (stageName.isEmpty())
	    {
	      QMessageBox::critical(0, "Save Images", recoveryError);
	      return;
	    }
	  QSaveFile output(stageName);
	  const QByteArray format = f.completeSuffix().toLatin1();
	  if (!output.open(QIODevice::WriteOnly) ||
	      !timage.save(&output, format.constData()) ||
	      !output.commit())
	    {
	      output.cancelWriting();
	      QMessageBox::critical(0, "Save Images",
		QString("Cannot write '%1': %2").arg(flname, output.errorString()));
	      return;
	    }
	  progress.setValue((int)(100*(float)(d-dmin)/(float)dsz));
	}
    }
  for (int i=0; i<stagedFiles.size(); ++i)
    if (!QFile::rename(stagedFiles.at(i), finalFiles.at(i)))
      {
        QMessageBox::critical(0, "Save Images",
                              QString("Cannot commit image '%1': %2")
                              .arg(finalFiles.at(i), recoveryError));
        return;
      }
  if (!recovery.commit(&recoveryError))
    {
      QMessageBox::critical(0, "Save Images",
                            QString("The image batch could not be committed: %1")
                            .arg(recoveryError));
      return;
    }
  progress.setValue(100);
}
			       
void
RemapWidget::batchProcess()
{
  if (m_imageWidget)
    Raw2Pvl::batchProcess(m_volData.get(), m_timeseriesFiles);
  else
    QMessageBox::information(0, "Error", "Batch Process what ???  Load a volume data first !!"); 
}

bool
RemapWidget::saveTrimmed(int dmin, int dmax,
			 int wmin, int wmax,
			 int hmin, int hmax)
{
  // Quick RAW is an explicit UI action.  A legitimate 1x1x1 ROI at the
  // origin must continue through the normal Save As path.
  m_quickRawRequested = false;
  const bool result = saveTrimmedResult(dmin, dmax, wmin, wmax, hmin, hmax);
  m_quickRawRequested = false;
  return result;
}

bool
RemapWidget::saveQuickRaw()
{
  m_quickRawRequested = true;
  const bool result = saveTrimmedResult(0, 0, 0, 0, 0, 0);
  m_quickRawRequested = false;
  return result;
}

bool
RemapWidget::saveTrimmedResult(int dmin, int dmax,
			       int wmin, int wmax,
			       int hmin, int hmax)
{
  if (m_quickRawRequested)
    {
      return Raw2Pvl::quickRaw(m_volData.get(),
			       m_volumeFile);
    }
  
  
  //-----------------------------------------------------
  // -- take care of RGB/A volumes
  if (m_volData->voxelType() == _Rgb ||
      m_volData->voxelType() == _Rgba)
    {
      QString trimRawFile;
      trimRawFile = QFileDialog::getSaveFileName(0,
						 "Save trimmed RGB volume to netCDF file",
						 Global::previousDirectory(),
						 "NetCDF Files (*.pvl.nc)",
						 0,
						 QFileDialog::DontUseNativeDialog);

      if (trimRawFile.isEmpty())
	return false;

      if (trimRawFile.endsWith(".pvl.nc.pvl.nc"))
	  trimRawFile.chop(7);
      if (!trimRawFile.endsWith(".pvl.nc"))
	 trimRawFile += ".pvl.nc";

      if (!m_volData->saveTrimmed(trimRawFile,
				  dmin, dmax,
				  wmin, wmax,
				  hmin, hmax))
	return false;

      QMessageBox::information(0, "Save RGB Volume", "Done");
      return true;
    }
  //-----------------------------------------------------

  if (!m_mergeVolumes)
    {
      return Raw2Pvl::savePvl(m_volData.get(),
			      dmin, dmax, wmin, wmax, hmin, hmax,
			      m_timeseriesFiles);
    }
  else
    {
      m_volData->setVoxelInfo(m_vu, m_vx, m_vy, m_vz);
      return Raw2Pvl::mergeVolumes(m_volData.get(),
				   dmin, dmax, wmin, wmax, hmin, hmax,
				   m_timeseriesFiles);
    }
}

void
RemapWidget::saveIsosurface(int dmin, int dmax,
			    int wmin, int wmax,
			    int hmin, int hmax)
{
  //-----------------------------------------------------
  // -- take care of RGB/A volumes
  if (m_volData->voxelType() == _Rgb ||
      m_volData->voxelType() == _Rgba)
    {
      QMessageBox::information(0, "Save Isosurface for RGB Volume", "Not possible");
      return;
    }
  //-----------------------------------------------------

  Raw2Pvl::saveIsosurface(m_volData.get(),
			  dmin, dmax, wmin, wmax, hmin, hmax,
			  m_timeseriesFiles);
}

void
RemapWidget::handleTimeSeries(QString voltype,
			      QString plugin)
{
  QStringList flnms;
  flnms = QFileDialog::getOpenFileNames(0,
					QString("Load %1").arg(voltype),
					Global::previousDirectory(),
					"*",
					0,
					QFileDialog::DontUseNativeDialog);
  
  if (flnms.size() == 0)
    return;
  
  if (flnms.count() > 1)
    {
      FilesListDialog fld(flnms);
      fld.exec();
      if (fld.result() == QDialog::Rejected)
	return;
      flnms = fld.files();
    }
  QString mesg;
  mesg = "All operations on the first volume will be used\n";
  mesg += "as a template for others in the timeseries.\n";
  mesg += "Subsampling, smoothing, remaping applied to\n";
  mesg += "the first volume will be applied to others.";
  QMessageBox::information(0, "Time series", mesg);

  QFileInfo f(flnms[0]);
  Global::setPreviousDirectory(f.absolutePath());

  const QStringList firstVolume = QStringList() << flnms.first();
  bool loaded = false;
  if (plugin.contains("nc",Qt::CaseInsensitive))
    loaded = setFile(firstVolume, plugin, true);
  else
    loaded = setFile(firstVolume, plugin, false);

  if (!loaded)
    return;

  m_timeseriesFiles = flnms;

  m_mergeVolumes = false;
}

void
RemapWidget::handleMergeVolumes(QString voltype,
				QString plugin)
{
  QStringList flnms;
  flnms = QFileDialog::getOpenFileNames(0,
					QString("Load %1").arg(voltype),
					Global::previousDirectory(),
					"*",
					0,
					QFileDialog::DontUseNativeDialog);
  
  if (flnms.size() == 0)
    return;
  
  if (flnms.count() > 1)
    {
      FilesListDialog fld(flnms);
      fld.exec();
      if (fld.result() == QDialog::Rejected)
	return;
      flnms = fld.files();
    }
//  QString mesg;
//  mesg = "All operations on the first volume will be used\n";
//  mesg += "as a template for others in the timeseries.\n";
//  mesg += "Subsampling, smoothing, remaping applied to\n";
//  mesg += "the first volume will be applied to others.";
//  QMessageBox::information(0, "Time series", mesg);

  QFileInfo f(flnms[0]);
  Global::setPreviousDirectory(f.absolutePath());

  const QStringList firstVolume = QStringList() << flnms.first();
  bool loaded = false;
  if (plugin.contains("nc",Qt::CaseInsensitive))
    loaded = setFile(firstVolume, plugin, true);
  else
    loaded = setFile(firstVolume, plugin, false);

  if (!loaded)
    return;

  m_timeseriesFiles = flnms;

  m_mergeVolumes = true;
}

bool
RemapWidget::mergeVolumes(QString voltype,
			  QString plugin,
			  QStringList flnms)
{
  if (flnms.isEmpty())
    return false;

  QFileInfo f(flnms[0]);
  Global::setPreviousDirectory(f.absolutePath());

  const QStringList firstVolume = QStringList() << flnms.first();
  bool loaded = false;
  if (plugin.contains("nc",Qt::CaseInsensitive))
    loaded = setFile(firstVolume, plugin, true, true);
  else
    loaded = setFile(firstVolume, plugin, false, true);

  if (!loaded)
    return false;

  m_timeseriesFiles = flnms;

  m_mergeVolumes = true;
  return true;
}
