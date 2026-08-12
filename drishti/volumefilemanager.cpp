#include "volumefilemanager.h"
#include <QtGui>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>

#include <limits>
#include <new>

VolumeFileManager::VolumeFileManager()
{
  m_slice = 0;
  m_sliceCapacity = 0;
  m_block = 0;
  m_blockCapacity = 0;
  m_blockSlices = 10;
  m_startBlock = m_endBlock = 0;
  m_filenames.clear();
  m_volData = 0;
  m_volDataCapacity = 0;
  m_memmapped = false;
  reset();
}

VolumeFileManager::~VolumeFileManager() { reset(); }

void VolumeFileManager::setMemMapped(bool b)
{
  m_memmapped = b;

  if (m_volData)
    delete [] m_volData;
  m_volData = 0;
  m_volDataCapacity = 0;

  m_memChanged = false;
}

bool VolumeFileManager::isMemMapped() { return m_memmapped; }

void VolumeFileManager::setMemChanged(bool b) { m_memChanged = b; }

void
VolumeFileManager::reset()
{
  m_baseFilename.clear();
  m_filenames.clear();
  m_header = m_slabSize = 0;
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;
  m_lastError.clear();

  m_filename.clear();
  m_slabno = m_prevslabno = -1;

  if (m_slice)
    delete [] m_slice;
  m_slice = 0;
  m_sliceCapacity = 0;

  if (m_block)
    delete [] m_block;
  m_block = 0;
  m_blockCapacity = 0;
  m_startBlock = m_endBlock = 0;

  if (m_volData)
    delete [] m_volData;
  m_volData = 0;
  m_volDataCapacity = 0;

  if (m_qfile.isOpen())
    m_qfile.close();

  m_memmapped = false;
  m_memChanged = false;
}

QString
VolumeFileManager::lastError() const
{
  return m_lastError;
}

bool
VolumeFileManager::checkedMultiply(qint64 a, qint64 b, qint64& result)
{
  if (a < 0 || b < 0)
    return false;

  if (a == 0 || b == 0)
    {
      result = 0;
      return true;
    }

  if (a > std::numeric_limits<qint64>::max()/b)
    return false;

  result = a*b;
  return true;
}

bool
VolumeFileManager::checkedAdd(qint64 a, qint64 b, qint64& result)
{
  if (a < 0 || b < 0 ||
      a > std::numeric_limits<qint64>::max()-b)
    return false;

  result = a+b;
  return true;
}

bool
VolumeFileManager::setError(const QString& error)
{
  m_lastError = error;
  return false;
}

void
VolumeFileManager::clearError()
{
  m_lastError.clear();
}

bool
VolumeFileManager::validateGeometry(const QString& operation)
{
  if (m_depth <= 0 || m_width <= 0 || m_height <= 0)
    return setError(QString("%1: invalid volume dimensions %2 x %3 x %4")
                    .arg(operation).arg(m_depth).arg(m_width).arg(m_height));

  if (m_slabSize <= 0 || m_slabSize > std::numeric_limits<int>::max())
    return setError(QString("%1: invalid slab size %2")
                    .arg(operation).arg(m_slabSize));

  if (m_header < 0)
    return setError(QString("%1: invalid header size %2")
                    .arg(operation).arg(m_header));

  if (m_voxelType < _UChar || m_voxelType > _Float ||
      (m_bytesPerVoxel != 1 &&
       m_bytesPerVoxel != 2 &&
       m_bytesPerVoxel != 4))
    return setError(QString("%1: invalid voxel type %2")
                    .arg(operation).arg(m_voxelType));

  if (m_filenames.isEmpty() && m_baseFilename.isEmpty())
    return setError(QString("%1: no volume filename is configured")
                    .arg(operation));

  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1 + (m_depth-1)/slabSize;
  if (m_baseFilename.isEmpty() && m_filenames.count() < nslabs)
    return setError(QString("%1: only %2 of %3 slab filenames are configured")
                    .arg(operation).arg(m_filenames.count()).arg(nslabs));
  for(int slab=0; slab<qMin(nslabs, m_filenames.count()); ++slab)
    {
      if (m_filenames[slab].isEmpty())
        return setError(QString("%1: slab %2 has an empty filename")
                        .arg(operation).arg(slab));
    }

  return true;
}

bool
VolumeFileManager::sliceByteCount(qint64& bytes, const QString& operation)
{
  qint64 voxels = 0;
  if (!checkedMultiply(static_cast<qint64>(m_width),
                       static_cast<qint64>(m_height), voxels) ||
      !checkedMultiply(voxels, m_bytesPerVoxel, bytes) ||
      bytes <= 0)
    return setError(QString("%1: slice byte count overflows")
                    .arg(operation));

  return true;
}

bool
VolumeFileManager::volumeByteCount(qint64& bytes, const QString& operation)
{
  qint64 sliceBytes = 0;
  if (!sliceByteCount(sliceBytes, operation) ||
      !checkedMultiply(sliceBytes, static_cast<qint64>(m_depth), bytes) ||
      bytes <= 0)
    return setError(QString("%1: volume byte count overflows")
                    .arg(operation));

  return true;
}

bool
VolumeFileManager::slabFileSize(int slab,
                                qint64 sliceBytes,
                                qint64& bytes,
                                const QString& operation)
{
  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1 + (m_depth-1)/slabSize;
  if (slab < 0 || slab >= nslabs)
    return setError(QString("%1: slab index %2 is outside [0, %3)")
                    .arg(operation).arg(slab).arg(nslabs));

  const qint64 firstSlice = static_cast<qint64>(slab)*slabSize;
  const int slices = qMin(slabSize,
                          m_depth-static_cast<int>(firstSlice));
  qint64 dataBytes = 0;
  if (!checkedMultiply(static_cast<qint64>(slices),
                       sliceBytes, dataBytes) ||
      !checkedAdd(m_header, dataBytes, bytes))
    return setError(QString("%1: slab file size overflows").arg(operation));

  return true;
}

bool
VolumeFileManager::ensureSliceCapacity(qint64 bytes,
                                       const QString& operation)
{
  if (bytes <= 0 ||
      static_cast<quint64>(bytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: requested slice buffer is too large")
                    .arg(operation));

  const size_t requested = static_cast<size_t>(bytes);
  if (m_slice && m_sliceCapacity >= requested)
    return true;

  uchar *replacement = new (std::nothrow) uchar[requested];
  if (!replacement)
    return setError(QString("%1: cannot allocate %2-byte slice buffer")
                    .arg(operation).arg(bytes));

  delete [] m_slice;
  m_slice = replacement;
  m_sliceCapacity = requested;
  return true;
}

bool
VolumeFileManager::ensureBlockCapacity(qint64 bytes,
                                       const QString& operation)
{
  if (bytes <= 0 ||
      static_cast<quint64>(bytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: requested interpolation buffer is too large")
                    .arg(operation));

  const size_t requested = static_cast<size_t>(bytes);
  if (m_block && m_blockCapacity >= requested)
    return true;

  uchar *replacement = new (std::nothrow) uchar[requested];
  if (!replacement)
    return setError(QString("%1: cannot allocate %2-byte interpolation buffer")
                    .arg(operation).arg(bytes));

  delete [] m_block;
  m_block = replacement;
  m_blockCapacity = requested;
  m_startBlock = m_endBlock = 0;
  return true;
}

QString
VolumeFileManager::slabFilename(int slab) const
{
  if (slab >= 0 && slab < m_filenames.count())
    return m_filenames[slab];

  return m_baseFilename +
         QString(".%1").arg(slab+1, 3, 10, QChar('0'));
}

bool
VolumeFileManager::openSlab(int slab,
                            QIODevice::OpenMode mode,
                            const QString& operation)
{
  if (slab < 0)
    return setError(QString("%1: invalid slab index %2")
                    .arg(operation).arg(slab));

  if (m_qfile.isOpen())
    m_qfile.close();

  m_filename = slabFilename(slab);
  m_qfile.setFileName(m_filename);
  if (!m_qfile.open(mode))
    return setError(QString("%1: cannot open '%2': %3")
                    .arg(operation).arg(m_filename).arg(m_qfile.errorString()));

  return true;
}

bool
VolumeFileManager::seekFile(QFile& file,
                            qint64 offset,
                            const QString& operation)
{
  if (offset < 0 || !file.seek(offset))
    return setError(QString("%1: cannot seek '%2' to byte %3: %4")
                    .arg(operation).arg(file.fileName()).arg(offset)
                    .arg(file.errorString()));

  return true;
}

bool
VolumeFileManager::readExact(QFile& file,
                             uchar *destination,
                             qint64 bytes,
                             const QString& operation)
{
  if (!destination || bytes < 0)
    return setError(QString("%1: invalid read buffer")
                    .arg(operation));

  qint64 done = 0;
  while (done < bytes)
    {
      const qint64 count = file.read(reinterpret_cast<char*>(destination) + done,
                                     bytes-done);
      if (count <= 0)
        return setError(QString("%1: short read from '%2' (%3 of %4 bytes): %5")
                        .arg(operation).arg(file.fileName()).arg(done).arg(bytes)
                        .arg(file.errorString()));
      done += count;
    }

  return true;
}

bool
VolumeFileManager::writeExact(QFile& file,
                              const uchar *source,
                              qint64 bytes,
                              const QString& operation)
{
  if (!source || bytes < 0)
    return setError(QString("%1: invalid write buffer")
                    .arg(operation));

  qint64 done = 0;
  while (done < bytes)
    {
      const qint64 count = file.write(reinterpret_cast<const char*>(source) + done,
                                      bytes-done);
      if (count <= 0)
        return setError(QString("%1: short write to '%2' (%3 of %4 bytes): %5")
                        .arg(operation).arg(file.fileName()).arg(done).arg(bytes)
                        .arg(file.errorString()));
      done += count;
    }

  return true;
}

bool
VolumeFileManager::flushAndCheckSize(QFile& file,
                                     qint64 expectedSize,
                                     const QString& operation)
{
  if (!file.flush())
    return setError(QString("%1: cannot flush '%2': %3")
                    .arg(operation).arg(file.fileName()).arg(file.errorString()));

  const qint64 actualSize = file.size();
  if (actualSize != expectedSize)
    return setError(QString("%1: '%2' has %3 bytes, expected %4")
                    .arg(operation).arg(file.fileName())
                    .arg(actualSize).arg(expectedSize));

  return true;
}

void
VolumeFileManager::cleanupPartialFiles(const QStringList& filenames)
{
  QStringList failures;
  for(int i=0; i<filenames.count(); ++i)
    {
      if (QFileInfo::exists(filenames[i]) && !QFile::remove(filenames[i]))
        failures << filenames[i];
    }

  if (!failures.isEmpty())
    {
      const QString cleanupError =
        QString("cleanup failed for: %1").arg(failures.join(", "));
      if (m_lastError.isEmpty())
        m_lastError = cleanupError;
      else
        m_lastError += QString("; %1").arg(cleanupError);
    }
}

void VolumeFileManager::closeQFile()
{
  if (m_qfile.isOpen())
    m_qfile.close();
}

int VolumeFileManager::depth() { return m_depth; }
int VolumeFileManager::width() { return m_width; }
int VolumeFileManager::height() { return m_height; }

void VolumeFileManager::setFilenameList(QStringList flist) { m_filenames = flist; }
void VolumeFileManager::setBaseFilename(QString bfn) { m_baseFilename = bfn; }
void VolumeFileManager::setDepth(int d) { m_depth = d; }
void VolumeFileManager::setWidth(int w) { m_width = w; }
void VolumeFileManager::setHeight(int h) { m_height = h; }
void VolumeFileManager::setHeaderSize(int hs) { m_header = hs; }
void VolumeFileManager::setSlabSize(int ss) { m_slabSize = ss; }
void VolumeFileManager::setVoxelType(int vt)
{
  m_voxelType = vt;
  m_bytesPerVoxel = 0;
  if (m_voxelType == _UChar || m_voxelType == _Char)
    m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort || m_voxelType == _Short)
    m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int || m_voxelType == _Float)
    m_bytesPerVoxel = 4;
}

