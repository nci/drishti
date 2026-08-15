#ifndef SAMPLINGCONTRACT_H
#define SAMPLINGCONTRACT_H

#include <cstdint>

namespace SamplingContract
{
struct Axis
{
  int source;
  int factor;
  int output;

  Axis() : source(0), factor(1), output(0) {}
  Axis(int sourceSize, int blockSize)
    : source(sourceSize), factor(blockSize > 0 ? blockSize : 1),
      output(sourceSize > 0 ? (sourceSize + factor - 1)/factor : 0) {}
};

inline bool valid(const Axis& axis)
{
  return axis.source > 0 && axis.factor > 0 && axis.output > 0;
}

inline void block(const Axis& axis, int index, int& begin, int& end)
{
  begin = index * axis.factor;
  end = begin + axis.factor - 1;
  if (begin < 0) begin = 0;
  if (end >= axis.source) end = axis.source - 1;
}

inline std::uint64_t sampleCount(const Axis& depth,
                                 const Axis& width,
                                 const Axis& height,
                                 int depthIndex,
                                 int widthIndex,
                                 int heightIndex)
{
  int d0, d1, w0, w1, h0, h1;
  block(depth, depthIndex, d0, d1);
  block(width, widthIndex, w0, w1);
  block(height, heightIndex, h0, h1);
  if (d1 < d0 || w1 < w0 || h1 < h0) return 0;
  return static_cast<std::uint64_t>(d1-d0+1) *
         static_cast<std::uint64_t>(w1-w0+1) *
         static_cast<std::uint64_t>(h1-h0+1);
}

// A factor of one is nearest/identity.  A factor above one is a box average
// over the actual edge block; partial blocks are never padded or discarded.
inline bool isNearest(int depthFactor, int xyFactor)
{
  return depthFactor <= 1 && xyFactor <= 1;
}
}

#endif
