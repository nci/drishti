#include "importmemoryadmission.h"

#include <algorithm>
#include <limits>
#include <new>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) || \
      (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

namespace
{
const std::uint64_t kMiB = 1024ULL*1024ULL;
const std::uint64_t kGiB = 1024ULL*kMiB;
const std::uint64_t kMinimumSystemReserveBytes = 2ULL*kGiB;
const std::uint64_t kMinimumCommitReserveBytes = 2ULL*kGiB;
const std::uint64_t kMinimumIntegratedGpuReserveBytes = 512ULL*kMiB;
const std::uint64_t kMaximumIntegratedGpuReserveBytes = 2ULL*kGiB;

std::uint64_t subtractReserve(std::uint64_t available,
                              std::uint64_t reserve)
{
  return available > reserve ? available-reserve : 0;
}

void calculateReserves(std::uint64_t totalPhysicalBytes,
                       std::uint64_t& systemReserveBytes,
                       std::uint64_t& integratedGpuReserveBytes)
{
  // Integer ratios avoid floating-point rounding near UINT64_MAX.
  const std::uint64_t proportionalSystemReserve = totalPhysicalBytes/5;
  const std::uint64_t proportionalGpuReserve = totalPhysicalBytes/16;
  systemReserveBytes = std::max(kMinimumSystemReserveBytes,
                                proportionalSystemReserve);
  integratedGpuReserveBytes = std::min(
    kMaximumIntegratedGpuReserveBytes,
    std::max(kMinimumIntegratedGpuReserveBytes, proportionalGpuReserve));
}

#if defined(__unix__) || defined(__unix) || defined(unix) || \
    (defined(__APPLE__) && defined(__MACH__))
bool sysconfBytes(int pageCountName, std::uint64_t& bytes)
{
  const long pages = sysconf(pageCountName);
#if defined(_SC_PAGESIZE)
  const long pageSize = sysconf(_SC_PAGESIZE);
#elif defined(_SC_PAGE_SIZE)
  const long pageSize = sysconf(_SC_PAGE_SIZE);
#else
  const long pageSize = -1;
#endif
  if (pages <= 0 || pageSize <= 0)
    return false;
  return checkedImportMultiply(static_cast<std::uint64_t>(pages),
                               static_cast<std::uint64_t>(pageSize), bytes);
}
#endif
}

ImportMemoryStatus::ImportMemoryStatus()
  : totalPhysicalBytes(0),
    availablePhysicalBytes(0),
    commitLimitBytes(0),
    availableCommitBytes(0),
    availablePhysicalKnown(false),
    availableCommitKnown(false),
#if defined(_WIN32)
    commitRequired(true)
#else
    commitRequired(false)
#endif
{
}

ImportMemoryAdmission::ImportMemoryAdmission()
  : approved(false),
    physicalMemoryChecked(false),
    commitMemoryChecked(false),
    requiredBytes(0),
    systemReserveBytes(0),
    integratedGpuReserveBytes(0),
    availablePhysicalBudgetBytes(0),
    availableCommitBudgetBytes(0),
    reason(ImportMemoryAdmissionReason::InvalidRequest),
    reservation()
{
}

bool
checkedImportMultiply(std::uint64_t first,
                      std::uint64_t second,
                      std::uint64_t& result)
{
  if (first == 0 || second == 0)
    {
      result = 0;
      return true;
    }
  if (first > std::numeric_limits<std::uint64_t>::max()/second)
    return false;
  result = first*second;
  return true;
}

bool
checkedImportAdd(std::uint64_t first,
                 std::uint64_t second,
                 std::uint64_t& result)
{
  if (first > std::numeric_limits<std::uint64_t>::max()-second)
    return false;
  result = first+second;
  return true;
}

bool
checkedImportImageDecodeWorkingSet(std::uint64_t pixelCount,
                                   std::uint64_t codecSafetyBytes,
                                   std::uint64_t& requiredBytes)
{
  requiredBytes = 0;
  if (pixelCount == 0)
    return false;

  // Qt 5 can retain an 8-byte RGBA64 source while creating a 4-byte ARGB32
  // conversion. Codec-private scratch is covered separately by the caller.
  std::uint64_t imageBytes = 0;
  if (!checkedImportMultiply(pixelCount, 12, imageBytes) ||
      !checkedImportAdd(imageBytes, codecSafetyBytes, requiredBytes))
    return false;
  return true;
}

bool
queryImportMemoryStatus(ImportMemoryStatus& status, void *context)
{
  (void)context;
  status = ImportMemoryStatus();

#if defined(_WIN32)
  MEMORYSTATUSEX nativeStatus = {};
  nativeStatus.dwLength = sizeof(nativeStatus);
  if (!GlobalMemoryStatusEx(&nativeStatus))
    return false;

  status.totalPhysicalBytes =
    static_cast<std::uint64_t>(nativeStatus.ullTotalPhys);
  status.availablePhysicalBytes = std::min(
    status.totalPhysicalBytes,
    static_cast<std::uint64_t>(nativeStatus.ullAvailPhys));
  status.availablePhysicalKnown = status.totalPhysicalBytes > 0;

  status.commitLimitBytes =
    static_cast<std::uint64_t>(nativeStatus.ullTotalPageFile);
  status.availableCommitBytes =
    static_cast<std::uint64_t>(nativeStatus.ullAvailPageFile);
  status.availableCommitKnown = status.commitLimitBytes > 0 &&
    status.availableCommitBytes <= status.commitLimitBytes;
  return status.availablePhysicalKnown && status.availableCommitKnown;

#elif defined(__unix__) || defined(__unix) || defined(unix) || \
      (defined(__APPLE__) && defined(__MACH__))
#if defined(_SC_PHYS_PAGES)
  (void)sysconfBytes(_SC_PHYS_PAGES, status.totalPhysicalBytes);
#endif
#if defined(_SC_AVPHYS_PAGES)
  status.availablePhysicalKnown =
    sysconfBytes(_SC_AVPHYS_PAGES, status.availablePhysicalBytes);
#endif
  if (status.availablePhysicalKnown && status.totalPhysicalBytes > 0)
    status.availablePhysicalBytes = std::min(
      status.availablePhysicalBytes, status.totalPhysicalBytes);
  return status.totalPhysicalBytes > 0 && status.availablePhysicalKnown;
#else
  return false;
#endif
}

ImportMemoryAdmission
evaluateImportMemoryAdmission(std::uint64_t requiredBytes,
                              ImportMemoryStatusProvider provider,
                              void *providerContext)
{
  ImportMemoryAdmission admission;
  admission.requiredBytes = requiredBytes;
  if (requiredBytes == 0)
    return admission;

  if (requiredBytes >
      static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
      admission.reason = ImportMemoryAdmissionReason::AddressSpaceLimit;
      return admission;
    }

  if (!provider)
    provider = queryImportMemoryStatus;

  ImportMemoryStatus status;
  if (!provider(status, providerContext) ||
      !status.availablePhysicalKnown || status.totalPhysicalBytes == 0 ||
      status.availablePhysicalBytes > status.totalPhysicalBytes ||
      (status.commitRequired &&
       (!status.availableCommitKnown || status.commitLimitBytes == 0 ||
        status.availableCommitBytes > status.commitLimitBytes)))
    {
      admission.reason = ImportMemoryAdmissionReason::MemoryStatusUnavailable;
      return admission;
    }

  admission.physicalMemoryChecked = true;
  admission.commitMemoryChecked = status.commitRequired;
  calculateReserves(status.totalPhysicalBytes,
                    admission.systemReserveBytes,
                    admission.integratedGpuReserveBytes);

  std::uint64_t combinedReserveBytes = 0;
  if (!checkedImportAdd(admission.systemReserveBytes,
                        admission.integratedGpuReserveBytes,
                        combinedReserveBytes))
    {
      admission.reason = ImportMemoryAdmissionReason::ArithmeticOverflow;
      return admission;
    }

  admission.availablePhysicalBudgetBytes = subtractReserve(
    status.availablePhysicalBytes, combinedReserveBytes);
  if (status.availableCommitKnown)
    admission.availableCommitBudgetBytes = subtractReserve(
      status.availableCommitBytes, kMinimumCommitReserveBytes);

  if (requiredBytes > admission.availablePhysicalBudgetBytes)
    {
      admission.reason =
        ImportMemoryAdmissionReason::InsufficientPhysicalMemory;
      return admission;
    }
  if (status.commitRequired &&
      requiredBytes > admission.availableCommitBudgetBytes)
    {
      admission.reason = ImportMemoryAdmissionReason::InsufficientCommit;
      return admission;
    }

  admission.approved = true;
  admission.reason = ImportMemoryAdmissionReason::Approved;
  return admission;
}

bool
reserveImportMemory(const ImportMemoryAdmission& admission,
                    std::shared_ptr<ProcessMemoryReservation>& reservation)
{
  reservation.reset();
  if (!admission.approved || admission.requiredBytes == 0)
    return false;

  std::shared_ptr<ProcessMemoryReservation> candidate(
    new (std::nothrow) ProcessMemoryReservation());
  if (!candidate || !candidate->acquire(
        admission.requiredBytes,
        admission.availablePhysicalBudgetBytes,
        admission.commitMemoryChecked,
        admission.availableCommitBudgetBytes))
    return false;

  reservation = candidate;
  return true;
}