QStringList VolumeFileManager::filenameList() { return m_filenames; }
QString VolumeFileManager::baseFilename() { return m_baseFilename; }
int VolumeFileManager::headerSize() { return m_header; }
int VolumeFileManager::slabSize() { return m_slabSize; }
QString VolumeFileManager::fileName() { return m_filename; }

void
VolumeFileManager::removeFile()
{
  clearError();
  if (!validateGeometry("remove volume files"))
    return;

  const int nslabs = 1 + (m_depth-1)/static_cast<int>(m_slabSize);
  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);

      QFile::remove(m_filename);
    }

  reset();
}

int VolumeFileManager::bytesPerVoxel() { return m_bytesPerVoxel; }
int VolumeFileManager::voxelType() { return m_voxelType; }

int
VolumeFileManager::readVoxelType()
{
  clearError();
  uchar vt = 0;
  if (m_qfile.isOpen())
    {
      if (!m_qfile.isReadable() ||
          !seekFile(m_qfile, 0, "read voxel type") ||
          !readExact(m_qfile, &vt, 1, "read voxel type"))
        {
          m_qfile.close();
          return -1;
        }
      m_qfile.close();
    }
  else
    {
      if (m_filenames.count() > 0)
	m_qfile.setFileName(m_filenames[0]);
      else
	m_qfile.setFileName(m_baseFilename + ".001");

      if (!m_qfile.open(QFile::ReadOnly))
        {
          setError(QString("read voxel type: cannot open '%1': %2")
                   .arg(m_qfile.fileName()).arg(m_qfile.errorString()));
          return -1;
        }
      if (!readExact(m_qfile, &vt, 1, "read voxel type"))
        {
          m_qfile.close();
          return -1;
        }
      m_qfile.close();
    }
  return vt;
}

bool
VolumeFileManager::exists()
{
  clearError();
  if (m_qfile.isOpen())
    m_qfile.close();
  if (!validateGeometry("check volume files"))
    return false;

  qint64 bps = 0;
  if (!sliceByteCount(bps, "check volume files"))
    return false;

  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1 + (m_depth-1)/slabSize;

  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);

      const qint64 firstSlice = static_cast<qint64>(ns)*slabSize;
      const int nslices = qMin(slabSize,
                               m_depth-static_cast<int>(firstSlice));
      qint64 fsize = 0;
      qint64 expectedSize = 0;
      if (!checkedMultiply(static_cast<qint64>(nslices), bps, fsize) ||
          !checkedAdd(m_header, fsize, expectedSize))
        return setError("check volume files: file size overflows");

      m_qfile.setFileName(m_filename);
			       
      if (m_qfile.exists() == false ||
	  m_qfile.size() != expectedSize)
	return setError(QString("check volume files: '%1' is missing or has an unexpected size")
                        .arg(m_filename));
    }

  return true;
}

bool
VolumeFileManager::createFile(bool writeHeader, bool writeData)
{
  clearError();
  if (!validateGeometry("create volume files"))
    return false;

  qint64 bps = 0;
  if (!sliceByteCount(bps, "create volume files"))
    return false;

  if (writeData)
    {
      if (!ensureSliceCapacity(bps, "create volume files"))
        return false;
      memset(m_slice, 0, static_cast<size_t>(bps));
    }

  m_slabno = m_prevslabno = -1;
  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1 + (m_depth-1)/slabSize;
  
  uchar vt = 0;
  if (m_voxelType == _Char) vt = 1;
  else if (m_voxelType == _UShort) vt = 2;
  else if (m_voxelType == _Short) vt = 3;
  else if (m_voxelType == _Int) vt = 4;
  else if (m_voxelType == _Float) vt = 8;

  if (writeHeader)
    m_header = 13;

  QStringList touchedFiles;

  QProgressDialog progress(QString("Allocating space for\n%1\non disk").\
			   arg(m_baseFilename),
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);

      progress.setLabelText(m_filename);
      if (qApp)
        qApp->processEvents();

      if (m_qfile.isOpen())
	m_qfile.close();

      m_qfile.setFileName(m_filename);
      if (!m_qfile.open(QFile::WriteOnly | QFile::Truncate))
        {
          setError(QString("create volume files: cannot open '%1': %2")
                   .arg(m_filename).arg(m_qfile.errorString()));
          break;
        }
      touchedFiles << m_filename;

      const qint64 firstSlice = static_cast<qint64>(ns)*slabSize;
      const int nslices = qMin(slabSize,
                               m_depth-static_cast<int>(firstSlice));
      bool ok = true;
      if (writeHeader)
	{
	  const qint32 fileSlices = nslices;
	  const qint32 fileWidth = m_width;
	  const qint32 fileHeight = m_height;
	  ok = writeExact(m_qfile, &vt, 1, "create volume header") &&
	       writeExact(m_qfile,
                          reinterpret_cast<const uchar*>(&fileSlices), 4,
                          "create volume header") &&
	       writeExact(m_qfile,
                          reinterpret_cast<const uchar*>(&fileWidth), 4,
                          "create volume header") &&
	       writeExact(m_qfile,
                          reinterpret_cast<const uchar*>(&fileHeight), 4,
                          "create volume header");
	}

      progress.setValue(10);

      if (ok && writeData)
	{
	  for(int t=0; t<nslices && ok; t++)
	    {
	      ok = writeExact(m_qfile, m_slice, bps,
                              "initialize volume data");
	      progress.setValue((int)(100*(float)t/(float)nslices));
	      if (qApp)
	        qApp->processEvents();
	    }
	}

      qint64 dataBytes = 0;
      qint64 expectedSize = writeHeader ? 13 : 0;
      if (ok && writeData &&
          (!checkedMultiply(static_cast<qint64>(nslices), bps, dataBytes) ||
           !checkedAdd(expectedSize, dataBytes, expectedSize)))
        ok = setError("create volume files: final file size overflows");

      if (ok)
        ok = flushAndCheckSize(m_qfile, expectedSize,
                               "create volume files");

      m_qfile.close();
      if (!ok)
        break;
    }

  if (!m_lastError.isEmpty())
    {
      if (m_qfile.isOpen())
        m_qfile.close();
      cleanupPartialFiles(touchedFiles);
      return false;
    }

  progress.setValue(100);

  if (m_memmapped)
    {
      if (!createMemFile())
        {
          cleanupPartialFiles(touchedFiles);
          return false;
        }
    }

  return true;
}

