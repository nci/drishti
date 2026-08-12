#include "../volumepluginvalidation.h"

#include <QCoreApplication>

#include <cmath>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>

namespace
{
class FakePlugin : public QObject, public VolInterface
{
  Q_OBJECT
  Q_INTERFACES(VolInterface)

public:
  enum Mode
  {
    Valid,
    ThrowInit,
    ThrowSetFile,
    ThrowMetadata,
    ThrowBadAllocation,
    RejectInput,
    EmptyHistogram,
    OversizedHistogram,
    ZeroHistogram,
    InvalidRange,
    InvalidSpacing
  };

  enum ReadOperation
  {
    DepthRead,
    WidthRead,
    HeightRead,
    RawValueRead
  };

  enum ReadFailure
  {
    ReadSucceeds,
    ReadBadAllocation,
    ReadStandardException,
    ReadUnknownException,
    ReadStatusError,
    ReadCanceled
  };

  enum RangeFailure
  {
    RangeSucceeds,
    RangeBadAllocation,
    RangeStandardException,
    RangeUnknownException,
    RangeStatusError,
    RangeCanceled,
    RangeInvalidHistogram
  };

  explicit FakePlugin(Mode mode,
                      ReadOperation readOperation = DepthRead,
                      ReadFailure readFailure = ReadSucceeds,
                      RangeFailure rangeFailure = RangeSucceeds)
    : m_mode(mode),
      m_readOperation(readOperation),
      m_readFailure(readFailure),
      m_rangeFailure(rangeFailure),
      m_rawMinimum(0.0f),
      m_rawMaximum(11.0f),
      m_canceled(false)
  {}

  QStringList registerPlugin() override { return QStringList(); }
  void init() override
  {
    if (m_mode == ThrowInit)
      throw std::runtime_error("init failure");
  }
  void set4DVolume(bool) override {}
  void clear() override {}
  bool setFile(QStringList) override
  {
    if (m_mode == ThrowSetFile)
      throw std::runtime_error("setFile failure");
    if (m_mode == ThrowBadAllocation)
      throw std::bad_alloc();
    return m_mode != RejectInput;
  }
  void replaceFile(QString) override {}
  void gridSize(int& depth, int& width, int& height) override
  {
    if (m_mode == ThrowMetadata)
      throw std::runtime_error("metadata failure");
    depth = 2;
    width = 2;
    height = 3;
  }
  void voxelSize(float& x, float& y, float& z) override
  {
    x = m_mode == InvalidSpacing ?
      std::numeric_limits<float>::quiet_NaN() : 1.0f;
    y = 1.0f;
    z = 1.0f;
  }
  QString description() override { return QStringLiteral("fake"); }
  int voxelUnit() override { return _Micron; }
  int voxelType() override { return _UShort; }
  int headerBytes() override { return 0; }
  QList<uint> histogram() override
  {
    if (m_mode == EmptyHistogram ||
        m_rangeFailure == RangeInvalidHistogram)
      return QList<uint>();
    QList<uint> values;
    const int size = m_mode == OversizedHistogram ? 65537 : 65536;
    values.reserve(size);
    for (int index=0; index<size; ++index)
      values.append(0);
    if (m_mode != ZeroHistogram)
      values[0] = 12;
    return values;
  }
  void setMinMax(float rawMinimum, float rawMaximum) override
  {
    m_lastError.clear();
    m_canceled = false;
    switch (m_rangeFailure)
      {
      case RangeSucceeds:
      case RangeInvalidHistogram:
        m_rawMinimum = rawMinimum;
        m_rawMaximum = rawMaximum;
        return;
      case RangeBadAllocation:
        throw std::bad_alloc();
      case RangeStandardException:
        throw std::runtime_error("fixture range failure");
      case RangeUnknownException:
        throw 9;
      case RangeStatusError:
        m_lastError = "fixture range status error";
        return;
      case RangeCanceled:
        m_canceled = true;
        return;
      }
  }
  float rawMin() override
  {
    return m_mode == InvalidRange ?
      std::numeric_limits<float>::quiet_NaN() : m_rawMinimum;
  }
  float rawMax() override { return m_rawMaximum; }
  void getDepthSlice(int, uchar *destination) override
  {
    applyReadFailure(DepthRead);
    destination[0] = 11;
  }
  void getWidthSlice(int, uchar *destination) override
  {
    applyReadFailure(WidthRead);
    destination[0] = 12;
  }
  void getHeightSlice(int, uchar *destination) override
  {
    applyReadFailure(HeightRead);
    destination[0] = 13;
  }
  QVariant rawValue(int, int, int) override
  {
    applyReadFailure(RawValueRead);
    return QVariant(17);
  }

