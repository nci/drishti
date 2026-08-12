#include "../importmemoryadmission.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace
{
const std::uint64_t kGiB = 1024ULL*1024ULL*1024ULL;

struct FakeProvider
{
  ImportMemoryStatus status;
  bool succeeds;
};

bool provideStatus(ImportMemoryStatus& status, void *context)
{
  FakeProvider *provider = static_cast<FakeProvider*>(context);
  if (!provider || !provider->succeeds)
    return false;
  status = provider->status;
  return true;
}

FakeProvider knownMemory(std::uint64_t availablePhysical,
                         std::uint64_t availableCommit)
{
  FakeProvider provider;
  provider.succeeds = true;
  provider.status.totalPhysicalBytes = 16*kGiB;
  provider.status.availablePhysicalBytes = availablePhysical;
  provider.status.availablePhysicalKnown = true;
  provider.status.commitLimitBytes = 24*kGiB;
  provider.status.availableCommitBytes = availableCommit;
  provider.status.availableCommitKnown = true;
  provider.status.commitRequired = true;
  return provider;
}

int fail(const char *message)
{
  std::cerr << message << std::endl;
  return 1;
}
}

int main()
{
  std::uint64_t result = 0;
  if (!checkedImportMultiply(4096, 4096, result) ||
      result != 16777216ULL)
    return fail("checked multiplication rejected a valid plane");
  if (checkedImportMultiply(std::numeric_limits<std::uint64_t>::max(),
                            2, result))
    return fail("multiplication overflow was accepted");
  if (checkedImportAdd(std::numeric_limits<std::uint64_t>::max(), 1, result))
    return fail("addition overflow was accepted");

  const std::uint64_t imagePixels = 4096ULL*4096ULL;
  const std::uint64_t codecSafetyBytes = 64ULL*1024ULL*1024ULL;
  if (!checkedImportImageDecodeWorkingSet(
        imagePixels, codecSafetyBytes, result) ||
      result != 256ULL*1024ULL*1024ULL)
    return fail("image decode peak did not include 8-byte source and ARGB32 conversion");
  if (checkedImportImageDecodeWorkingSet(
        std::numeric_limits<std::uint64_t>::max()/12+1,
        codecSafetyBytes, result))
    return fail("image decode working-set overflow was accepted");
  if (checkedImportImageDecodeWorkingSet(0, codecSafetyBytes, result))
    return fail("zero-pixel image decode was accepted");

  FakeProvider roomy = knownMemory(12*kGiB, 18*kGiB);
  ImportMemoryAdmission approved = evaluateImportMemoryAdmission(
    512ULL*1024ULL*1024ULL, provideStatus, &roomy);
  if (!approved.approved || !approved.physicalMemoryChecked ||
      !approved.commitMemoryChecked)
    return fail("a bounded conversion was not admitted");

  FakeProvider physical = knownMemory(5*kGiB, 18*kGiB);
  ImportMemoryAdmission physicalRejected = evaluateImportMemoryAdmission(
    1024ULL*1024ULL*1024ULL, provideStatus, &physical);
  if (physicalRejected.approved || physicalRejected.reason !=
      ImportMemoryAdmissionReason::InsufficientPhysicalMemory)
    return fail("physical pressure did not reject the conversion");

  FakeProvider commitHeadroom = knownMemory(12*kGiB, 5*kGiB);
  ImportMemoryAdmission commitApproved = evaluateImportMemoryAdmission(
    1024ULL*1024ULL*1024ULL, provideStatus, &commitHeadroom);
  if (!commitApproved.approved ||
      commitApproved.availableCommitBudgetBytes != 3*kGiB)
    return fail("safe Commit headroom was rejected by physical-memory reserves");

  FakeProvider commit = knownMemory(12*kGiB, 2*kGiB + 512ULL*1024ULL*1024ULL);
  ImportMemoryAdmission commitRejected = evaluateImportMemoryAdmission(
    1024ULL*1024ULL*1024ULL, provideStatus, &commit);
  if (commitRejected.approved || commitRejected.reason !=
      ImportMemoryAdmissionReason::InsufficientCommit)
    return fail("Commit pressure did not reject the conversion");

  FakeProvider unknown = roomy;
  unknown.status.availableCommitKnown = false;
  ImportMemoryAdmission unavailable = evaluateImportMemoryAdmission(
    1, provideStatus, &unknown);
  if (unavailable.approved || unavailable.reason !=
      ImportMemoryAdmissionReason::MemoryStatusUnavailable)
    return fail("missing required Commit status was accepted");

  FakeProvider physicalOnly = roomy;
  physicalOnly.status.commitRequired = false;
  physicalOnly.status.availableCommitKnown = false;
  ImportMemoryAdmission nonWindows = evaluateImportMemoryAdmission(
    1, provideStatus, &physicalOnly);
  if (!nonWindows.approved || nonWindows.commitMemoryChecked)
    return fail("physical-only platform admission is incorrect");

  std::cout << "Import memory admission smoke passed" << std::endl;
  return 0;
}