uchar*
VolumeFileManager::getSlice(int d)
{
  const QString operation = "read depth slice";
  clearError();
  if (!validateGeometry(operation) || d < 0 || d >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: depth index %2 is outside [0, %3)")
                 .arg(operation).arg(d).arg(m_depth));
      return 0;
    }

  qint64 bps = 0;
  if (!sliceByteCount(bps, operation) ||
      !ensureSliceCapacity(bps, operation))
    return 0;
  memset(m_slice, 0, static_cast<size_t>(bps));

  const int slabSize = static_cast<int>(m_slabSize);
  m_slabno = d/slabSize;
  qint64 localBytes = 0;
  qint64 offset = 0;
  if (!checkedMultiply(static_cast<qint64>(d-m_slabno*slabSize),
                       bps, localBytes) ||
      !checkedAdd(m_header, localBytes, offset))
    {
      setError(QString("%1: file offset overflows").arg(operation));
      return 0;
    }

  const bool ok = openSlab(m_slabno, QFile::ReadOnly, operation) &&
                  seekFile(m_qfile, offset, operation) &&
                  readExact(m_qfile, m_slice, bps, operation);
  m_qfile.close();
  if (!ok)
    {
      memset(m_slice, 0, static_cast<size_t>(bps));
      return 0;
    }

  return m_slice;
}

uchar*
VolumeFileManager::getWidthSlice(int w)
{
  const QString operation = "read width slice";
  clearError();
  if (!validateGeometry(operation) || w < 0 || w >= m_width)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: width index %2 is outside [0, %3)")
                 .arg(operation).arg(w).arg(m_width));
      return 0;
    }

  qint64 bps = 0;
  qint64 rowBytes = 0;
  qint64 planeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_height),
                       m_bytesPerVoxel, rowBytes) ||
      !checkedMultiply(static_cast<qint64>(m_depth),
                       rowBytes, planeBytes) ||
      !ensureSliceCapacity(planeBytes, operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: plane byte count overflows").arg(operation));
      return 0;
    }
  memset(m_slice, 0, static_cast<size_t>(planeBytes));

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  for(int d=0; d<m_depth; ++d)
    {
      const int slab = d/slabSize;
      if (previousSlab != slab)
        {
          if (m_qfile.isOpen())
            m_qfile.close();
          if (!openSlab(slab, QFile::ReadOnly, operation))
            {
              memset(m_slice, 0, static_cast<size_t>(planeBytes));
              return 0;
            }
          previousSlab = slab;
        }

      qint64 sliceOffset = 0;
      qint64 widthOffset = 0;
      qint64 dataOffset = 0;
      qint64 fileOffset = 0;
      qint64 outputOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(d-slab*slabSize),
                           bps, sliceOffset) ||
          !checkedMultiply(static_cast<qint64>(w),
                           rowBytes, widthOffset) ||
          !checkedAdd(sliceOffset, widthOffset, dataOffset) ||
          !checkedAdd(m_header, dataOffset, fileOffset) ||
          !checkedMultiply(static_cast<qint64>(d),
                           rowBytes, outputOffset) ||
          !seekFile(m_qfile, fileOffset, operation) ||
          !readExact(m_qfile, m_slice+outputOffset,
                     rowBytes, operation))
        {
          if (m_lastError.isEmpty())
            setError(QString("%1: byte offset overflows").arg(operation));
          m_qfile.close();
          memset(m_slice, 0, static_cast<size_t>(planeBytes));
          return 0;
        }
    }
  m_qfile.close();

  return m_slice;
}

uchar*
VolumeFileManager::getHeightSlice(int h)
{
  const QString operation = "read height slice";
  clearError();
  if (!validateGeometry(operation) || h < 0 || h >= m_height)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: height index %2 is outside [0, %3)")
                 .arg(operation).arg(h).arg(m_height));
      return 0;
    }

  qint64 bps = 0;
  qint64 planeVoxels = 0;
  qint64 planeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_depth),
                       static_cast<qint64>(m_width), planeVoxels) ||
      !checkedMultiply(planeVoxels, m_bytesPerVoxel, planeBytes) ||
      !ensureSliceCapacity(planeBytes, operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: plane byte count overflows").arg(operation));
      return 0;
    }
  memset(m_slice, 0, static_cast<size_t>(planeBytes));

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  for(int d=0; d<m_depth; ++d)
    {
      const int slab = d/slabSize;
      if (previousSlab != slab)
        {
          if (m_qfile.isOpen())
            m_qfile.close();
          if (!openSlab(slab, QFile::ReadOnly, operation))
            {
              memset(m_slice, 0, static_cast<size_t>(planeBytes));
              return 0;
            }
          previousSlab = slab;
        }

      for(int w=0; w<m_width; ++w)
        {
          qint64 sliceOffset = 0;
          qint64 rowVoxel = 0;
          qint64 voxelOffset = 0;
          qint64 dataOffset = 0;
          qint64 fileOffset = 0;
          qint64 outputVoxel = 0;
          qint64 outputOffset = 0;
          if (!checkedMultiply(static_cast<qint64>(d-slab*slabSize),
                               bps, sliceOffset) ||
              !checkedMultiply(static_cast<qint64>(w),
                               static_cast<qint64>(m_height), rowVoxel) ||
              !checkedAdd(rowVoxel, static_cast<qint64>(h), voxelOffset) ||
              !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
              !checkedAdd(sliceOffset, dataOffset, dataOffset) ||
              !checkedAdd(m_header, dataOffset, fileOffset) ||
              !checkedMultiply(static_cast<qint64>(d),
                               static_cast<qint64>(m_width), outputVoxel) ||
              !checkedAdd(outputVoxel, static_cast<qint64>(w), outputVoxel) ||
              !checkedMultiply(outputVoxel, m_bytesPerVoxel, outputOffset) ||
              !seekFile(m_qfile, fileOffset, operation) ||
              !readExact(m_qfile, m_slice+outputOffset,
                         m_bytesPerVoxel, operation))
            {
              if (m_lastError.isEmpty())
                setError(QString("%1: byte offset overflows").arg(operation));
              m_qfile.close();
              memset(m_slice, 0, static_cast<size_t>(planeBytes));
              return 0;
            }
        }
    }
  m_qfile.close();

  return m_slice;
}

bool
VolumeFileManager::setSlice(int d, uchar *tmp)
{
  const QString operation = "write depth slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || d < 0 || d >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: depth index %2 is outside [0, %3)")
                 .arg(operation).arg(d).arg(m_depth));
      return false;
    }

  qint64 bps = 0;
  if (!sliceByteCount(bps, operation))
    return false;

  const int slabSize = static_cast<int>(m_slabSize);
  m_slabno = d/slabSize;
  qint64 localBytes = 0;
  qint64 offset = 0;
  qint64 endOffset = 0;
  qint64 maximumSize = 0;
  if (!checkedMultiply(static_cast<qint64>(d-m_slabno*slabSize),
                       bps, localBytes) ||
      !checkedAdd(m_header, localBytes, offset) ||
      !checkedAdd(offset, bps, endOffset) ||
      !slabFileSize(m_slabno, bps, maximumSize, operation) ||
      endOffset > maximumSize)
    return setError(QString("%1: file offset overflows").arg(operation));

  if (!openSlab(m_slabno, QFile::ReadWrite, operation))
    return false;
  const qint64 previousSize = m_qfile.size();
  if (previousSize < 0 || previousSize > maximumSize)
    {
      m_qfile.close();
      return setError(QString("%1: '%2' has invalid size %3 (maximum %4)")
                      .arg(operation).arg(m_filename)
                      .arg(previousSize).arg(maximumSize));
    }
  const qint64 expectedSize = qMax(previousSize, endOffset);
  const bool ok = seekFile(m_qfile, offset, operation) &&
                  writeExact(m_qfile, tmp, bps, operation) &&
                  flushAndCheckSize(m_qfile, expectedSize, operation);
  m_qfile.close();
  return ok;
}

