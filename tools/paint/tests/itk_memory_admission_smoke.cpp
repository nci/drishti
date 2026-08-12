#include "itkmemoryadmission.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
const std::uint64_t kMiB = 1024ULL*1024ULL;
const std::uint64_t kGiB = 1024ULL*kMiB;

struct FakeMemoryProvider
{
  bool succeeds;
  SystemMemoryStatus status;

  FakeMemoryProvider() : succeeds(true) {}
};

bool provideMemoryStatus(SystemMemoryStatus& status, void *context)
{
  FakeMemoryProvider *provider =
    static_cast<FakeMemoryProvider *>(context);
  if (!provider->succeeds)
    return false;
  status = provider->status;
  return true;
}

SystemMemoryStatus knownMemory(std::uint64_t totalPhysicalBytes,
                               std::uint64_t availablePhysicalBytes,
                               std::uint64_t commitLimitBytes,
                               std::uint64_t availableCommitBytes)
{
  SystemMemoryStatus status;
  status.totalPhysicalBytes = totalPhysicalBytes;
  status.availablePhysicalBytes = availablePhysicalBytes;
  status.availablePhysicalKnown = true;
  status.commitLimitBytes = commitLimitBytes;
  status.availableCommitBytes = availableCommitBytes;
  status.committedBytes = commitLimitBytes-availableCommitBytes;
  status.availableCommitKnown = true;
  return status;
}

bool hasBudgetFields(const QString& message)
{
  return message.contains(QStringLiteral("Required:")) &&
         message.contains(QStringLiteral("physical budget:")) &&
         message.contains(QStringLiteral("Commit budget:"));
}

int fail(const std::string& message)
{
  std::cerr << "FAILED: " << message << std::endl;
  return 1;
}
}

int main()
{
  struct ExpectedProfile
  {
    ItkMemoryWorkload workload;
    std::uint64_t bytesPerVoxel;
    std::uint64_t overheadMiB;
  };

  const ExpectedProfile expected[] = {
    { ItkMemoryWorkload::BinaryThinning, 32ULL, 64ULL },
    { ItkMemoryWorkload::ConnectedComponents, 48ULL, 64ULL },
    { ItkMemoryWorkload::DistanceMap, 48ULL, 64ULL },
    { ItkMemoryWorkload::EdgePreserving, 128ULL, 128ULL },
    { ItkMemoryWorkload::Smoothing, 96ULL, 128ULL },
    { ItkMemoryWorkload::VesselEnhancingDiffusion, 256ULL, 256ULL }
  };
  for (const ExpectedProfile& item : expected)
    {
      const ItkMemoryProfile profile = itkMemoryProfile(item.workload);
      if (profile.bytesPerVoxel != item.bytesPerVoxel ||
          profile.fixedOverheadBytes != item.overheadMiB*kMiB)
        return fail("an ITK workload memory profile changed unexpectedly");
    }

  FakeMemoryProvider roomyProvider;
  roomyProvider.status = knownMemory(32*kGiB, 24*kGiB,
                                      64*kGiB, 48*kGiB);
  const PaintAlgorithmMemoryAdmission approved =
    evaluateItkMemoryAdmission(
      ItkMemoryWorkload::VesselEnhancingDiffusion,
      16, 16, 16, provideMemoryStatus, &roomyProvider);
  if (!approved.approved || !approved.physicalMemoryChecked ||
      !approved.commitMemoryChecked ||
      approved.requiredBytes != 257*kMiB)
    return fail("a small VED workload was not admitted with its full estimate");

  FakeMemoryProvider physicalProvider;
  physicalProvider.status = knownMemory(16*kGiB, 5*kGiB,
                                         64*kGiB, 48*kGiB);
  const PaintAlgorithmMemoryAdmission physicalRejected =
    evaluateItkMemoryAdmission(
      ItkMemoryWorkload::VesselEnhancingDiffusion,
      160, 160, 160, provideMemoryStatus, &physicalProvider);
  if (physicalRejected.approved ||
      physicalRejected.reason !=
        PaintAlgorithmMemoryAdmissionReason::InsufficientPhysicalMemory ||
      !hasBudgetFields(itkMemoryAdmissionMessage(
        ItkMemoryWorkload::VesselEnhancingDiffusion, physicalRejected)))
    return fail("physical-memory pressure did not reject VED with budgets");

  FakeMemoryProvider commitProvider;
  commitProvider.status = knownMemory(16*kGiB, 16*kGiB,
                                       32*kGiB, 1*kGiB);
  const PaintAlgorithmMemoryAdmission commitRejected =
    evaluateItkMemoryAdmission(
      ItkMemoryWorkload::VesselEnhancingDiffusion,
      160, 160, 160, provideMemoryStatus, &commitProvider);
  if (commitRejected.approved ||
      commitRejected.reason !=
        PaintAlgorithmMemoryAdmissionReason::InsufficientCommit ||
      !hasBudgetFields(itkMemoryAdmissionMessage(
        ItkMemoryWorkload::VesselEnhancingDiffusion, commitRejected)))
    return fail("Commit pressure did not reject VED with budgets");

  FakeMemoryProvider unavailableProvider;
  unavailableProvider.succeeds = false;
  const PaintAlgorithmMemoryAdmission unavailable =
    evaluateItkMemoryAdmission(
      ItkMemoryWorkload::BinaryThinning,
      1, 1, 1, provideMemoryStatus, &unavailableProvider);
  if (unavailable.approved ||
      unavailable.reason !=
        PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable ||
      !hasBudgetFields(itkMemoryAdmissionMessage(
        ItkMemoryWorkload::BinaryThinning, unavailable)))
    return fail("unknown memory status was not rejected with budget fields");

  const PaintAlgorithmMemoryAdmission overflow =
    evaluateItkMemoryAdmission(
      ItkMemoryWorkload::Smoothing,
      std::numeric_limits<std::uint64_t>::max(), 2, 1,
      provideMemoryStatus, &roomyProvider);
  if (overflow.approved ||
      overflow.reason !=
        PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow)
    return fail("overflowing ITK dimensions were admitted");

  try
    {
      requireItkMemoryAdmission(
        ItkMemoryWorkload::VesselEnhancingDiffusion,
        160, 160, 160, provideMemoryStatus, &physicalProvider);
      return fail("the throwing admission wrapper accepted a rejected workload");
    }
  catch (const std::runtime_error& error)
    {
      if (!hasBudgetFields(QString::fromLocal8Bit(error.what())))
        return fail("the throwing admission wrapper omitted memory budgets");
    }

  std::cout << "ITK memory admission smoke passed" << std::endl;
  return 0;
}
