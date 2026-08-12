#include "../plugins/dicom/dicomhistogramutils.h"

#include <iostream>
#include <limits>

int main()
{
  if (DicomHistogramUtils::signedShortIndex(
        std::numeric_limits<short>::min()) != 0 ||
      DicomHistogramUtils::signedShortIndex(-1) != 32767 ||
      DicomHistogramUtils::signedShortIndex(0) != 32768 ||
      DicomHistogramUtils::signedShortIndex(
        std::numeric_limits<short>::max()) != 65535)
    return 1;

  std::cout << "DICOM signed-short histogram mapping smoke passed\n";
  return 0;
}
