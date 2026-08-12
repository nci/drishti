#ifndef DICOMHISTOGRAMUTILS_H
#define DICOMHISTOGRAMUTILS_H

#include <limits>

namespace DicomHistogramUtils
{
inline int signedShortIndex(short value)
{
  return static_cast<int>(value)-
         static_cast<int>(std::numeric_limits<short>::min());
}
}

#endif