  Q_INVOKABLE QString lastError() const { return m_lastError; }
  Q_INVOKABLE bool wasCanceled() const { return m_canceled; }

private:
  void applyReadFailure(ReadOperation operation)
  {
    m_lastError.clear();
    m_canceled = false;
    if (operation != m_readOperation)
      return;

    switch (m_readFailure)
      {
      case ReadSucceeds:
        return;
      case ReadBadAllocation:
        throw std::bad_alloc();
      case ReadStandardException:
        throw std::runtime_error("fixture read failure");
      case ReadUnknownException:
        throw 7;
      case ReadStatusError:
        m_lastError = "fixture status error";
        return;
      case ReadCanceled:
        m_canceled = true;
        return;
      }
  }

  Mode m_mode;
  ReadOperation m_readOperation;
  ReadFailure m_readFailure;
  RangeFailure m_rangeFailure;
  float m_rawMinimum;
  float m_rawMaximum;
  QString m_lastError;
  bool m_canceled;
};

int fail(const QString& message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

bool rejected(FakePlugin::Mode mode, const QString& expectedText)
{
  FakePlugin plugin(mode);
  VolumePluginMetadata metadata;
  const bool loaded = loadNativeVolumePlugin(
    &plugin, QStringList() << QStringLiteral("fixture"),
    false, false, &metadata);
  return !loaded && !metadata.error.isEmpty() &&
         (expectedText.isEmpty() ||
         metadata.error.contains(expectedText, Qt::CaseInsensitive));
}

bool readRejected(FakePlugin::ReadOperation operation,
                  FakePlugin::ReadFailure failure,
                  const QString& expectedText,
                  bool expectedCanceled)
{
  FakePlugin plugin(FakePlugin::Valid, operation, failure);
  VolumePluginOperationStatus status;
  uchar destination[4] = {0, 0, 0, 0};
  QVariant value(99);
  bool read = false;
  switch (operation)
    {
    case FakePlugin::DepthRead:
      read = readNativeVolumePluginSlice(
        &plugin, VolumePluginSliceAxis::Depth, 0, destination, &status);
      break;
    case FakePlugin::WidthRead:
      read = readNativeVolumePluginSlice(
        &plugin, VolumePluginSliceAxis::Width, 0, destination, &status);
      break;
    case FakePlugin::HeightRead:
      read = readNativeVolumePluginSlice(
        &plugin, VolumePluginSliceAxis::Height, 0, destination, &status);
      break;
    case FakePlugin::RawValueRead:
      read = readNativeVolumePluginRawValue(
        &plugin, 0, 0, 0, &value, &status);
      break;
    }
  return !read && !status.error.isEmpty() &&
         status.error.contains(expectedText, Qt::CaseInsensitive) &&
         status.canceled == expectedCanceled &&
         (operation != FakePlugin::RawValueRead || !value.isValid());
}

bool rangeRejected(FakePlugin::RangeFailure failure,
                   const QString& expectedText,
                   bool expectedCanceled)
{
  FakePlugin plugin(FakePlugin::Valid, FakePlugin::DepthRead,
                    FakePlugin::ReadSucceeds, failure);
  QList<uint> histogram;
  VolumePluginOperationStatus status;
  const bool updated = updateNativeVolumePluginRange(
    &plugin, 2.0f, 8.0f, &histogram, &status);
  return !updated && histogram.isEmpty() && !status.error.isEmpty() &&
         status.error.contains(expectedText, Qt::CaseInsensitive) &&
         status.canceled == expectedCanceled;
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);

  FakePlugin valid(FakePlugin::Valid);
  VolumePluginMetadata metadata;
  if (!loadNativeVolumePlugin(&valid, QStringList() << "fixture",
                              false, true, &metadata) ||
      metadata.depth != 2 || metadata.width != 2 || metadata.height != 3 ||
      metadata.histogram.size() != 65536)
    return fail(QString("Valid fake plugin was rejected: %1")
                  .arg(metadata.error));

  const struct TestCase
  {
    FakePlugin::Mode mode;
    const char *expectedText;
  } tests[] = {
    { FakePlugin::ThrowInit, "exception" },
    { FakePlugin::ThrowSetFile, "exception" },
    { FakePlugin::ThrowMetadata, "exception" },
    { FakePlugin::ThrowBadAllocation, "out of memory" },
    { FakePlugin::RejectInput, "rejected" },
    { FakePlugin::EmptyHistogram, "histogram" },
    { FakePlugin::OversizedHistogram, "histogram" },
    { FakePlugin::ZeroHistogram, "no samples" },
    { FakePlugin::InvalidRange, "range" },
    { FakePlugin::InvalidSpacing, "spacing" }
  };
  for (const TestCase& test : tests)
    if (!rejected(test.mode, QString::fromLatin1(test.expectedText)))
      return fail(QString("Invalid fake plugin mode %1 was not safely rejected")
                    .arg(static_cast<int>(test.mode)));

  uchar slice[4] = {0, 0, 0, 0};
  QVariant rawValue;
  VolumePluginOperationStatus operationStatus;
  if (!readNativeVolumePluginSlice(
        &valid, VolumePluginSliceAxis::Depth, 0, slice, &operationStatus) ||
      slice[0] != 11 ||
      !readNativeVolumePluginSlice(
        &valid, VolumePluginSliceAxis::Width, 0, slice, &operationStatus) ||
      slice[0] != 12 ||
      !readNativeVolumePluginSlice(
        &valid, VolumePluginSliceAxis::Height, 0, slice, &operationStatus) ||
      slice[0] != 13 ||
      !readNativeVolumePluginRawValue(
        &valid, 0, 0, 0, &rawValue, &operationStatus) ||
      rawValue.toInt() != 17)
    return fail("Valid native read operation was rejected");

  const struct ReadFailureCase
  {
    FakePlugin::ReadFailure failure;
    const char *expectedText;
    bool canceled;
  } readFailures[] = {
    { FakePlugin::ReadBadAllocation, "out of memory", false },
    { FakePlugin::ReadStandardException, "exception", false },
    { FakePlugin::ReadUnknownException, "unknown exception", false },
    { FakePlugin::ReadStatusError, "fixture status error", false },
    { FakePlugin::ReadCanceled, "canceled", true }
  };
  const FakePlugin::ReadOperation readOperations[] = {
    FakePlugin::DepthRead,
    FakePlugin::WidthRead,
    FakePlugin::HeightRead,
    FakePlugin::RawValueRead
  };
  for (FakePlugin::ReadOperation operation : readOperations)
    for (const ReadFailureCase& failure : readFailures)
      if (!readRejected(operation, failure.failure,
                        QString::fromLatin1(failure.expectedText),
                        failure.canceled))
        return fail(QString("Native read operation %1 did not safely reject "
                            "failure mode %2")
                      .arg(static_cast<int>(operation))
                      .arg(static_cast<int>(failure.failure)));

  FakePlugin validRange(FakePlugin::Valid);
  QList<uint> rangeHistogram;
  if (!updateNativeVolumePluginRange(
        &validRange, 2.0f, 8.0f, &rangeHistogram, &operationStatus) ||
      rangeHistogram.size() != 65536 || validRange.rawMin() != 2.0f ||
      validRange.rawMax() != 8.0f)
    return fail(QString("Valid native range update was rejected: %1")
                  .arg(operationStatus.error));

  const struct RangeFailureCase
  {
    FakePlugin::RangeFailure failure;
    const char *expectedText;
    bool canceled;
  } rangeFailures[] = {
    { FakePlugin::RangeBadAllocation, "out of memory", false },
    { FakePlugin::RangeStandardException, "exception", false },
    { FakePlugin::RangeUnknownException, "unknown exception", false },
    { FakePlugin::RangeStatusError, "fixture range status error", false },
    { FakePlugin::RangeCanceled, "canceled", true },
    { FakePlugin::RangeInvalidHistogram, "histogram", false }
  };
  for (const RangeFailureCase& failure : rangeFailures)
    if (!rangeRejected(failure.failure,
                       QString::fromLatin1(failure.expectedText),
                       failure.canceled))
      return fail(QString("Native range update did not safely reject failure "
                          "mode %1")
                    .arg(static_cast<int>(failure.failure)));

  if (readNativeVolumePluginSlice(
        nullptr, VolumePluginSliceAxis::Depth, 0, slice, &operationStatus) ||
      operationStatus.error.isEmpty() ||
      readNativeVolumePluginSlice(
        &valid, VolumePluginSliceAxis::Depth, 0, nullptr, &operationStatus) ||
      operationStatus.error.isEmpty() ||
      readNativeVolumePluginRawValue(
        &valid, 0, 0, 0, nullptr, &operationStatus) ||
      operationStatus.error.isEmpty())
    return fail("Invalid native read arguments were not safely rejected");

  if (updateNativeVolumePluginRange(
        nullptr, 0.0f, 1.0f, &rangeHistogram, &operationStatus) ||
      operationStatus.error.isEmpty() ||
      updateNativeVolumePluginRange(
        &validRange, 0.0f, 1.0f, nullptr, &operationStatus) ||
      operationStatus.error.isEmpty() ||
      updateNativeVolumePluginRange(
        &validRange, 2.0f, 1.0f, &rangeHistogram, &operationStatus) ||
      operationStatus.error.isEmpty())
    return fail("Invalid native range-update arguments were not rejected");

  std::cout << "Volume plugin validation smoke passed" << std::endl;
  return 0;
}

#include "volume_plugin_validation_smoke.moc"