bool
VolumeFileManager::setWidthSlice(int w, uchar *tmp)
{
  const QString operation = "write width slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || w < 0 || w >= m_width)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: width index %2 is outside [0, %3)")
                 .arg(operation).arg(w).arg(m_width));
      return false;
    }

  qint64 bps = 0;
  qint64 rowBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_height),
                       m_bytesPerVoxel, rowBytes))
    return setError(QString("%1: row byte count overflows").arg(operation));

  if (m_qfile.isOpen())
    m_qfile.close();

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  qint64 expectedSize = 0;
  qint64 maximumSize = 0;
  for(int d=0; d<m_depth; ++d)
    {
      const int slab = d/slabSize;
      if (previousSlab != slab)
        {
          if (m_qfile.isOpen())
            {
              if (!flushAndCheckSize(m_qfile, expectedSize, operation))
                {
                  m_qfile.close();
                  return false;
                }
              m_qfile.close();
            }
          if (!openSlab(slab, QFile::ReadWrite, operation))
            return false;
          expectedSize = m_qfile.size();
          if (!slabFileSize(slab, bps, maximumSize, operation) ||
              expectedSize < 0 || expectedSize > maximumSize)
            {
              m_qfile.close();
              if (m_lastError.isEmpty())
                setError(QString("%1: '%2' has invalid size %3 (maximum %4)")
                         .arg(operation).arg(m_filename)
                         .arg(expectedSize).arg(maximumSize));
              return false;
            }
          previousSlab = slab;
        }

      qint64 sliceOffset = 0;
      qint64 widthOffset = 0;
      qint64 dataOffset = 0;
      qint64 fileOffset = 0;
      qint64 endOffset = 0;
      qint64 inputOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(d-slab*slabSize),
                           bps, sliceOffset) ||
          !checkedMultiply(static_cast<qint64>(w),
                           rowBytes, widthOffset) ||
          !checkedAdd(sliceOffset, widthOffset, dataOffset) ||
          !checkedAdd(m_header, dataOffset, fileOffset) ||
          !checkedAdd(fileOffset, rowBytes, endOffset) ||
          !checkedMultiply(static_cast<qint64>(d),
                           rowBytes, inputOffset))
        {
          m_qfile.close();
          return setError(QString("%1: byte offset overflows").arg(operation));
        }

      if (endOffset > maximumSize)
        {
          m_qfile.close();
          return setError(QString("%1: write exceeds the configured slab size")
                          .arg(operation));
        }

      expectedSize = qMax(expectedSize, endOffset);
      if (!seekFile(m_qfile, fileOffset, operation) ||
          !writeExact(m_qfile, tmp+inputOffset, rowBytes, operation))
        {
          m_qfile.close();
          return false;
        }
    }

  const bool ok = !m_qfile.isOpen() ||
                  flushAndCheckSize(m_qfile, expectedSize, operation);
  m_qfile.close();
  return ok;
}

bool
VolumeFileManager::setHeightSlice(int h, uchar *tmp)
{
  const QString operation = "write height slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || h < 0 || h >= m_height)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: height index %2 is outside [0, %3)")
                 .arg(operation).arg(h).arg(m_height));
      return false;
    }

  qint64 bps = 0;
  if (!sliceByteCount(bps, operation))
    return false;
  if (m_qfile.isOpen())
    m_qfile.close();

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  qint64 expectedSize = 0;
  qint64 maximumSize = 0;
  for(int d=0; d<m_depth; ++d)
    {
      const int slab = d/slabSize;
      if (previousSlab != slab)
        {
          if (m_qfile.isOpen())
            {
              if (!flushAndCheckSize(m_qfile, expectedSize, operation))
                {
                  m_qfile.close();
                  return false;
                }
              m_qfile.close();
            }
          if (!openSlab(slab, QFile::ReadWrite, operation))
            return false;
          expectedSize = m_qfile.size();
          if (!slabFileSize(slab, bps, maximumSize, operation) ||
              expectedSize < 0 || expectedSize > maximumSize)
            {
              m_qfile.close();
              if (m_lastError.isEmpty())
                setError(QString("%1: '%2' has invalid size %3 (maximum %4)")
                         .arg(operation).arg(m_filename)
                         .arg(expectedSize).arg(maximumSize));
              return false;
            }
          previousSlab = slab;
        }

      for(int w=0; w<m_width; ++w)
        {
          qint64 sliceOffset = 0;
          qint64 rowVoxel = 0;
          qint64 voxelOffset = 0;
          qint64 dataOffset = 0;
          qint64 fileOffset = 0;
          qint64 endOffset = 0;
          qint64 inputVoxel = 0;
          qint64 inputOffset = 0;
          if (!checkedMultiply(static_cast<qint64>(d-slab*slabSize),
                               bps, sliceOffset) ||
              !checkedMultiply(static_cast<qint64>(w),
                               static_cast<qint64>(m_height), rowVoxel) ||
              !checkedAdd(rowVoxel, static_cast<qint64>(h), voxelOffset) ||
              !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
              !checkedAdd(sliceOffset, dataOffset, dataOffset) ||
              !checkedAdd(m_header, dataOffset, fileOffset) ||
              !checkedAdd(fileOffset, m_bytesPerVoxel, endOffset) ||
              !checkedMultiply(static_cast<qint64>(d),
                               static_cast<qint64>(m_width), inputVoxel) ||
              !checkedAdd(inputVoxel, static_cast<qint64>(w), inputVoxel) ||
              !checkedMultiply(inputVoxel, m_bytesPerVoxel, inputOffset))
            {
              m_qfile.close();
              return setError(QString("%1: byte offset overflows").arg(operation));
            }

          if (endOffset > maximumSize)
            {
              m_qfile.close();
              return setError(QString("%1: write exceeds the configured slab size")
                              .arg(operation));
            }

          expectedSize = qMax(expectedSize, endOffset);
          if (!seekFile(m_qfile, fileOffset, operation) ||
              !writeExact(m_qfile, tmp+inputOffset,
                          m_bytesPerVoxel, operation))
            {
              m_qfile.close();
              return false;
            }
        }
    }

  const bool ok = !m_qfile.isOpen() ||
                  flushAndCheckSize(m_qfile, expectedSize, operation);
  m_qfile.close();
  return ok;
}

uchar*
VolumeFileManager::rawValue(int d, int w, int h)
{
  const QString operation = "read voxel";
  clearError();
  if (!validateGeometry(operation) ||
      !ensureSliceCapacity(8, operation))
    return 0;

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return m_slice;

  qint64 bps = 0;
  if (!sliceByteCount(bps, operation))
    return 0;

  const int slabSize = static_cast<int>(m_slabSize);
  m_slabno = d/slabSize;
  qint64 sliceOffset = 0;
  qint64 rowVoxel = 0;
  qint64 voxelOffset = 0;
  qint64 dataOffset = 0;
  qint64 fileOffset = 0;
  if (!checkedMultiply(static_cast<qint64>(d-m_slabno*slabSize),
                       bps, sliceOffset) ||
      !checkedMultiply(static_cast<qint64>(w),
                       static_cast<qint64>(m_height), rowVoxel) ||
      !checkedAdd(rowVoxel, static_cast<qint64>(h), voxelOffset) ||
      !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
      !checkedAdd(sliceOffset, dataOffset, dataOffset) ||
      !checkedAdd(m_header, dataOffset, fileOffset))
    {
      setError(QString("%1: file offset overflows").arg(operation));
      return 0;
    }

  const bool ok = openSlab(m_slabno, QFile::ReadOnly, operation) &&
                  seekFile(m_qfile, fileOffset, operation) &&
                  readExact(m_qfile, m_slice, m_bytesPerVoxel, operation);
  m_qfile.close();
  if (!ok)
    {
      memset(m_slice, 0, 8);
      return 0;
    }
  return m_slice;
}

#define interpVal(T)					\
  T *v[8];						\
  for(int i=0; i<8; i++)				\
    v[i] = (T*)(rv + i*m_bytesPerVoxel);		\
							\
  T vb = ((1-dd)*(1-ww)*(1-hh)*(*v[0]) +		\
	  (1-dd)*(1-ww)*(  hh)*(*v[1]) +		\
	  (1-dd)*(  ww)*(1-hh)*(*v[2]) +		\
	  (1-dd)*(  ww)*(  hh)*(*v[3]) +		\
	  (  dd)*(1-ww)*(1-hh)*(*v[4]) +		\
	  (  dd)*(1-ww)*(  hh)*(*v[5]) +		\
	  (  dd)*(  ww)*(1-hh)*(*v[6]) +		\
	  (  dd)*(  ww)*(  hh)*(*v[7]));		\
  memcpy(m_slice, &vb, sizeof(T));


