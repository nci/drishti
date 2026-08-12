#include "volumepluginvalidation.h"

#include "pluginoperationstatus.h"

#include <QObject>

#include <cmath>
#include <exception>
#include <limits>
#include <new>

namespace
{
QString exceptionMessage(const QString& decoder, const std::exception& error)
{
  return QString("%1 raised an exception: %2")
    .arg(decoder, QString::fromLocal8Bit(error.what()));
}

template <typename Operation>
bool runNativeVolumePluginOperation(
  VolInterface *plugin,
  const QString& operationDescription,
  const QString& cancellationMessage,
  VolumePluginOperationStatus *status,
  Operation operation)
{
  if (!status)
    return false;
  *status = VolumePluginOperationStatus();
  if (!plugin)
    {
      status->error = "The volume decoder interface is unavailable.";
      return false;
    }

  try
    {
      operation();
      QObject *pluginObject = dynamic_cast<QObject*>(plugin);
      status->error = importPluginLastError(pluginObject);
      status->canceled = importPluginWasCanceled(pluginObject);
      if (status->canceled && status->error.isEmpty())
        status->error = cancellationMessage;
      return status->error.isEmpty() && !status->canceled;
    }
  catch (const std::bad_alloc&)
    {
      status->error = QStringLiteral(
        "%1 ran out of memory; the operation was stopped.")
        .arg(operationDescription);
    }
  catch (const std::exception& error)
    {
      status->error = exceptionMessage(operationDescription, error);
    }
  catch (...)
    {
      status->error = QStringLiteral("%1 raised an unknown exception.")
        .arg(operationDescription);
    }
  return false;
}

QString sliceAxisName(VolumePluginSliceAxis axis)
{
  switch (axis)
    {
    case VolumePluginSliceAxis::Depth:
      return QStringLiteral("depth");
    case VolumePluginSliceAxis::Width:
      return QStringLiteral("width");
    case VolumePluginSliceAxis::Height:
      return QStringLiteral("height");
    }
  return QString();
}
}

bool validateVolumePluginMetadata(const VolumePluginMetadata& metadata,
                                  QString *error)
{
  if (!error)
    return false;
  if (metadata.depth <= 0 || metadata.width <= 0 || metadata.height <= 0 ||
      metadata.voxelType < _UChar || metadata.voxelType > _Rgba)
    {
      *error = "The volume decoder returned invalid dimensions or voxel type.";
      return false;
    }
  if (metadata.voxelUnit < _Nounit || metadata.voxelUnit > _Kiloparsec ||
      metadata.headerBytes < 0)
    {
      *error = "The volume decoder returned an invalid unit or header size.";
      return false;
    }
  if (!std::isfinite(metadata.voxelSizeX) || metadata.voxelSizeX <= 0.0f ||
      !std::isfinite(metadata.voxelSizeY) || metadata.voxelSizeY <= 0.0f ||
      !std::isfinite(metadata.voxelSizeZ) || metadata.voxelSizeZ <= 0.0f)
    {
      *error = "The volume decoder returned invalid voxel spacing.";
      return false;
    }
  if (!std::isfinite(metadata.rawMinimum) ||
      !std::isfinite(metadata.rawMaximum) ||
      metadata.rawMinimum > metadata.rawMaximum)
    {
      *error = "The volume decoder returned an invalid finite value range.";
      return false;
    }
  if (metadata.histogram.isEmpty() || metadata.histogram.size() > 65536)
    {
      *error = "The volume decoder returned an empty or oversized histogram.";
      return false;
    }

  quint64 histogramTotal = 0;
  for (uint count : metadata.histogram)
    {
      if (histogramTotal > std::numeric_limits<quint64>::max()-count)
        {
          *error = "The volume decoder histogram count overflowed.";
          return false;
        }
      histogramTotal += count;
    }
  if (histogramTotal == 0)
    {
      *error = "The volume decoder histogram contains no samples.";
      return false;
    }
  return true;
}

