#include "../samplingcontract.h"

#include <cassert>

int main()
{
  SamplingContract::Axis depth(5, 2);
  SamplingContract::Axis width(5, 2);
  SamplingContract::Axis height(3, 2);
  assert(SamplingContract::valid(depth));
  assert(depth.output == 3 && width.output == 3 && height.output == 2);
  int begin = 0, end = 0;
  SamplingContract::block(depth, 2, begin, end);
  assert(begin == 4 && end == 4);
  assert(SamplingContract::sampleCount(depth, width, height, 2, 2, 1) == 2);
  assert(SamplingContract::isNearest(1, 1));
  assert(!SamplingContract::isNearest(2, 1));
  return 0;
}
