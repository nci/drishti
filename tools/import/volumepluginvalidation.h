#ifndef VOLUMEPLUGINVALIDATION_H
#define VOLUMEPLUGINVALIDATION_H

#include "common.h"
#include "volinterface.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

struct VolumePluginMetadata
{
  QString description;
  int depth = 0;
  int width = 0;
  int height = 0;
  int voxelType = _UChar;
  int voxelUnit = _Micron;
  int headerBytes = 0;
  float voxelSizeX = 1.0f;
  float voxelSizeY = 1.0f;
  float voxelSizeZ = 1.0f;
  float rawMinimum = 0.0f;
  float rawMaximum = 0.0f;
  QList<uint> histogram;
  QString error;
  bool canceled = false;
};

enum class VolumePluginSliceAxis
{
  Depth,
  Width,
  Height
};

struct VolumePluginOperationStatus
{
  QString error;
  bool canceled = false;
};

bool validateVolumePluginMetadata(const VolumePluginMetadata& metadata,
                                  QString *error);

bool loadNativeVolumePlugin(VolInterface *plugin,
                            const QStringList& files,
                            bool volume4d,
                            bool skipRawDialog,
                            VolumePluginMetadata *metadata);

bool readNativeVolumePluginSlice(VolInterface *plugin,
                                 VolumePluginSliceAxis axis,
                                 int sliceIndex,
                                 uchar *destination,
                                 VolumePluginOperationStatus *status);

bool readNativeVolumePluginRawValue(VolInterface *plugin,
                                    int depth,
                                    int width,
                                    int height,
                                    QVariant *value,
                                    VolumePluginOperationStatus *status);

bool updateNativeVolumePluginRange(VolInterface *plugin,
                                   float rawMinimum,
                                   float rawMaximum,
                                   QList<uint> *histogram,
                                   VolumePluginOperationStatus *status);

#endif
