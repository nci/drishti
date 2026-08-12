#include "../sliceorderutils.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QVector>

#include <cstring>
#include <limits>

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}

template <typename T>
QVector<T> guarded(std::initializer_list<T> values, T guard)
{
  QVector<T> result;
  result.reserve(static_cast<int>(values.size())+2);
  result.append(guard);
  for (T value : values)
    result.append(value);
  result.append(guard);
  return result;
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  QString error;

  QVector<quint8> volume8 = guarded<quint8>(
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, 0xa5);
  QVector<quint16> mask8 = guarded<quint16>(
    {101, 102, 103, 104, 105, 106,
     107, 108, 109, 110, 111, 112}, 0xa55a);
  if (!reversePaintSliceOrder(volume8.data()+1, 1,
                              reinterpret_cast<uchar*>(mask8.data()+1),
                              3, 2, 2, error))
    return fail("8-bit volume reversal failed: "+error);
  if (volume8 != guarded<quint8>(
        {9, 10, 11, 12, 5, 6, 7, 8, 1, 2, 3, 4}, 0xa5) ||
      mask8 != guarded<quint16>(
        {109, 110, 111, 112, 105, 106,
         107, 108, 101, 102, 103, 104}, 0xa55a))
    return fail("8-bit volume or 16-bit mask reversal changed the wrong bytes");

  QVector<quint16> volume16 = guarded<quint16>(
    {1, 2, 3, 4, 5, 6, 7, 8}, 0xface);
  QVector<quint16> mask16 = guarded<quint16>(
    {11, 12, 13, 14, 15, 16, 17, 18}, 0xcafe);
  if (!reversePaintSliceOrder(
        reinterpret_cast<uchar*>(volume16.data()+1), 2,
        reinterpret_cast<uchar*>(mask16.data()+1), 2, 2, 2, error))
    return fail("16-bit volume reversal failed: "+error);
  if (volume16 != guarded<quint16>(
        {5, 6, 7, 8, 1, 2, 3, 4}, 0xface) ||
      mask16 != guarded<quint16>(
        {15, 16, 17, 18, 11, 12, 13, 14}, 0xcafe))
    return fail("16-bit pointer arithmetic crossed a slice boundary");

  const QVector<quint16> unchangedVolume = volume16;
  const QVector<quint16> unchangedMask = mask16;
  if (reversePaintSliceOrder(
        reinterpret_cast<uchar*>(volume16.data()+1), 2,
        reinterpret_cast<uchar*>(mask16.data()+1),
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max(), error) ||
      volume16 != unchangedVolume || mask16 != unchangedMask || error.isEmpty())
    return fail("Overflowing dimensions were not rejected transactionally");

  QTextStream(stdout)
    << "Paint slice ordering smoke passed: 8/16-bit reversal, guards and overflow"
    << Qt::endl;
  return 0;
}