uchar*
VolumeFileManager::interpolatedRawValue(float dv, float wv, float hv)
{
  const QString operation = "interpolate voxel";
  clearError();
  if (!validateGeometry(operation) ||
      !ensureSliceCapacity(8, operation))
    return 0;

  int d = dv;
  int w = wv;
  int h = hv;
  int d1 = d+1;
  int w1 = w+1;
  int h1 = h+1;
  float dd = dv-d;
  float ww = wv-w;
  float hh = hv-h;
  
  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d1 >= m_depth ||
      w < 0 || w1 >= m_width ||
      h < 0 || h1 >= m_height)
    return m_slice;

  int da[8], wa[8], ha[8];
  da[0]=d;  wa[0]=w;  ha[0]=h;
  da[1]=d;  wa[1]=w;  ha[1]=h1;
  da[2]=d;  wa[2]=w1; ha[2]=h;
  da[3]=d;  wa[3]=w1; ha[3]=h1;
  da[4]=d1; wa[4]=w;  ha[4]=h;
  da[5]=d1; wa[5]=w;  ha[5]=h1;
  da[6]=d1; wa[6]=w1; ha[6]=h;
  da[7]=d1; wa[7]=w1; ha[7]=h1;

  qint64 bps = 0;
  qint64 sampleBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(8, m_bytesPerVoxel, sampleBytes) ||
      static_cast<quint64>(sampleBytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    {
      setError(QString("%1: sample byte count overflows").arg(operation));
      return 0;
    }

  uchar *rv = new (std::nothrow) uchar[static_cast<size_t>(sampleBytes)];
  if (!rv)
    {
      setError(QString("%1: cannot allocate sample buffer").arg(operation));
      return 0;
    }
  memset(rv, 0, static_cast<size_t>(sampleBytes));

  int pslno = -1;
  const int slabSize = static_cast<int>(m_slabSize);
  for(int i=0; i<8; i++)
    {
      m_slabno = da[i]/slabSize;
      if (m_slabno != pslno)
	{
	  if (!openSlab(m_slabno, QFile::ReadOnly, operation))
            {
              delete [] rv;
              memset(m_slice, 0, 8);
              return 0;
            }
	}

      qint64 sliceOffset = 0;
      qint64 rowVoxel = 0;
      qint64 voxelOffset = 0;
      qint64 dataOffset = 0;
      qint64 fileOffset = 0;
      qint64 sampleOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(da[i]-m_slabno*slabSize),
                           bps, sliceOffset) ||
          !checkedMultiply(static_cast<qint64>(wa[i]),
                           static_cast<qint64>(m_height), rowVoxel) ||
          !checkedAdd(rowVoxel, static_cast<qint64>(ha[i]), voxelOffset) ||
          !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
          !checkedAdd(sliceOffset, dataOffset, dataOffset) ||
          !checkedAdd(m_header, dataOffset, fileOffset) ||
          !checkedMultiply(static_cast<qint64>(i),
                           m_bytesPerVoxel, sampleOffset) ||
          !seekFile(m_qfile, fileOffset, operation) ||
          !readExact(m_qfile, rv+sampleOffset,
                     m_bytesPerVoxel, operation))
        {
          if (m_lastError.isEmpty())
            setError(QString("%1: byte offset overflows").arg(operation));
          delete [] rv;
          m_qfile.close();
          memset(m_slice, 0, 8);
          return 0;
        }

      pslno = m_slabno;
    }
  
  if (m_voxelType == _UChar)
    {
      interpVal(uchar);
    }
  else if (m_voxelType == _Char)
    {
      interpVal(char);
    }
  else if (m_voxelType == _UShort)
    {
      interpVal(ushort);
    }
  else if (m_voxelType == _Short)
    {
      interpVal(short);
    }
  else if (m_voxelType == _Int)
    {
      interpVal(int);
    }
  else if (m_voxelType == _Float)
    {
      interpVal(float);
    }
  
  delete [] rv;

  m_qfile.close();

  return m_slice;
}

void
VolumeFileManager::startBlockInterpolation()
{
  const QString operation = "start block interpolation";
  clearError();
  if (!validateGeometry(operation))
    return;

  qint64 bps = 0;
  qint64 blockBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_blockSlices),
                       bps, blockBytes) ||
      !ensureBlockCapacity(blockBytes, operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: buffer byte count overflows").arg(operation));
      return;
    }

  memset(m_block, 0, static_cast<size_t>(blockBytes));
  readBlocks(0);
}

void
VolumeFileManager::endBlockInterpolation()
{
  if (m_block)
    delete [] m_block;

  m_block = 0;
  m_blockCapacity = 0;
  m_startBlock = m_endBlock = 0;
}

bool
VolumeFileManager::readBlocks(int d)
{
  const QString operation = "read interpolation block";
  if (!validateGeometry(operation))
    return false;
  if (d > std::numeric_limits<int>::max()-m_blockSlices)
    return setError(QString("%1: block range overflows").arg(operation));

  qint64 bps = 0;
  qint64 blockBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_blockSlices),
                       bps, blockBytes) ||
      !ensureBlockCapacity(blockBytes, operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: buffer byte count overflows").arg(operation));
      return false;
    }
  memset(m_block, 0, static_cast<size_t>(blockBytes));

  m_startBlock = d;
  m_endBlock = d+m_blockSlices;

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  for(int blockIndex=0; blockIndex<m_blockSlices; ++blockIndex)
    {
      const qint64 volumeDepth = static_cast<qint64>(d)+blockIndex;
      qint64 outputOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(blockIndex),
                           bps, outputOffset))
        {
          setError(QString("%1: block offset overflows").arg(operation));
          m_qfile.close();
          memset(m_block, 0, static_cast<size_t>(blockBytes));
          return false;
        }

      if (volumeDepth < 0 || volumeDepth >= m_depth)
        continue;

      const int slab = static_cast<int>(volumeDepth)/slabSize;
      if (slab != previousSlab)
        {
          if (m_qfile.isOpen())
            m_qfile.close();
          if (!openSlab(slab, QFile::ReadOnly, operation))
            {
              memset(m_block, 0, static_cast<size_t>(blockBytes));
              return false;
            }
          previousSlab = slab;
        }

      qint64 sliceOffset = 0;
      qint64 fileOffset = 0;
      if (!checkedMultiply(volumeDepth-static_cast<qint64>(slab)*slabSize,
                           bps, sliceOffset) ||
          !checkedAdd(m_header, sliceOffset, fileOffset) ||
          !seekFile(m_qfile, fileOffset, operation) ||
          !readExact(m_qfile, m_block+outputOffset, bps, operation))
        {
          if (m_lastError.isEmpty())
            setError(QString("%1: byte offset overflows").arg(operation));
          m_qfile.close();
          memset(m_block, 0, static_cast<size_t>(blockBytes));
          return false;
        }
    }

  m_qfile.close();
  return true;
}

uchar*
VolumeFileManager::blockInterpolatedRawValue(float dv, float wv, float hv)
{
  const QString operation = "interpolate voxel block";
  clearError();
  if (!validateGeometry(operation) ||
      !ensureSliceCapacity(8, operation))
    return 0;

  int d = dv;
  int w = wv;
  int h = hv;
  int d1 = d+1;
  int w1 = w+1;
  int h1 = h+1;
  float dd = dv-d;
  float ww = wv-w;
  float hh = hv-h;
  
  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d1 >= m_depth ||
      w < 0 || w1 >= m_width ||
      h < 0 || h1 >= m_height)
    return m_slice;

  int da[8], wa[8], ha[8];
  da[0]=d;  wa[0]=w;  ha[0]=h;
  da[1]=d;  wa[1]=w;  ha[1]=h1;
  da[2]=d;  wa[2]=w1; ha[2]=h;
  da[3]=d;  wa[3]=w1; ha[3]=h1;
  da[4]=d1; wa[4]=w;  ha[4]=h;
  da[5]=d1; wa[5]=w;  ha[5]=h1;
  da[6]=d1; wa[6]=w1; ha[6]=h;
  da[7]=d1; wa[7]=w1; ha[7]=h1;

  qint64 bps = 0;
  qint64 sampleBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(8, m_bytesPerVoxel, sampleBytes) ||
      static_cast<quint64>(sampleBytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    {
      setError(QString("%1: sample byte count overflows").arg(operation));
      return 0;
    }

  uchar *rv = new (std::nothrow) uchar[static_cast<size_t>(sampleBytes)];
  if (!rv)
    {
      setError(QString("%1: cannot allocate sample buffer").arg(operation));
      return 0;
    }
  memset(rv, 0, static_cast<size_t>(sampleBytes));

  if (!m_block)
    {
      if (!readBlocks(da[0]))
        {
          delete [] rv;
          memset(m_slice, 0, 8);
          return 0;
        }
    }

  for(int i=0; i<8; i++)
    {
      if (da[i] < m_startBlock ||
	  da[i] >= m_endBlock)
	{
          if (!readBlocks(da[i]))
            {
              delete [] rv;
              memset(m_slice, 0, 8);
              return 0;
            }
        }

      qint64 blockSliceOffset = 0;
      qint64 rowVoxel = 0;
      qint64 voxelOffset = 0;
      qint64 dataOffset = 0;
      qint64 sampleOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(da[i]-m_startBlock),
                           bps, blockSliceOffset) ||
          !checkedMultiply(static_cast<qint64>(wa[i]),
                           static_cast<qint64>(m_height), rowVoxel) ||
          !checkedAdd(rowVoxel, static_cast<qint64>(ha[i]), voxelOffset) ||
          !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
          !checkedAdd(blockSliceOffset, dataOffset, dataOffset) ||
          !checkedMultiply(static_cast<qint64>(i),
                           m_bytesPerVoxel, sampleOffset))
        {
          delete [] rv;
          memset(m_slice, 0, 8);
          setError(QString("%1: byte offset overflows").arg(operation));
          return 0;
        }

      memcpy(rv+sampleOffset, m_block+dataOffset,
	     static_cast<size_t>(m_bytesPerVoxel));
    }
  
  if (m_voxelType == _UChar)
    {
      interpVal(uchar);
    }
  else if (m_voxelType == _Char)
    {
      interpVal(char);
    }
  else if (m_voxelType == _UShort)
    {
      interpVal(ushort);
    }
  else if (m_voxelType == _Short)
    {
      interpVal(short);
    }
  else if (m_voxelType == _Int)
    {
      interpVal(int);
    }
  else if (m_voxelType == _Float)
    {
      interpVal(float);
    }
  
  delete [] rv;

  return m_slice;
}

