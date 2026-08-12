#include "../volumevaluemapping.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
bool expectMapped(double value,
                  const std::vector<float>& rawMap,
                  const std::vector<int>& pvlMap,
                  int expected)
{
  int actual = -1;
  return mapImportValueToPvl(value, rawMap, pvlMap, actual) &&
    actual == expected;
}
}

int main()
{
  const std::vector<float> normalRaw = {0.0f, 10.0f};
  const std::vector<int> normalPvl = {0, 255};
  if (!expectMapped(5.0, normalRaw, normalPvl, 127) ||
      !expectMapped(-std::numeric_limits<double>::infinity(),
                    normalRaw, normalPvl, 0) ||
      !expectMapped(std::numeric_limits<double>::infinity(),
                    normalRaw, normalPvl, 255) ||
      !expectMapped(std::numeric_limits<double>::quiet_NaN(),
                    normalRaw, normalPvl, 0))
    return 1;

  const std::vector<float> constantRaw = {7.0f, 7.0f};
  if (!expectMapped(7.0, constantRaw, normalPvl, 0) ||
      !expectMapped(8.0, constantRaw, normalPvl, 255))
    return 2;

  const std::vector<float> repeatedRaw = {0.0f, 5.0f, 5.0f, 10.0f};
  const std::vector<int> repeatedPvl = {0, 100, 150, 255};
  if (!expectMapped(5.0, repeatedRaw, repeatedPvl, 100) ||
      !expectMapped(7.5, repeatedRaw, repeatedPvl, 202))
    return 3;

  const std::vector<float> descendingRaw = {10.0f, 0.0f};
  int mapped = 0;
  if (mapImportValueToPvl(5.0, descendingRaw, normalPvl, mapped) ||
      mapImportValueToPvl(5.0, normalRaw, std::vector<int>{0}, mapped))
    return 4;

  std::cout << "Volume value mapping smoke passed\n";
  return 0;
}
