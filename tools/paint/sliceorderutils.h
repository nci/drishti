#ifndef SLICEORDERUTILS_H
#define SLICEORDERUTILS_H

#include <QString>
#include <QtGlobal>

bool reversePaintSliceOrder(uchar *volume,
                            int volumeBytesPerVoxel,
                            uchar *mask,
                            int depth,
                            int width,
                            int height,
                            QString &error);

#endif