bool
VolumeFileManager::saveMemFile()
{
  const QString operation = "save memory volume";
  clearError();
  if (!m_memChanged)
    return true;

  if (!validateGeometry(operation))
    return false;

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation))
    return false;
  if (static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: volume is too large for this process")
                    .arg(operation));
  if (!m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    return setError(QString("%1: memory volume is not allocated")
                    .arg(operation));

  uchar vt = 0;
  if (m_voxelType == _Char) vt = 1;
  else if (m_voxelType == _UShort) vt = 2;
  else if (m_voxelType == _Short) vt = 3;
  else if (m_voxelType == _Int) vt = 4;
  else if (m_voxelType == _Float) vt = 8;

  QProgressDialog progress(QString("Saving %1").\
			   arg(m_baseFilename),
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  m_header = 13;
  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1 + (m_depth-1)/slabSize;
  QStringList touchedFiles;
  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);

      progress.setLabelText(m_filename);
      if (qApp)
        qApp->processEvents();

      if (m_qfile.isOpen())
	m_qfile.close();

      m_qfile.setFileName(m_filename);
      if (!m_qfile.open(QFile::WriteOnly | QFile::Truncate))
        {
          setError(QString("%1: cannot open '%2': %3")
                   .arg(operation).arg(m_filename).arg(m_qfile.errorString()));
          break;
        }
      touchedFiles << m_filename;

      const qint64 firstSlice = static_cast<qint64>(ns)*slabSize;
      const int nslices = qMin(slabSize,
                               m_depth-static_cast<int>(firstSlice));
      const qint32 fileSlices = nslices;
      const qint32 fileWidth = m_width;
      const qint32 fileHeight = m_height;
      bool ok = writeExact(m_qfile, &vt, 1, operation) &&
                writeExact(m_qfile,
                           reinterpret_cast<const uchar*>(&fileSlices), 4,
                           operation) &&
                writeExact(m_qfile,
                           reinterpret_cast<const uchar*>(&fileWidth), 4,
                           operation) &&
                writeExact(m_qfile,
                           reinterpret_cast<const uchar*>(&fileHeight), 4,
                           operation);

      qint64 slabBytes = 0;
      qint64 sourceOffset = 0;
      qint64 expectedSize = 0;
      if (ok &&
          (!checkedMultiply(static_cast<qint64>(nslices), bps, slabBytes) ||
           !checkedMultiply(firstSlice, bps, sourceOffset) ||
           !checkedAdd(m_header, slabBytes, expectedSize)))
        ok = setError(QString("%1: byte count overflows").arg(operation));
      if (ok)
        ok = writeExact(m_qfile, m_volData+sourceOffset,
                        slabBytes, operation) &&
             flushAndCheckSize(m_qfile, expectedSize, operation);

      m_qfile.close();
      if (!ok)
        break;

      progress.setValue((int)(100.0*(firstSlice+nslices)/m_depth));
      if (qApp)
        qApp->processEvents();
    }

  if (!m_lastError.isEmpty())
    {
      if (m_qfile.isOpen())
        m_qfile.close();
      cleanupPartialFiles(touchedFiles);
      return false;
    }

  progress.setValue(100);

  m_memChanged = false;
  return true;
}

bool
VolumeFileManager::loadMemFile()
{
  const QString operation = "load memory volume";
  clearError();
  if (!m_memmapped)
    return true;

  if (!validateGeometry(operation))
    return false;

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation))
    return false;
  if (static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: volume is too large for this process")
                    .arg(operation));

  uchar *replacement =
    new (std::nothrow) uchar[static_cast<size_t>(volumeBytes)];
  if (!replacement)
    return setError(QString("%1: cannot allocate %2-byte volume buffer")
                    .arg(operation).arg(volumeBytes));
  memset(replacement, 0, static_cast<size_t>(volumeBytes));

  QProgressDialog progress(QString("Loading %1").\
			   arg(m_baseFilename),
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1 + (m_depth-1)/slabSize;
  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);

      m_qfile.setFileName(m_filename);
      if (!m_qfile.open(QFile::ReadOnly))
        {
          setError(QString("%1: cannot open '%2': %3")
                   .arg(operation).arg(m_filename).arg(m_qfile.errorString()));
          break;
        }

      const qint64 firstSlice = static_cast<qint64>(ns)*slabSize;
      const int nslices = qMin(slabSize,
                               m_depth-static_cast<int>(firstSlice));
      qint64 slabBytes = 0;
      qint64 destinationOffset = 0;
      qint64 expectedSize = 0;
      bool ok = checkedMultiply(static_cast<qint64>(nslices),
                                bps, slabBytes) &&
                checkedMultiply(firstSlice, bps, destinationOffset) &&
                checkedAdd(m_header, slabBytes, expectedSize);
      if (!ok)
        setError(QString("%1: byte count overflows").arg(operation));
      if (ok && m_qfile.size() != expectedSize)
        ok = setError(QString("%1: '%2' has %3 bytes, expected %4")
                      .arg(operation).arg(m_filename)
                      .arg(m_qfile.size()).arg(expectedSize));
      if (ok)
        ok = seekFile(m_qfile, m_header, operation) &&
             readExact(m_qfile, replacement+destinationOffset,
                       slabBytes, operation);
      
      progress.setLabelText(QString("%1 : %2 %3")
                            .arg(m_filename).arg(firstSlice)
                            .arg(firstSlice+nslices-1));

      m_qfile.close();
      if (!ok)
        break;

      progress.setValue((int)(100.0*(firstSlice+nslices)/m_depth));
      if (qApp)
        qApp->processEvents();
    }

  if (!m_lastError.isEmpty())
    {
      if (m_qfile.isOpen())
        m_qfile.close();
      delete [] replacement;
      return false;
    }

  delete [] m_volData;
  m_volData = replacement;
  m_volDataCapacity = static_cast<size_t>(volumeBytes);
  progress.setValue(100);

  m_memChanged = false;
  return true;
}

bool
VolumeFileManager::createMemFile()
{
  const QString operation = "allocate memory volume";
  qint64 volumeBytes = 0;
  if (!validateGeometry(operation) ||
      !volumeByteCount(volumeBytes, operation))
    return false;
  if (static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: volume is too large for this process")
                    .arg(operation));

  uchar *replacement =
    new (std::nothrow) uchar[static_cast<size_t>(volumeBytes)];
  if (!replacement)
    return setError(QString("%1: cannot allocate %2-byte volume buffer")
                    .arg(operation).arg(volumeBytes));
  memset(replacement, 0, static_cast<size_t>(volumeBytes));

  delete [] m_volData;
  m_volData = replacement;
  m_volDataCapacity = static_cast<size_t>(volumeBytes);
  return true;
}

uchar*
VolumeFileManager::getSliceMem(int d)
{
  if (!m_memmapped)
    return getSlice(d);

  const QString operation = "read memory depth slice";
  clearError();
  if (!validateGeometry(operation) || d < 0 || d >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: depth index %2 is outside [0, %3)")
                 .arg(operation).arg(d).arg(m_depth));
      return 0;
    }

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 sourceOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(d), bps, sourceOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity) ||
      !ensureSliceCapacity(bps, operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return 0;
    }

  memset(m_slice, 0, static_cast<size_t>(bps));
  memcpy(m_slice, m_volData+sourceOffset, static_cast<size_t>(bps));

  return m_slice;
}
bool
VolumeFileManager::setSliceMem(int d, uchar *tmp)
{
  if (!m_memmapped)
    return setSlice(d, tmp);

  const QString operation = "write memory depth slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || d < 0 || d >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: depth index %2 is outside [0, %3)")
                 .arg(operation).arg(d).arg(m_depth));
      return false;
    }

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 destinationOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(d), bps, destinationOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return false;
    }

  // Commit the in-memory copy only after the matching disk write succeeds.
  if (!setSlice(d, tmp))
    return false;
  memcpy(m_volData+destinationOffset, tmp, static_cast<size_t>(bps));
  return true;
}