bool loadNativeVolumePlugin(VolInterface *plugin,
                            const QStringList& files,
                            bool volume4d,
                            bool skipRawDialog,
                            VolumePluginMetadata *metadata)
{
  if (!metadata)
    return false;
  *metadata = VolumePluginMetadata();
  if (!plugin)
    {
      metadata->error = "The volume decoder interface is unavailable.";
      return false;
    }

  try
    {
      plugin->init();
      plugin->set4DVolume(volume4d);
      if (skipRawDialog)
        plugin->setValue("skiprawdialog", 1.0f);
      if (!plugin->setFile(files))
        {
          QObject *pluginObject = dynamic_cast<QObject*>(plugin);
          metadata->error = importPluginLastError(pluginObject);
          metadata->canceled = importPluginWasCanceled(pluginObject);
          if (metadata->error.isEmpty())
            metadata->error = "The volume decoder rejected the selected input.";
          return false;
        }

      metadata->description = plugin->description();
      plugin->gridSize(metadata->depth, metadata->width, metadata->height);
      metadata->voxelType = plugin->voxelType();
      metadata->voxelUnit = plugin->voxelUnit();
      metadata->headerBytes = plugin->headerBytes();
      plugin->voxelSize(metadata->voxelSizeX,
                        metadata->voxelSizeY,
                        metadata->voxelSizeZ);
      metadata->rawMinimum = plugin->rawMin();
      metadata->rawMaximum = plugin->rawMax();
      metadata->histogram = plugin->histogram();
      return validateVolumePluginMetadata(*metadata, &metadata->error);
    }
  catch (const std::bad_alloc&)
    {
      metadata->error =
        "The volume decoder ran out of memory; the input was rejected.";
    }
  catch (const std::exception& error)
    {
      metadata->error = exceptionMessage("The volume decoder", error);
    }
  catch (...)
    {
      metadata->error = "The volume decoder raised an unknown exception.";
    }
  return false;
}

bool readNativeVolumePluginSlice(VolInterface *plugin,
                                 VolumePluginSliceAxis axis,
                                 int sliceIndex,
                                 uchar *destination,
                                 VolumePluginOperationStatus *status)
{
  if (!status)
    return false;
  *status = VolumePluginOperationStatus();
  if (!destination)
    {
      status->error = "The volume decoder slice destination is null.";
      return false;
    }

  const QString axisName = sliceAxisName(axis);
  if (axisName.isEmpty())
    {
      status->error = "The requested volume decoder slice axis is invalid.";
      return false;
    }

  const QString operationDescription = QStringLiteral(
    "The volume decoder %1-slice read").arg(axisName);
  const QString cancellationMessage = QStringLiteral(
    "The volume decoder canceled %1-slice decoding.").arg(axisName);
  return runNativeVolumePluginOperation(
    plugin, operationDescription, cancellationMessage, status,
    [plugin, axis, sliceIndex, destination]()
    {
      switch (axis)
        {
        case VolumePluginSliceAxis::Depth:
          plugin->getDepthSlice(sliceIndex, destination);
          break;
        case VolumePluginSliceAxis::Width:
          plugin->getWidthSlice(sliceIndex, destination);
          break;
        case VolumePluginSliceAxis::Height:
          plugin->getHeightSlice(sliceIndex, destination);
          break;
        }
    });
}

bool readNativeVolumePluginRawValue(VolInterface *plugin,
                                    int depth,
                                    int width,
                                    int height,
                                    QVariant *value,
                                    VolumePluginOperationStatus *status)
{
  if (!status)
    return false;
  *status = VolumePluginOperationStatus();
  if (!value)
    {
      status->error = "The volume decoder raw-value destination is null.";
      return false;
    }
  *value = QVariant();

  const bool read = runNativeVolumePluginOperation(
    plugin,
    QStringLiteral("The volume decoder raw-value read"),
    QStringLiteral("The volume decoder canceled raw-value decoding."),
    status,
    [plugin, depth, width, height, value]()
    {
      *value = plugin->rawValue(depth, width, height);
    });
  if (!read)
    *value = QVariant();
  return read;
}

bool updateNativeVolumePluginRange(VolInterface *plugin,
                                   float rawMinimum,
                                   float rawMaximum,
                                   QList<uint> *histogram,
                                   VolumePluginOperationStatus *status)
{
  if (!status)
    return false;
  *status = VolumePluginOperationStatus();
  if (!histogram)
    {
      status->error = "The volume decoder histogram destination is null.";
      return false;
    }
  histogram->clear();
  if (!std::isfinite(rawMinimum) || !std::isfinite(rawMaximum) ||
      rawMinimum > rawMaximum)
    {
      status->error = "The requested volume decoder range is invalid.";
      return false;
    }

  const bool updated = runNativeVolumePluginOperation(
    plugin,
    QStringLiteral("The volume decoder range update"),
    QStringLiteral("The volume decoder canceled histogram regeneration."),
    status,
    [plugin, rawMinimum, rawMaximum, histogram]()
    {
      plugin->setMinMax(rawMinimum, rawMaximum);
      *histogram = plugin->histogram();
    });
  if (!updated)
    {
      histogram->clear();
      return false;
    }

  VolumePluginMetadata candidate;
  candidate.depth = candidate.width = candidate.height = 1;
  candidate.rawMinimum = rawMinimum;
  candidate.rawMaximum = rawMaximum;
  candidate.histogram = *histogram;
  if (!validateVolumePluginMetadata(candidate, &status->error))
    {
      histogram->clear();
      return false;
    }
  return true;
}
