#include "volume.h"
#include "staticfunctions.h"
#include "global.h"
#include "getmemorysize.h"

#include <QDebug>
#include <QSaveFile>

#include <limits>
#include <memory>
#include <new>

namespace
{
bool planeByteCount(int first, int second, int bytesPerVoxel, size_t& bytes)
{
  if (first <= 0 || second <= 0 || bytesPerVoxel <= 0)
    return false;
  qint64 count = static_cast<qint64>(first)*second;
  if (count > std::numeric_limits<qint64>::max()/bytesPerVoxel)
    return false;
  count *= bytesPerVoxel;
  if (static_cast<quint64>(count) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return false;
  bytes = static_cast<size_t>(count);
  return true;
}

QString memorySizeText(std::uint64_t bytes)
{
  const long double gibibytes =
    static_cast<long double>(bytes)/(1024.0L*1024.0L*1024.0L);
  return QString("%1 GiB").arg(static_cast<double>(gibibytes), 0, 'f', 2);
}

QString memoryAdmissionReason(PaintMemoryAdmissionReason reason)
{
  switch (reason)
    {
    case PaintMemoryAdmissionReason::InMemoryApproved:
      return "current physical-memory and Commit budgets permit in-memory mode";
    case PaintMemoryAdmissionReason::MemoryStatusUnavailable:
      return "current available-memory information is unavailable";
    case PaintMemoryAdmissionReason::AddressSpaceLimit:
      return "a volume buffer exceeds the process address-space limit";
    case PaintMemoryAdmissionReason::LargeVolume:
      return "the resident volume is above the conservative large-volume threshold";
    case PaintMemoryAdmissionReason::InsufficientPhysicalMemory:
      return "the estimated peak exceeds current available physical memory after reserves";
    case PaintMemoryAdmissionReason::InsufficientCommit:
      return "the estimated peak exceeds current Commit headroom after reserves";
    case PaintMemoryAdmissionReason::ArithmeticOverflow:
      return "the volume memory estimate overflows a 64-bit byte count";
    }
  return "unknown memory-admission result";
}

bool currentMemoryAdmission(int depth, int width, int height,
                            int rawBytesPerVoxel, int maskBytesPerVoxel,
                            SystemMemoryStatus& status,
                            PaintMemoryAdmission& admission,
                            QString& error)
{
  if (depth <= 0 || width <= 0 || height <= 0 ||
      (rawBytesPerVoxel != 1 && rawBytesPerVoxel != 2) ||
      (maskBytesPerVoxel != 1 && maskBytesPerVoxel != 2))
    {
      error = QString("Cannot evaluate memory admission for invalid volume "
                      "geometry or voxel sizes");
      return false;
    }

  if (!getSystemMemoryStatus(status))
    status = SystemMemoryStatus();
  if (!evaluatePaintMemoryAdmission(
        static_cast<std::uint64_t>(depth),
        static_cast<std::uint64_t>(width),
        static_cast<std::uint64_t>(height),
        static_cast<std::uint32_t>(rawBytesPerVoxel),
        static_cast<std::uint32_t>(maskBytesPerVoxel),
        status, admission))
    {
      error = memoryAdmissionReason(admission.reason);
      return false;
    }
  return true;
}

void logMemoryAdmission(const SystemMemoryStatus& status,
                        const PaintMemoryAdmission& admission)
{
  const QString commitBudget = status.availableCommitKnown ?
    memorySizeText(admission.availableCommitBudgetBytes) :
    QString("unavailable");
  qInfo().noquote()
    << QString("Paint volume memory admission: mode=%1; reason=%2; "
               "resident=%3; estimated peak=%4; physical budget=%5; "
               "available physical=%6/%7; Commit budget=%8; "
               "Commit headroom/limit=%9/%10; system reserve=%11; "
               "iGPU reserve=%12; Commit reserve=%13")
         .arg(admission.useInMemory ? "in-memory" : "rejected")
         .arg(memoryAdmissionReason(admission.reason))
         .arg(memorySizeText(admission.residentVolumeBytes))
         .arg(memorySizeText(admission.estimatedPeakBytes))
         .arg(memorySizeText(admission.availablePhysicalBudgetBytes))
         .arg(memorySizeText(status.availablePhysicalBytes))
         .arg(memorySizeText(status.totalPhysicalBytes))
         .arg(commitBudget)
         .arg(status.availableCommitKnown ?
                memorySizeText(status.availableCommitBytes) :
                QString("unavailable"))
         .arg(status.availableCommitKnown ?
                memorySizeText(status.commitLimitBytes) :
                QString("unavailable"))
         .arg(memorySizeText(admission.systemReserveBytes))
         .arg(memorySizeText(admission.integratedGpuReserveBytes))
         .arg(memorySizeText(admission.commitReserveBytes));
}
}

bool
Volume::checkFileSave()
{
  m_lastError.clear();
  const bool saved = m_mask.checkFileSave();
  if (!saved)
    m_lastError = m_mask.lastError();
  return saved;
}

bool
Volume::createUndo()
{
  m_lastError.clear();
  const bool created = m_mask.createUndo();
  if (!created)
    m_lastError = m_mask.lastError();
  return created;
}

bool
Volume::undo()
{
  m_lastError.clear();
  const bool restored = m_mask.undo();
  if (!restored)
    m_lastError = m_mask.lastError();
  return restored;
}

bool
Volume::exiting()
{
  m_lastError.clear();
  const bool saved = m_mask.exiting();
  if (!saved)
    m_lastError = m_mask.lastError();
  return saved;
}

bool
Volume::saveIntermediateResults(bool forceSave)
{
  m_lastError.clear();
  const bool saved = m_mask.saveIntermediateResults(forceSave);
  if (!saved)
    m_lastError = m_mask.lastError();
  return saved;
}

bool
Volume::exportMask()
{
  m_lastError.clear();
  const bool exported = m_mask.exportMask();
  if (!exported)
    m_lastError = m_mask.lastError();
  return exported;
}
void Volume::checkPoint() { m_mask.checkPoint(); }
bool Volume::loadCheckPoint() { return m_mask.loadCheckPoint(); }
bool Volume::loadCheckPoint(QString flnm) { return m_mask.loadCheckPoint(flnm); }
bool Volume::deleteCheckPoint() { return m_mask.deleteCheckPoint(); }

bool
Volume::reloadMask()
{
  m_lastError.clear();
  const bool loaded = m_mask.loadMemFile();
  if (!loaded) m_lastError = m_mask.lastError();
  return loaded;
}

bool
Volume::loadRawMask(QString flnm)
{
  m_lastError.clear();
  const bool loaded = m_mask.loadRawFile(flnm);
  if (!loaded) m_lastError = m_mask.lastError();
  return loaded;
}

void
Volume::saveTagNames(QStringList tagNames)
{
  m_mask.saveTagNames(tagNames);
}
QStringList
Volume::loadTagNames()
{
  return m_mask.loadTagNames();
}

bool
Volume::saveMaskBlock(int d, int w, int h, int rad)
{
  m_lastError.clear();
  const bool saved = m_mask.saveMaskBlock(d, w, h, rad);
  if (!saved)
    m_lastError = m_mask.lastError();
  return saved;
}

bool
Volume::saveMaskBlock(QList< QList<int> > bl)
{
  m_lastError.clear();
  const bool saved = m_mask.saveMaskBlock(bl);
  if (!saved)
    m_lastError = m_mask.lastError();
  return saved;
}

bool
Volume::offloadMemFile()
{
  m_lastError.clear();
  if (!m_mask.offloadMemFile())
    {
      m_lastError = m_mask.lastError();
      return false;
    }
  if (!m_pvlFileManager.setMemMapped(false))
    {
      m_lastError = m_pvlFileManager.lastError();
      return false;
    }
  return true;
}

bool
Volume::loadMemFile()
{
  m_lastError.clear();
  if (m_pvlFileManager.memVolDataPtr() && m_mask.memMaskDataPtr())
    return true;

  const int rawBytesPerVoxel = Global::bytesPerVoxel();
  const int maskBytesPerVoxel = Global::bytesPerMask();
  SystemMemoryStatus memoryStatus;
  PaintMemoryAdmission admission;
  if (!currentMemoryAdmission(m_depth, m_width, m_height,
                              rawBytesPerVoxel, maskBytesPerVoxel,
                              memoryStatus, admission, m_lastError))
    return false;
  logMemoryAdmission(memoryStatus, admission);
  if (!admission.useInMemory)
    {
      m_lastError = QString("In-memory loading was refused: %1. "
                            "The volume remains offloaded; a complete "
                            "out-of-core editing backend is not available.")
                      .arg(memoryAdmissionReason(admission.reason));
      return false;
    }

  if (!m_pvlFileManager.setMemMapped(true))
    {
      m_lastError = m_pvlFileManager.lastError();
      return false;
    }
  if (!m_pvlFileManager.loadMemFile())
    {
      m_lastError = m_pvlFileManager.lastError();
      return false;
    }
  if (!m_mask.loadMemFile())
    {
      m_lastError = m_mask.lastError();
      return false;
    }
  return true;
}

bool
Volume::setMaskDepthSlice(int slc, uchar* tagData)
{
  m_lastError.clear();
  const bool written = m_mask.setMaskDepthSlice(slc, tagData);
  if (!written)
    m_lastError = m_mask.lastError();
  return written;
}

uchar*
Volume::getMaskDepthSliceImage(int slc)
{
  m_lastError.clear();
  uchar *slice = m_mask.getMaskDepthSliceImage(slc);
  if (!slice)
    m_lastError = m_mask.lastError();
  return slice;
}

uchar*
Volume::getMaskWidthSliceImage(int slc)
{
  m_lastError.clear();
  uchar *slice = m_mask.getMaskWidthSliceImage(slc);
  if (!slice)
    m_lastError = m_mask.lastError();
  return slice;
}

uchar*
Volume::getMaskHeightSliceImage(int slc)
{
  m_lastError.clear();
  uchar *slice = m_mask.getMaskHeightSliceImage(slc);
  if (!slice)
    m_lastError = m_mask.lastError();
  return slice;
}

Volume::Volume()
{
  m_valid = false;

  m_depth = m_height = m_width = 0;
  m_slice = 0;
  m_fileName.clear();

  (void)m_mask.reset();

  m_1dHistogram = 0;
  m_2dHistogram = 0;
  m_histImageData1D = 0;
  m_histImageData2D = 0;

  m_histogramImage1D = QImage(256, 256, QImage::Format_RGB32);
  m_histogramImage2D = QImage(256, 256, QImage::Format_RGB32);
}

Volume::~Volume() { (void)reset(); }

bool Volume::isValid() { return m_valid; }

bool Volume::reset()
{
  if (!m_mask.reset())
    {
      m_lastError = m_mask.lastError();
      return false;
    }

  m_valid = false;

  if (!m_pvlFileManager.reset())
    {
      m_lastError = m_pvlFileManager.lastError();
      return false;
    }

  m_fileName.clear();

  if (m_slice) delete [] m_slice;
  if (m_1dHistogram) delete [] m_1dHistogram;
  if (m_2dHistogram) delete [] m_2dHistogram;
  if (m_histImageData1D) delete m_histImageData1D;
  if (m_histImageData2D) delete m_histImageData2D;

  m_slice = 0;
  m_depth = m_height = m_width = 0;
  m_lastError.clear();
  m_1dHistogram = 0;
  m_2dHistogram = 0;
  m_histImageData1D = 0;
  m_histImageData2D = 0;
  return true;
}


void
Volume::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

bool
Volume::setFile(QString volfile)
{
  if (!reset())
    return false;

  const auto failLoad = [this](QString detail)
    {
      (void)reset();
      m_lastError = detail.isEmpty() ?
        QString("Cannot load the volume") : detail;
      QMessageBox::critical(0, "Volume Load Error", m_lastError);
      return false;
    };

  if (!StaticFunctions::xmlHeaderFile(volfile))
    {
      QMessageBox::information(0, "Error",
	QString("%1 is not a valid preprocessed volume file").
			       arg(volfile));
      return false;
    }

  StaticFunctions::getDimensionsFromHeader(volfile,
					   m_depth, m_width, m_height);

  int slabSize = StaticFunctions::getSlabsizeFromHeader(volfile);

  if (m_depth <= 0 || m_width <= 0 || m_height <= 0 || slabSize <= 0)
    return failLoad(QString("Invalid volume geometry %1 x %2 x %3 or slab size %4")
                    .arg(m_depth).arg(m_width).arg(m_height).arg(slabSize));

  m_fileName = volfile;

  int voxelType = StaticFunctions::getPvlVoxelTypeFromHeader(volfile);
  int headerSize = StaticFunctions::getPvlHeadersizeFromHeader(volfile);
  QStringList pvlnames = StaticFunctions::getPvlNamesFromHeader(volfile);
  if (voxelType != VolumeFileManager::_UChar &&
      voxelType != VolumeFileManager::_UShort)
    return failLoad(QString("Drishti Paint supports only unsigned 8-bit and 16-bit volumes (voxel type %1)")
                    .arg(voxelType));
  if (headerSize < 0 || pvlnames.isEmpty())
    return failLoad("The volume header has no valid data file or header size");

  m_pvlFileManager.setFilenameList(pvlnames);
  m_pvlFileManager.setBaseFilename(m_fileName);
  m_pvlFileManager.setVoxelType(voxelType);
  m_pvlFileManager.setDepth(m_depth);
  m_pvlFileManager.setWidth(m_width);
  m_pvlFileManager.setHeight(m_height);
  m_pvlFileManager.setHeaderSize(headerSize);
  m_pvlFileManager.setSlabSize(slabSize);

  const int bpv = (voxelType == VolumeFileManager::_UShort ? 2 : 1);

  Global::setBytesPerVoxel(bpv);

  const int maskBytesPerVoxel = Global::bytesPerMask();
  if (maskBytesPerVoxel != 1 && maskBytesPerVoxel != 2)
    return failLoad(QString("Unsupported mask element size %1")
                    .arg(maskBytesPerVoxel));

  SystemMemoryStatus memoryStatus;
  PaintMemoryAdmission admission;
  QString admissionError;
  if (!currentMemoryAdmission(m_depth, m_width, m_height,
                              bpv, maskBytesPerVoxel,
                              memoryStatus, admission, admissionError))
    return failLoad(admissionError);

  logMemoryAdmission(memoryStatus, admission);
  if (!admission.useInMemory)
    {
      const QString commitBudget = memoryStatus.availableCommitKnown ?
        memorySizeText(admission.availableCommitBudgetBytes) :
        QString("unavailable");
      return failLoad(
        QString("Drishti Paint refused to load this volume fully into memory: "
                "%1. Estimated peak %2; usable physical-memory budget %3; "
                "usable Commit budget %4. This build does not yet provide a "
                "complete out-of-core editing backend, so continuing would "
                "make some annotation tools unsafe. Close other applications, "
                "enable a system-managed page file, or use a smaller/downsampled "
                "volume.")
          .arg(memoryAdmissionReason(admission.reason))
          .arg(memorySizeText(admission.estimatedPeakBytes))
          .arg(memorySizeText(admission.availablePhysicalBudgetBytes))
          .arg(commitBudget));
    }

  const bool inMem = true;
  if (!m_pvlFileManager.setMemMapped(inMem))
    return failLoad(m_pvlFileManager.lastError());


  if (!m_pvlFileManager.loadMemFile())
    return failLoad(m_pvlFileManager.lastError());
  
  QString mfile = m_fileName;
  mfile.chop(6);
  mfile += QString("mask");
  if (!m_mask.setFile(mfile, inMem))
    return failLoad(m_mask.lastError());
  m_mask.setVoxelType(maskBytesPerVoxel == 1 ?
                      VolumeFileManager::_UChar :
                      VolumeFileManager::_UShort);
  if (!m_mask.setGridSize(m_depth, m_width, m_height, slabSize))
    return failLoad(m_mask.lastError().isEmpty() ?
                    QString("Cannot create or load the mask volume") :
                    m_mask.lastError());

  if (!genHistogram(false))
    return failLoad(m_lastError);
  //generateHistogramImage();

  m_valid = true;

  return true;
}

bool
Volume::genHistogram(bool forceHistogram)
{
  m_lastError.clear();
  if (!m_1dHistogram)
    m_1dHistogram = new (std::nothrow) int[256];
  if (!m_2dHistogram)
    m_2dHistogram = new (std::nothrow) int[256*256];
  if (!m_1dHistogram || !m_2dHistogram)
    {
      m_lastError = "Cannot allocate histogram buffers";
      return false;
    }
  memset(m_1dHistogram, 0, 256*sizeof(int));
  memset(m_2dHistogram, 0, 256*256*sizeof(int));

  QString hfilename = m_fileName;
  hfilename.chop(6);
  hfilename += QString("hist");
  QFile hfile(hfilename);
  const bool oneByte = (Global::bytesPerVoxel() == 1);
  const qint64 cachedBytes = oneByte ?
    256*sizeof(int) : 256*256*sizeof(int);

  if (!forceHistogram && hfile.exists() && hfile.size() == cachedBytes &&
      hfile.open(QFile::ReadOnly))
    {
      char *destination = oneByte ?
        reinterpret_cast<char*>(m_1dHistogram) :
        reinterpret_cast<char*>(m_2dHistogram);
      const bool loaded = (hfile.read(destination, cachedBytes) == cachedBytes);
      hfile.close();
      if (loaded)
        return true;
    }

  const int histogramBins = oneByte ? 256 : 256*256;
  std::unique_ptr<quint64[]> histogram(
    new (std::nothrow) quint64[histogramBins]);
  if (!histogram)
    {
      m_lastError = "Cannot allocate histogram counters";
      return false;
    }
  memset(histogram.get(), 0,
         static_cast<size_t>(histogramBins)*sizeof(quint64));

  QProgressDialog progress("Histogram Generation",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);
  progress.setWindowModality(Qt::ApplicationModal);
  qApp->processEvents();

  const qint64 planePixels = static_cast<qint64>(m_width)*m_height;
  for(int slc=0; slc<m_depth; slc++)
    {
      progress.setValue((int)(100.0*(float)slc/(float)m_depth));
      qApp->processEvents();

      uchar *vslice = m_pvlFileManager.getDepthSliceMem(slc);
      if (!vslice)
	{
	  m_lastError = m_pvlFileManager.lastError();
	  return false;
	}
      if (oneByte)
	{
	  for(qint64 j=0; j<planePixels; j++)
	    histogram[vslice[j]]++;
	}
      else
	{
	  const ushort *values = reinterpret_cast<const ushort*>(vslice);
	  for(qint64 j=0; j<planePixels; j++)
	    histogram[values[j]]++;
	}
    }

  const quint64 maximum =
    static_cast<quint64>(std::numeric_limits<int>::max());
  int *destination = oneByte ? m_1dHistogram : m_2dHistogram;
  for(int i=0; i<histogramBins; i++)
    destination[i] = static_cast<int>(qMin(histogram[i], maximum));

  QSaveFile output(hfilename);
  if (!output.open(QFile::WriteOnly))
    {
      progress.setValue(100);
      return true; // The histogram cache is optional.
    }
  const qint64 written = output.write(
    reinterpret_cast<const char*>(destination), cachedBytes);
  if (written != cachedBytes || !output.commit())
    {
      output.cancelWriting();
      progress.setValue(100);
      return true;
    }

  progress.setValue(100);
  return true;
}

uchar*
Volume::getDepthSliceImage(int slc)
{
  size_t nbytes = 0;
  if (!planeByteCount(m_width, m_height, Global::bytesPerVoxel(), nbytes))
    {
      m_lastError = "Depth-slice byte count overflows";
      return 0;
    }
  uchar *vslice = m_pvlFileManager.getDepthSliceMem(slc);
  if (!vslice)
    {
      m_lastError = m_pvlFileManager.lastError();
      return 0;
    }
  uchar *replacement = new (std::nothrow) uchar[nbytes];
  if (!replacement)
    {
      m_lastError = QString("Cannot allocate %1-byte depth slice")
                      .arg(static_cast<qulonglong>(nbytes));
      return 0;
    }
  memcpy(replacement, vslice, nbytes);
  delete [] m_slice;
  m_slice = replacement;

  return m_slice;
}

uchar*
Volume::getWidthSliceImage(int slc)
{
  size_t nbytes = 0;
  if (!planeByteCount(m_depth, m_height, Global::bytesPerVoxel(), nbytes))
    {
      m_lastError = "Width-slice byte count overflows";
      return 0;
    }
  uchar *vslice = m_pvlFileManager.getWidthSliceMem(slc);
  if (!vslice)
    {
      m_lastError = m_pvlFileManager.lastError();
      return 0;
    }
  uchar *replacement = new (std::nothrow) uchar[nbytes];
  if (!replacement)
    {
      m_lastError = QString("Cannot allocate %1-byte width slice")
                      .arg(static_cast<qulonglong>(nbytes));
      return 0;
    }
  memcpy(replacement, vslice, nbytes);
  delete [] m_slice;
  m_slice = replacement;

  return m_slice;
}

uchar*
Volume::getHeightSliceImage(int slc)
{
  size_t nbytes = 0;
  if (!planeByteCount(m_depth, m_width, Global::bytesPerVoxel(), nbytes))
    {
      m_lastError = "Height-slice byte count overflows";
      return 0;
    }
  uchar *vslice = m_pvlFileManager.getHeightSliceMem(slc);
  if (!vslice)
    {
      m_lastError = m_pvlFileManager.lastError();
      return 0;
    }
  uchar *replacement = new (std::nothrow) uchar[nbytes];
  if (!replacement)
    {
      m_lastError = QString("Cannot allocate %1-byte height slice")
                      .arg(static_cast<qulonglong>(nbytes));
      return 0;
    }
  memcpy(replacement, vslice, nbytes);
  delete [] m_slice;
  m_slice = replacement;

  return m_slice;
}


QList<int>
Volume::rawValue(int d, int w, int h)
{
  QList<int> vgt;
  vgt.clear();

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return vgt;

  uchar *vslice = m_pvlFileManager.rawValueMem(d, w, h);
  if (!vslice)
    {
      m_lastError = m_pvlFileManager.lastError();
      return vgt;
    }
  if (Global::bytesPerVoxel() == 1)
    vgt << vslice[0];
  else
    vgt << ((ushort*)vslice)[0];
  vgt << 0;
  
  vgt << m_mask.maskValue(d,w,h);

  return vgt;
}

void
Volume::generateHistogramImage()
{
  if(! m_histImageData1D)
    m_histImageData1D = new uchar[256*256*4];
  if(! m_histImageData2D)
    m_histImageData2D = new uchar[256*256*4];

  memset(m_histImageData1D, 0, 256*256*4);
  memset(m_histImageData2D, 0, 256*256*4);

  int *hist2D = m_2dHistogram;
  for (int i=0; i<256*256; i++)
    {
      m_histImageData2D[4*i + 3] = 255;
      m_histImageData2D[4*i + 0] = hist2D[i];
      m_histImageData2D[4*i + 1] = hist2D[i];
      m_histImageData2D[4*i + 2] = hist2D[i];
    }
  m_histogramImage2D = QImage(m_histImageData2D,
			      256, 256,
			      QImage::Format_ARGB32);
  m_histogramImage2D = m_histogramImage2D.mirrored();  


  int *hist1D = m_1dHistogram;
  //memset(m_histImageData1D, 0, 4*256*256);
  for (int i=0; i<256; i++)
    {
      for (int j=0; j<256; j++)
	{
	  int idx = 256*j + i;
	  m_histImageData1D[4*idx + 3] = 255;
	}

//      int h = hist1D[i];
//      for (int j=0; j<h; j++)
//	{
//	  int idx = 256*j + i;
//	  m_histImageData1D[4*idx + 0] = 255*j/h;
//	  m_histImageData1D[4*idx + 1] = 255*j/h;
//	  m_histImageData1D[4*idx + 2] = 255*j/h;
//	}
    }
  m_histogramImage1D = QImage(m_histImageData1D,
			      256, 256,
			      QImage::Format_ARGB32);
  m_histogramImage1D = m_histogramImage1D.mirrored();  
}

bool
Volume::tagDSlice(int currslice, uchar *usermask)
{
  const bool written = m_mask.tagDSlice(currslice, usermask);
  if (!written) m_lastError = m_mask.lastError();
  return written;
}

bool
Volume::tagWSlice(int currslice, uchar *usermask)
{
  const bool written = m_mask.tagWSlice(currslice, usermask);
  if (!written) m_lastError = m_mask.lastError();
  return written;
}

bool
Volume::tagHSlice(int currslice, uchar *usermask)
{
  const bool written = m_mask.tagHSlice(currslice, usermask);
  if (!written) m_lastError = m_mask.lastError();
  return written;
}

bool
Volume::saveModifiedOriginalVolume()
{
  m_pvlFileManager.setMemChanged(true);
  const bool saved = m_pvlFileManager.saveMemFile();
  if (!saved) m_lastError = m_pvlFileManager.lastError();
  return saved;
}

QString
Volume::lastError() const
{
  if (!m_lastError.isEmpty())
    return m_lastError;
  const QString volumeError = m_pvlFileManager.lastError();
  return volumeError.isEmpty() ? m_mask.lastError() : volumeError;
}


void
Volume::findStartEndForTag(int tag,
			   int &minD, int &maxD,
			   int &minW, int &maxW,
			   int &minH, int &maxH)
{
  uchar *maskData = memMaskDataPtr();
  
  minD = 0;
  maxD = m_depth-1;
  minW = 0;
  maxW = m_width-1;
  minH = 0;
  maxH = m_height-1;

  if (!maskData)
    {
      minD = maxD = minW = maxW = minH = maxH = 0;
      m_lastError = m_mask.lastError();
      return;
    }

  const int maskBytes = Global::bytesPerMask();
  if (maskBytes != 1 && maskBytes != 2)
    {
      minD = maxD = minW = maxW = minH = maxH = 0;
      m_lastError = QString("Unsupported mask element size %1").arg(maskBytes);
      return;
    }
  const ushort *maskDataUS = reinterpret_cast<const ushort*>(maskData);
  auto maskValueAt = [&](int d, int w, int h) -> int
    {
      const qint64 index =
        (static_cast<qint64>(d)*m_width+w)*m_height+h;
      return maskBytes == 1 ? maskData[index] : maskDataUS[index];
    };

  bool ok;

  //--------------
  ok = false;
  for(int d=0; d<m_depth; d++)
    {
      for(int w=0; w<m_width; w++)
      for(int h=0; h<m_height; h++)
	{
	  if (maskValueAt(d, w, h) == tag)
	    {
	      minD = d;
	      ok = true;
	      break;
	    }
	}
      if (ok)
	break;
    }
  ok = false;
  for(int d=m_depth-1; d>minD; d--)
    {
      for(int w=0; w<m_width; w++)
      for(int h=0; h<m_height; h++)
	{
	  if (maskValueAt(d, w, h) == tag)
	    {
	      maxD = d;
	      ok = true;
	      break;
	    }
	}
      if (ok)
	break;
    }
  //--------------

  //--------------
  ok = false;
  for(int w=0; w<m_width; w++)
    {
      for(int d=minD; d<=maxD; d++)
      for(int h=0; h<m_height; h++)
	{
	  if (maskValueAt(d, w, h) == tag)
	    {
	      minW = w;
	      ok = true;
	      break;
	    }
	}
      if (ok)
	break;
    }
  ok = false;
  for(int w=m_width-1; w>minW; w--)
    {
      for(int d=minD; d<=maxD; d++)
      for(int h=0; h<m_height; h++)
	{
	  if (maskValueAt(d, w, h) == tag)
	    {
	      maxW = w;
	      ok = true;
	      break;
	    }
	}
      if (ok)
	break;
    }
  //--------------

  //--------------
  ok = false;
  for(int h=0; h<m_height; h++)
    {
      for(int d=minD; d<=maxD; d++)
	for(int w=minW; w<=maxW; w++)
	{
	  if (maskValueAt(d, w, h) == tag)
	    {
	      minH = h;
	      ok = true;
	      break;
	    }
	}
      if (ok)
	break;
    }
  ok = false;
  for(int h=m_height-1; h>minH; h--)
    {
      for(int d=minD; d<=maxD; d++)
	for(int w=minW; w<=maxW; w++)
	{
	  if (maskValueAt(d, w, h) == tag)
	    {
	      maxH = h;
	      ok = true;
	      break;
	    }
	}
      if (ok)
	break;
    }
  //--------------
}