uchar*
VolumeFileManager::getWidthSliceMem(int w)
{
  if (!m_memmapped)
    return getWidthSlice(w);

  const QString operation = "read memory width slice";
  clearError();
  if (!validateGeometry(operation) || w < 0 || w >= m_width)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: width index %2 is outside [0, %3)")
                 .arg(operation).arg(w).arg(m_width));
      return 0;
    }

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 rowBytes = 0;
  qint64 planeBytes = 0;
  qint64 widthOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(m_height),
                       m_bytesPerVoxel, rowBytes) ||
      !checkedMultiply(static_cast<qint64>(m_depth),
                       rowBytes, planeBytes) ||
      !checkedMultiply(static_cast<qint64>(w),
                       rowBytes, widthOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity) ||
      !ensureSliceCapacity(planeBytes, operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return 0;
    }

  memset(m_slice, 0, static_cast<size_t>(planeBytes));
  for(int d=0; d<m_depth; ++d)
    {
      qint64 volumeOffset = 0;
      qint64 outputOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(d), bps, volumeOffset) ||
          !checkedAdd(volumeOffset, widthOffset, volumeOffset) ||
          !checkedMultiply(static_cast<qint64>(d),
                           rowBytes, outputOffset))
        {
          memset(m_slice, 0, static_cast<size_t>(planeBytes));
          setError(QString("%1: byte offset overflows").arg(operation));
          return 0;
        }
      memcpy(m_slice+outputOffset, m_volData+volumeOffset,
             static_cast<size_t>(rowBytes));
    }

  return m_slice;
}
bool
VolumeFileManager::setWidthSliceMem(int w, uchar *tmp)
{
  if (!m_memmapped)
    return setWidthSlice(w, tmp);

  const QString operation = "write memory width slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || w < 0 || w >= m_width)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: width index %2 is outside [0, %3)")
                 .arg(operation).arg(w).arg(m_width));
      return false;
    }

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 rowBytes = 0;
  qint64 widthOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(m_height),
                       m_bytesPerVoxel, rowBytes) ||
      !checkedMultiply(static_cast<qint64>(w),
                       rowBytes, widthOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return false;
    }

  for(int d=0; d<m_depth; ++d)
    {
      qint64 volumeOffset = 0;
      qint64 inputOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(d), bps, volumeOffset) ||
          !checkedAdd(volumeOffset, widthOffset, volumeOffset) ||
          !checkedMultiply(static_cast<qint64>(d),
                           rowBytes, inputOffset))
        return setError(QString("%1: byte offset overflows").arg(operation));
      memcpy(m_volData+volumeOffset, tmp+inputOffset,
             static_cast<size_t>(rowBytes));
    }

  m_memChanged = true;
  return true;
}

uchar*
VolumeFileManager::getHeightSliceMem(int h)
{
  if (!m_memmapped)
    return getHeightSlice(h);

  const QString operation = "read memory height slice";
  clearError();
  if (!validateGeometry(operation) || h < 0 || h >= m_height)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: height index %2 is outside [0, %3)")
                 .arg(operation).arg(h).arg(m_height));
      return 0;
    }

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 planeVoxels = 0;
  qint64 planeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(m_depth),
                       static_cast<qint64>(m_width), planeVoxels) ||
      !checkedMultiply(planeVoxels, m_bytesPerVoxel, planeBytes) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity) ||
      !ensureSliceCapacity(planeBytes, operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return 0;
    }

  memset(m_slice, 0, static_cast<size_t>(planeBytes));
  for(int d=0; d<m_depth; ++d)
    {
      for(int w=0; w<m_width; ++w)
        {
          qint64 volumeSliceOffset = 0;
          qint64 rowVoxel = 0;
          qint64 voxelOffset = 0;
          qint64 dataOffset = 0;
          qint64 outputVoxel = 0;
          qint64 outputOffset = 0;
          if (!checkedMultiply(static_cast<qint64>(d),
                               bps, volumeSliceOffset) ||
              !checkedMultiply(static_cast<qint64>(w),
                               static_cast<qint64>(m_height), rowVoxel) ||
              !checkedAdd(rowVoxel, static_cast<qint64>(h), voxelOffset) ||
              !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
              !checkedAdd(volumeSliceOffset, dataOffset, dataOffset) ||
              !checkedMultiply(static_cast<qint64>(d),
                               static_cast<qint64>(m_width), outputVoxel) ||
              !checkedAdd(outputVoxel, static_cast<qint64>(w), outputVoxel) ||
              !checkedMultiply(outputVoxel, m_bytesPerVoxel, outputOffset))
            {
              memset(m_slice, 0, static_cast<size_t>(planeBytes));
              setError(QString("%1: byte offset overflows").arg(operation));
              return 0;
            }
          memcpy(m_slice+outputOffset, m_volData+dataOffset,
                 static_cast<size_t>(m_bytesPerVoxel));
        }
    }
  
  return m_slice;
}
bool
VolumeFileManager::setHeightSliceMem(int h, uchar *tmp)
{
  if (!m_memmapped)
    return setHeightSlice(h, tmp);

  const QString operation = "write memory height slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || h < 0 || h >= m_height)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: height index %2 is outside [0, %3)")
                 .arg(operation).arg(h).arg(m_height));
      return false;
    }

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return false;
    }

  for(int d=0; d<m_depth; ++d)
    {
      for(int w=0; w<m_width; ++w)
        {
          qint64 volumeSliceOffset = 0;
          qint64 rowVoxel = 0;
          qint64 voxelOffset = 0;
          qint64 dataOffset = 0;
          qint64 inputVoxel = 0;
          qint64 inputOffset = 0;
          if (!checkedMultiply(static_cast<qint64>(d),
                               bps, volumeSliceOffset) ||
              !checkedMultiply(static_cast<qint64>(w),
                               static_cast<qint64>(m_height), rowVoxel) ||
              !checkedAdd(rowVoxel, static_cast<qint64>(h), voxelOffset) ||
              !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
              !checkedAdd(volumeSliceOffset, dataOffset, dataOffset) ||
              !checkedMultiply(static_cast<qint64>(d),
                               static_cast<qint64>(m_width), inputVoxel) ||
              !checkedAdd(inputVoxel, static_cast<qint64>(w), inputVoxel) ||
              !checkedMultiply(inputVoxel, m_bytesPerVoxel, inputOffset))
            return setError(QString("%1: byte offset overflows").arg(operation));
          memcpy(m_volData+dataOffset, tmp+inputOffset,
                 static_cast<size_t>(m_bytesPerVoxel));
        }
    }

  m_memChanged = true;
  return true;
}

uchar*
VolumeFileManager::rawValueMem(int d, int w, int h)
{
  if (!m_memmapped)
    return rawValue(d,w,h);

  const QString operation = "read memory voxel";
  clearError();
  if (!validateGeometry(operation) ||
      !ensureSliceCapacity(8, operation))
    return 0;

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return m_slice;

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 sliceOffset = 0;
  qint64 rowVoxel = 0;
  qint64 voxelOffset = 0;
  qint64 dataOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(d), bps, sliceOffset) ||
      !checkedMultiply(static_cast<qint64>(w),
                       static_cast<qint64>(m_height), rowVoxel) ||
      !checkedAdd(rowVoxel, static_cast<qint64>(h), voxelOffset) ||
      !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
      !checkedAdd(sliceOffset, dataOffset, dataOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return 0;
    }

  memcpy(m_slice, m_volData+dataOffset,
	 static_cast<size_t>(m_bytesPerVoxel));

  return m_slice;
}

bool
VolumeFileManager::setValueMem(int d, int w, int h, int val)
{
  const QString operation = "write memory voxel";
  clearError();
  if (!m_memmapped)
    return setError(QString("%1: memory mapping is disabled").arg(operation));

  if (!validateGeometry(operation))
    return false;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return setError(QString("%1: voxel index is outside the volume")
                    .arg(operation));

  qint64 volumeBytes = 0;
  qint64 sliceVoxels = 0;
  qint64 sliceOffset = 0;
  qint64 rowOffset = 0;
  qint64 voxelIndex = 0;
  if (!volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(m_width),
                       static_cast<qint64>(m_height), sliceVoxels) ||
      !checkedMultiply(static_cast<qint64>(d),
                       sliceVoxels, sliceOffset) ||
      !checkedMultiply(static_cast<qint64>(w),
                       static_cast<qint64>(m_height), rowOffset) ||
      !checkedAdd(sliceOffset, rowOffset, voxelIndex) ||
      !checkedAdd(voxelIndex, static_cast<qint64>(h), voxelIndex) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return false;
    }

  if (m_bytesPerVoxel == 1)
    m_volData[voxelIndex] = val;
  else if (m_bytesPerVoxel == 2)
    reinterpret_cast<ushort*>(m_volData)[voxelIndex] = val;
  else
    return setError(QString("%1: only 8-bit and 16-bit voxels are supported")
                    .arg(operation));
  
//  QMessageBox::information(0, "", QString("%1 %2 %3 : %4").\
//			   arg(d).arg(w).arg(h).arg(m_volData[d*m_width*m_height + w*m_height + h]));

  m_memChanged = true;

  return true;
}

