#include "graphcut/graphcut.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
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
  QString errorMessage;
  quint64 graphBytes = 0;
  if (!MaxFlowMinCut::estimateMemoryBytes(
        32, 24, graphBytes, errorMessage) || graphBytes == 0)
    return fail("cannot estimate Graph Cut graph memory");

  quint64 invocationBytes = 0;
  if (!MaxFlowMinCut::estimateInvocationMemoryBytes(
        32, 24, 100, 80, invocationBytes, errorMessage))
    return fail("cannot estimate Graph Cut invocation memory");

  const quint64 roiStagingBytes = 32ULL*24ULL*
    (sizeof(uchar) + 2*sizeof(ushort));
  const quint64 finalTagBytes = 100ULL*80ULL*sizeof(ushort);
  const quint64 expectedPeak = std::max(
    graphBytes+roiStagingBytes, roiStagingBytes+finalTagBytes);
  if (invocationBytes != expectedPeak)
    return fail("invocation estimate includes memory outside the new buffers");

  if (MaxFlowMinCut::estimateInvocationMemoryBytes(
        101, 24, 100, 80, invocationBytes, errorMessage) ||
      errorMessage.isEmpty())
    return fail("invalid ROI dimensions were accepted");

  FakeMemoryProvider ampleProvider;
  ampleProvider.status = knownMemory(64*kGiB, 64*kGiB,
                                     128*kGiB, 120*kGiB);
  PaintAlgorithmMemoryAdmission admission;
  if (!MaxFlowMinCut::admitMemoryBytes(
        MaxFlowMinCut::memoryLimitBytes(), errorMessage, &admission,
        provideMemoryStatus, &ampleProvider) ||
      !admission.approved || !errorMessage.isEmpty())
    return fail("the fixed 512 MiB boundary was not admitted");

  if (MaxFlowMinCut::admitMemoryBytes(
        MaxFlowMinCut::memoryLimitBytes()+1, errorMessage, &admission,
        provideMemoryStatus, &ampleProvider) ||
      !admission.approved || !hasBudgetFields(errorMessage) ||
      !errorMessage.contains(QStringLiteral("fixed 512 MiB")))
    return fail("the fixed 512 MiB upper boundary was not enforced");

  const quint64 requiredBytes = 16*kMiB;
  if (!MaxFlowMinCut::admitMemoryBytes(
        1, errorMessage, &admission,
        provideMemoryStatus, &ampleProvider))
    return fail("cannot obtain reserve values from ample-memory probe");
  const quint64 combinedReserve = admission.systemReserveBytes+
                                  admission.integratedGpuReserveBytes;

  FakeMemoryProvider physicalProvider;
  physicalProvider.status = knownMemory(
    64*kGiB, combinedReserve+requiredBytes-1,
    128*kGiB, 120*kGiB);
  if (MaxFlowMinCut::admitMemoryBytes(
        requiredBytes, errorMessage, &admission,
        provideMemoryStatus, &physicalProvider) ||
      admission.reason !=
        PaintAlgorithmMemoryAdmissionReason::InsufficientPhysicalMemory ||
      admission.availablePhysicalBudgetBytes != requiredBytes-1 ||
      !hasBudgetFields(errorMessage))
    return fail("live physical-memory rejection is incorrect");

  FakeMemoryProvider commitProvider;
  commitProvider.status = knownMemory(
    64*kGiB, 64*kGiB,
    128*kGiB, admission.commitReserveBytes+requiredBytes-1);
  if (MaxFlowMinCut::admitMemoryBytes(
        requiredBytes, errorMessage, &admission,
        provideMemoryStatus, &commitProvider) ||
      admission.reason != PaintAlgorithmMemoryAdmissionReason::InsufficientCommit ||
      admission.availableCommitBudgetBytes != requiredBytes-1 ||
      !hasBudgetFields(errorMessage))
    return fail("live Commit rejection is incorrect");

  FakeMemoryProvider unavailableProvider;
  unavailableProvider.succeeds = false;
  if (MaxFlowMinCut::admitMemoryBytes(
        requiredBytes, errorMessage, &admission,
        provideMemoryStatus, &unavailableProvider) ||
      admission.reason !=
        PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable ||
      unavailableProvider.calls != 1 ||
      !hasBudgetFields(errorMessage) ||
      !errorMessage.contains(QStringLiteral("unavailable")))
    return fail("unknown memory status was not reported with budgets");

  std::cout << "Graph Cut memory admission smoke passed" << std::endl;
  return 0;
}
