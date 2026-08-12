#ifndef VOLUMEVALUE_MAPPING_H
#define VOLUMEVALUE_MAPPING_H

#include <cmath>
#include <limits>

template <typename RawMap, typename PvlMap>
bool mapImportValueToPvl(double value,
                         const RawMap& rawMap,
                         const PvlMap& pvlMap,
                         int& mappedValue)
{
  const auto mapSize = rawMap.size();
  if (mapSize < 2 || pvlMap.size() != mapSize ||
      static_cast<unsigned long long>(mapSize) >
        static_cast<unsigned long long>(std::numeric_limits<int>::max()))
    return false;
  const int count = static_cast<int>(mapSize);

  for (int index=0; index<count; ++index)
    {
      const double point = static_cast<double>(rawMap[index]);
      if (!std::isfinite(point) ||
          (index > 0 && point < static_cast<double>(rawMap[index-1])))
        return false;
    }

  if (std::isnan(value) || value <= static_cast<double>(rawMap[0]))
    {
      mappedValue = static_cast<int>(pvlMap[0]);
      return true;
    }

  if (value >= static_cast<double>(rawMap[count-1]))
    {
      mappedValue = static_cast<int>(pvlMap[count-1]);
      return true;
    }

  for (int index=0; index<count-1; ++index)
    {
      const double lower = static_cast<double>(rawMap[index]);
      const double upper = static_cast<double>(rawMap[index+1]);
      if (value > upper)
        continue;

      if (upper <= lower)
        {
          mappedValue = static_cast<int>(pvlMap[index+1]);
          return true;
        }

      const double fraction = (value-lower)/(upper-lower);
      const double lowerPvl = static_cast<double>(pvlMap[index]);
      const double upperPvl = static_cast<double>(pvlMap[index+1]);
      const double mapped = lowerPvl + fraction*(upperPvl-lowerPvl);
      if (!std::isfinite(mapped))
        return false;

      if (mapped <= static_cast<double>(std::numeric_limits<int>::min()))
        mappedValue = std::numeric_limits<int>::min();
      else if (mapped >= static_cast<double>(std::numeric_limits<int>::max()))
        mappedValue = std::numeric_limits<int>::max();
      else
        mappedValue = static_cast<int>(mapped);
      return true;
    }

  mappedValue = static_cast<int>(pvlMap[count-1]);
  return true;
}

#endif