void
VolumeFileManager::saveBlock(int dmin, int dmax,
			     int wmin, int wmax,
			     int hmin, int hmax)
{
  const QString operation = "save memory block";
  clearError();
  if (!m_memmapped)
    return;

  if (dmin == -1 || wmin == -1 || hmin == -1 ||
      dmax == -1 || wmax == -1 || hmax == -1)
    {
      if (m_qfile.isOpen()) m_qfile.close();
      return;
    }

  if (!validateGeometry(operation))
    return;

  dmin = qMax(0, dmin);
  wmin = qMax(0, wmin);
  hmin = qMax(0, hmin);

  dmax = qMin(m_depth-1, dmax);
  wmax = qMin(m_width-1, wmax);
  hmax = qMin(m_height-1, hmax);

  if (dmin > dmax || wmin > wmax || hmin > hmax)
    {
      setError(QString("%1: block bounds are empty").arg(operation));
      return;
    }

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 writeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(hmax-hmin+1),
                       m_bytesPerVoxel, writeBytes) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return;
    }

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  qint64 expectedSize = 0;
  qint64 maximumSize = 0;

  for(int d=dmin; d<=dmax; d++)
    {
      m_slabno = d/slabSize;
      if (m_slabno != previousSlab)
        {
          if (m_qfile.isOpen())
            {
              if (!flushAndCheckSize(m_qfile, expectedSize, operation))
                {
                  m_qfile.close();
                  return;
                }
              m_qfile.close();
            }
          if (!openSlab(m_slabno, QFile::ReadWrite, operation))
            return;
          expectedSize = m_qfile.size();
          if (!slabFileSize(m_slabno, bps, maximumSize, operation) ||
              expectedSize < 0 || expectedSize > maximumSize)
            {
              m_qfile.close();
              if (m_lastError.isEmpty())
                setError(QString("%1: '%2' has invalid size %3 (maximum %4)")
                         .arg(operation).arg(m_filename)
                         .arg(expectedSize).arg(maximumSize));
              return;
            }
          previousSlab = m_slabno;
        }

      for(int w=wmin; w<=wmax; w++)
	{
          qint64 slabSliceOffset = 0;
          qint64 volumeSliceOffset = 0;
          qint64 rowVoxel = 0;
          qint64 voxelOffset = 0;
          qint64 rowOffset = 0;
          qint64 fileOffset = 0;
          qint64 endOffset = 0;
          qint64 sourceOffset = 0;
          if (!checkedMultiply(static_cast<qint64>(d-m_slabno*slabSize),
                               bps, slabSliceOffset) ||
              !checkedMultiply(static_cast<qint64>(d),
                               bps, volumeSliceOffset) ||
              !checkedMultiply(static_cast<qint64>(w),
                               static_cast<qint64>(m_height), rowVoxel) ||
              !checkedAdd(rowVoxel, static_cast<qint64>(hmin), voxelOffset) ||
              !checkedMultiply(voxelOffset, m_bytesPerVoxel, rowOffset) ||
              !checkedAdd(slabSliceOffset, rowOffset, fileOffset) ||
              !checkedAdd(m_header, fileOffset, fileOffset) ||
              !checkedAdd(fileOffset, writeBytes, endOffset) ||
              !checkedAdd(volumeSliceOffset, rowOffset, sourceOffset))
            {
              m_qfile.close();
              setError(QString("%1: byte offset overflows").arg(operation));
              return;
            }

          if (endOffset > maximumSize)
            {
              m_qfile.close();
              setError(QString("%1: write exceeds the configured slab size")
                       .arg(operation));
              return;
            }

          expectedSize = qMax(expectedSize, endOffset);
          if (!seekFile(m_qfile, fileOffset, operation) ||
              !writeExact(m_qfile, m_volData+sourceOffset,
                          writeBytes, operation))
            {
              m_qfile.close();
              return;
            }
	}      
    }

  if (m_qfile.isOpen())
    {
      flushAndCheckSize(m_qfile, expectedSize, operation);
      m_qfile.close();
    }
}

bool
VolumeFileManager::changeSliceOrdering()
{
  const QString operation = "change slice ordering";
  clearError();
  if (!validateGeometry(operation))
    return false;

  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1 + (m_depth-1)/slabSize;
  if (nslabs > 1)
    {
      QMessageBox::information(0, "", "Cannot change ordering : slices spread across multiple files.");
      return setError(QString("%1: slices span multiple files").arg(operation));
    }

  qint64 bps = 0;
  qint64 dataBytes = 0;
  qint64 expectedSize = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_depth), bps, dataBytes) ||
      !checkedAdd(m_header, dataBytes, expectedSize) ||
      !ensureSliceCapacity(qMax(m_header, bps), operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: byte count overflows").arg(operation));
      return false;
    }

  QStringList items;
  items << "Yes" << "No";
  bool ok;
  QString item = QInputDialog::getItem(0,
				       "Change Slice Ordering",
				       "Save in another file ?",
				       items,
				       0,
				       false,
				       &ok);
  if (!ok)
    return false;

  if (item == "Yes")
    {
      QString dirname = QFileInfo(m_baseFilename).absolutePath();
	
      QString newflnm = QFileDialog::getSaveFileName(0,
						     "Save File",
						     dirname,
						     "pvl.nc Files (*.pvl.nc)");
      if (newflnm.isEmpty())
	  return false;

      if (!QFile::copy(m_baseFilename, newflnm))
        return setError(QString("%1: cannot copy '%2' to '%3'")
                        .arg(operation).arg(m_baseFilename).arg(newflnm));

      m_qfile.setFileName(slabFilename(0));
      if (!m_qfile.open(QFile::ReadOnly))
        {
          QFile::remove(newflnm);
          return setError(QString("%1: cannot open '%2': %3")
                          .arg(operation).arg(m_qfile.fileName())
                          .arg(m_qfile.errorString()));
        }
      QFile newfile;
      newfile.setFileName(newflnm+".001");
      if (!newfile.open(QFile::WriteOnly | QFile::Truncate))
        {
          m_qfile.close();
          QFile::remove(newflnm);
          return setError(QString("%1: cannot open '%2': %3")
                          .arg(operation).arg(newfile.fileName())
                          .arg(newfile.errorString()));
        }

      bool ioOk = m_qfile.size() == expectedSize;
      if (!ioOk)
        setError(QString("%1: '%2' has %3 bytes, expected %4")
                 .arg(operation).arg(m_qfile.fileName())
                 .arg(m_qfile.size()).arg(expectedSize));
      if (ioOk && m_header > 0)
        ioOk = seekFile(m_qfile, 0, operation) &&
               readExact(m_qfile, m_slice, m_header, operation) &&
               writeExact(newfile, m_slice, m_header, operation);

      for(int d=0; d<m_depth && ioOk; d++)
	{
	  qint64 sourceOffset = 0;
	  qint64 destinationOffset = 0;
	  ioOk = checkedMultiply(static_cast<qint64>(d),
                                 bps, sourceOffset) &&
	         checkedAdd(m_header, sourceOffset, sourceOffset) &&
	         checkedMultiply(static_cast<qint64>(m_depth-1-d),
                                 bps, destinationOffset) &&
	         checkedAdd(m_header, destinationOffset, destinationOffset);
	  if (!ioOk)
            setError(QString("%1: byte offset overflows").arg(operation));
	  if (ioOk)
            ioOk = seekFile(m_qfile, sourceOffset, operation) &&
                   readExact(m_qfile, m_slice, bps, operation) &&
                   seekFile(newfile, destinationOffset, operation) &&
                   writeExact(newfile, m_slice, bps, operation);
	}

      if (ioOk)
        ioOk = flushAndCheckSize(newfile, expectedSize, operation);

      newfile.close();
      m_qfile.close();

      if (!ioOk)
        {
          QFile::remove(newflnm+".001");
          QFile::remove(newflnm);
          return false;
        }

      QMessageBox::information(0, "Change Slice Ordering",
			       QString("Volume saved to "+newflnm));

      return false; // just so that we don't reload the volume
    }
  
  
  const QString flnm = slabFilename(0);
  if (m_qfile.isOpen())
    m_qfile.close();
  m_qfile.setFileName(flnm);
  if (!m_qfile.open(QFile::ReadWrite))
    {
      QMessageBox::information(0, "", "Cannot change ordering : cannot open file for writing.");
      return setError(QString("%1: cannot open '%2': %3")
                      .arg(operation).arg(flnm).arg(m_qfile.errorString()));
    }

  if (m_qfile.size() != expectedSize)
    {
      const qint64 actualSize = m_qfile.size();
      m_qfile.close();
      return setError(QString("%1: '%2' has %3 bytes, expected %4")
                      .arg(operation).arg(flnm)
                      .arg(actualSize).arg(expectedSize));
    }

  if (static_cast<quint64>(bps) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    {
      m_qfile.close();
      return setError(QString("%1: temporary slice is too large")
                      .arg(operation));
    }

  uchar* tslice = new (std::nothrow) uchar[static_cast<size_t>(bps)];
  if (!tslice)
    {
      m_qfile.close();
      return setError(QString("%1: cannot allocate temporary slice")
                      .arg(operation));
    }

  bool ioOk = true;
  for(int d=0; d<m_depth/2 && ioOk; d++)
    {
      qint64 firstOffset = 0;
      qint64 secondOffset = 0;
      ioOk = checkedMultiply(static_cast<qint64>(d), bps, firstOffset) &&
             checkedAdd(m_header, firstOffset, firstOffset) &&
             checkedMultiply(static_cast<qint64>(m_depth-1-d),
                             bps, secondOffset) &&
             checkedAdd(m_header, secondOffset, secondOffset);
      if (!ioOk)
        setError(QString("%1: byte offset overflows").arg(operation));
      if (ioOk)
        ioOk = seekFile(m_qfile, firstOffset, operation) &&
               readExact(m_qfile, m_slice, bps, operation) &&
               seekFile(m_qfile, secondOffset, operation) &&
               readExact(m_qfile, tslice, bps, operation) &&
               seekFile(m_qfile, firstOffset, operation) &&
               writeExact(m_qfile, tslice, bps, operation) &&
               seekFile(m_qfile, secondOffset, operation) &&
               writeExact(m_qfile, m_slice, bps, operation);
    }

  if (ioOk)
    ioOk = flushAndCheckSize(m_qfile, expectedSize, operation);

  delete [] tslice;

  m_qfile.close();

  return ioOk;
}
