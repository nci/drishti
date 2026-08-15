#include "getmemorysize.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace
{
const std::uint64_t kMiB = 1024ULL*1024ULL;
const std::uint64_t kGiB = 1024ULL*kMiB;

struct FakeMemoryProvider
{
  bool succeeds;
  unsigned int calls;
  SystemMemoryStatus status;

  FakeMemoryProvider() : succeeds(true), calls(0) {}
};

bool provideMemoryStatus(SystemMemoryStatus& status, void *context)
{
  FakeMemoryProvider *provider =
    static_cast<FakeMemoryProvider *>(context);
  ++provider->calls;
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

PaintAlgorithmMemoryAdmission evaluateRequiredBytes(
  std::uint64_t requiredBytes,
  FakeMemoryProvider& provider)
{
  // One byte comes from the one-voxel volume; the rest is fixed overhead.
  return evaluatePaintAlgorithmMemoryAdmission(
    1, 1, 1, 1, requiredBytes-1,
    provideMemoryStatus, &provider);
}

int fail(const std::string& message)
{
  std::cerr << "FAILED: " << message << std::endl;
  return 1;
}
}

int main()
{
  std::uint64_t voxelCount = 0;
  std::uint64_t requiredBytes = 0;
  if (!calculatePaintAlgorithmRequiredBytes(
        10, 20, 30, 256, 512, voxelCount, requiredBytes) ||
      voxelCount != 6000 || requiredBytes != 1536512)
    return fail("required-byte calculation is incorrect");

  voxelCount = 99;
  requiredBytes = 99;
  if (calculatePaintAlgorithmRequiredBytes(
        std::numeric_limits<std::uint64_t>::max(), 2, 1, 1, 0,
        voxelCount, requiredBytes) ||
      voxelCount != 0 || requiredBytes != 0)
    return fail("overflowing required-byte calculation was accepted");

  if (calculatePaintAlgorithmRequiredBytes(
        1, 1, 1, std::numeric_limits<std::uint64_t>::max(), 1,
        voxelCount, requiredBytes) ||
      voxelCount != 0 || requiredBytes != 0)
    return fail("overflowing fixed-overhead addition was accepted");

  FakeMemoryProvider physicalProvider;
  physicalProvider.status = knownMemory(16*kGiB, 12*kGiB,
                                        64*kGiB, 48*kGiB);
  PaintAlgorithmMemoryAdmission probe =
    evaluateRequiredBytes(1, physicalProvider);
  if (!probe.approved || !probe.physicalMemoryChecked ||
      !probe.commitMemoryChecked ||
      probe.systemReserveBytes < 2*kGiB ||
      probe.integratedGpuReserveBytes < 512*kMiB ||
      probe.availablePhysicalBudgetBytes < 2)
    return fail("known-memory probe was not admitted with reserves");

  if (!reservePaintAlgorithmMemory(probe) ||
      !probe.reservation || !probe.reservation->active())
    return fail("an admitted Paint algorithm did not acquire its reservation");
  PaintAlgorithmMemoryAdmission competing =
    evaluateRequiredBytes(probe.availablePhysicalBudgetBytes,
                          physicalProvider);
  if (reservePaintAlgorithmMemory(competing))
    return fail("concurrent Paint algorithm reservation was incorrectly admitted");
  probe.reservation.reset();

  const std::uint64_t physicalBoundary =
    probe.availablePhysicalBudgetBytes;
  PaintAlgorithmMemoryAdmission justBelowPhysical =
    evaluateRequiredBytes(physicalBoundary-1, physicalProvider);
  if (!justBelowPhysical.approved ||
      justBelowPhysical.reason !=
        PaintAlgorithmMemoryAdmissionReason::Approved)
    return fail("request just below physical budget was rejected");

  PaintAlgorithmMemoryAdmission justAbovePhysical =
    evaluateRequiredBytes(physicalBoundary+1, physicalProvider);
  if (justAbovePhysical.approved ||
      justAbovePhysical.reason !=
        PaintAlgorithmMemoryAdmissionReason::InsufficientPhysicalMemory)
    return fail("request just above physical budget was not rejected");

  FakeMemoryProvider commitProvider;
  commitProvider.status = knownMemory(16*kGiB, 16*kGiB,
                                      32*kGiB, 8*kGiB);
  probe = evaluateRequiredBytes(1, commitProvider);
  if (!probe.approved || probe.availableCommitBudgetBytes < 2 ||
      probe.availablePhysicalBudgetBytes <= probe.availableCommitBudgetBytes)
    return fail("commit-boundary probe did not isolate the commit budget");

  const std::uint64_t commitBoundary = probe.availableCommitBudgetBytes;
  PaintAlgorithmMemoryAdmission justBelowCommit =
    evaluateRequiredBytes(commitBoundary-1, commitProvider);
  if (!justBelowCommit.approved)
    return fail("request just below commit budget was rejected");

  PaintAlgorithmMemoryAdmission justAboveCommit =
    evaluateRequiredBytes(commitBoundary+1, commitProvider);
  if (justAboveCommit.approved ||
      justAboveCommit.reason !=
        PaintAlgorithmMemoryAdmissionReason::InsufficientCommit)
    return fail("request just above commit budget was not rejected");

  FakeMemoryProvider overflowProvider;
  PaintAlgorithmMemoryAdmission overflow =
    evaluatePaintAlgorithmMemoryAdmission(
      std::numeric_limits<std::uint64_t>::max(), 2, 1, 1, 0,
      provideMemoryStatus, &overflowProvider);
  if (overflow.approved ||
      overflow.reason !=
        PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow ||
      overflowProvider.calls != 0)
    return fail("overflow did not fail before querying system memory");

  FakeMemoryProvider unavailableProvider;
  unavailableProvider.succeeds = false;
  PaintAlgorithmMemoryAdmission unavailable =
    evaluateRequiredBytes(1, unavailableProvider);
  if (unavailable.approved ||
      unavailable.reason !=
        PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable ||
      unavailableProvider.calls != 1)
    return fail("failed provider did not produce an unavailable decision");

  FakeMemoryProvider unknownPhysicalProvider;
  unknownPhysicalProvider.status = knownMemory(16*kGiB, 12*kGiB,
                                                32*kGiB, 16*kGiB);
  unknownPhysicalProvider.status.availablePhysicalKnown = false;
  PaintAlgorithmMemoryAdmission unknownPhysical =
    evaluateRequiredBytes(1, unknownPhysicalProvider);
  if (unknownPhysical.approved ||
      unknownPhysical.reason !=
        PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable)
    return fail("unknown physical status was admitted");

  FakeMemoryProvider noCommitProvider;
  noCommitProvider.status = knownMemory(16*kGiB, 12*kGiB,
                                        32*kGiB, 16*kGiB);
  noCommitProvider.status.availableCommitKnown = false;
  PaintAlgorithmMemoryAdmission noCommit =
    evaluateRequiredBytes(1, noCommitProvider);
#if defined(_WIN32)
  if (noCommit.approved ||
      noCommit.reason !=
        PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable)
    return fail("Windows admitted a request without commit status");
#else
  if (!noCommit.approved || noCommit.commitMemoryChecked ||
      !noCommit.physicalMemoryChecked)
    return fail("non-Windows physical-only fallback was not used");
#endif

  std::cout << "Algorithm memory admission smoke passed" << std::endl;
  return 0;
}
