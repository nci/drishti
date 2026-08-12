#include "getmemorysize.h"

#include <algorithm>
#include <limits>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) || \
      (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/types.h>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
#endif

namespace
{
const std::uint64_t kMiB = 1024ULL*1024ULL;
const std::uint64_t kGiB = 1024ULL*kMiB;
const std::uint64_t kOperationalReserveBytes = 512ULL*kMiB;
const std::uint64_t kMinimumSystemReserveBytes = 2ULL*kGiB;
const std::uint64_t kMinimumIntegratedGpuReserveBytes = 512ULL*kMiB;
const std::uint64_t kMaximumIntegratedGpuReserveBytes = 2ULL*kGiB;
const std::uint64_t kMinimumCommitReserveBytes = 512ULL*kMiB;
const std::uint64_t kMaximumCommitReserveBytes = 2ULL*kGiB;
const std::uint64_t kMaximumLargeVolumeThresholdBytes = 2ULL*kGiB;

bool checkedMultiply(std::uint64_t first,
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

bool checkedAdd(std::uint64_t first,
                std::uint64_t second,
                std::uint64_t& result)
{
  if (first > std::numeric_limits<std::uint64_t>::max()-second)
    return false;
  result = first+second;
  return true;
}

bool longDoubleToBytes(long double value, std::uint64_t& result)
{
  if (value < 0.0L ||
      value >= static_cast<long double>(
                std::numeric_limits<std::uint64_t>::max()))
    return false;

  result = static_cast<std::uint64_t>(value);
  if (static_cast<long double>(result) < value)
    {
      if (result == std::numeric_limits<std::uint64_t>::max())
        return false;
      ++result;
    }
  return true;
}

std::uint64_t subtractReserve(std::uint64_t available,
                              std::uint64_t reserve)
{
  return available > reserve ? available-reserve : 0;
}

bool calculatePaintMemoryReserves(std::uint64_t totalPhysicalBytes,
                                  std::uint64_t& systemReserveBytes,
                                  std::uint64_t& integratedGpuReserveBytes,
                                  std::uint64_t& largeVolumeThresholdBytes)
{
  std::uint64_t proportionalSystemReserve = 0;
  std::uint64_t proportionalGpuReserve = 0;
  std::uint64_t proportionalLargeThreshold = 0;
  if (!longDoubleToBytes(
        static_cast<long double>(totalPhysicalBytes)*0.20L,
        proportionalSystemReserve) ||
      !longDoubleToBytes(
        static_cast<long double>(totalPhysicalBytes)*0.0625L,
        proportionalGpuReserve) ||
      !longDoubleToBytes(
        static_cast<long double>(totalPhysicalBytes)*0.125L,
        proportionalLargeThreshold))
    return false;

  systemReserveBytes = std::max(
    kMinimumSystemReserveBytes, proportionalSystemReserve);
  integratedGpuReserveBytes = std::min(
    kMaximumIntegratedGpuReserveBytes,
    std::max(kMinimumIntegratedGpuReserveBytes, proportionalGpuReserve));
  largeVolumeThresholdBytes = std::min(
    kMaximumLargeVolumeThresholdBytes, proportionalLargeThreshold);
  return true;
}

bool calculatePaintCommitReserve(std::uint64_t totalPhysicalBytes,
                                 std::uint64_t& commitReserveBytes)
{
  std::uint64_t proportionalReserve = 0;
  if (!longDoubleToBytes(
        static_cast<long double>(totalPhysicalBytes)*0.03125L,
        proportionalReserve))
    return false;
  commitReserveBytes = std::min(
    kMaximumCommitReserveBytes,
    std::max(kMinimumCommitReserveBytes, proportionalReserve));
  return true;
}

#if defined(__unix__) || defined(__unix) || defined(unix) || \
    (defined(__APPLE__) && defined(__MACH__))
bool sysconfBytes(int pagesName, std::uint64_t& bytes)
{
  const long pages = sysconf(pagesName);
#if defined(_SC_PAGESIZE)
  const long pageSize = sysconf(_SC_PAGESIZE);
#elif defined(_SC_PAGE_SIZE)
  const long pageSize = sysconf(_SC_PAGE_SIZE);
#else
  const long pageSize = -1;
#endif
  if (pages <= 0 || pageSize <= 0)
    return false;
  return checkedMultiply(static_cast<std::uint64_t>(pages),
                         static_cast<std::uint64_t>(pageSize), bytes);
}
#endif
}

SystemMemoryStatus::SystemMemoryStatus()
  : totalPhysicalBytes(0),
    availablePhysicalBytes(0),
    commitLimitBytes(0),
    committedBytes(0),
    availableCommitBytes(0),
    availablePhysicalKnown(false),
    availableCommitKnown(false)
{
}

PaintMemoryAdmission::PaintMemoryAdmission()
  : useInMemory(false),
    voxelCount(0),
    rawVolumeBytes(0),
    maskVolumeBytes(0),
    residentVolumeBytes(0),
    estimatedPeakBytes(0),
    systemReserveBytes(0),
    integratedGpuReserveBytes(0),
    commitReserveBytes(0),
    availablePhysicalBudgetBytes(0),
    availableCommitBudgetBytes(0),
    largeVolumeThresholdBytes(0),
    reason(PaintMemoryAdmissionReason::MemoryStatusUnavailable)
{
}

PaintAlgorithmMemoryAdmission::PaintAlgorithmMemoryAdmission()
  : approved(false),
    physicalMemoryChecked(false),
    commitMemoryChecked(false),
    voxelCount(0),
    requiredBytes(0),
    systemReserveBytes(0),
    integratedGpuReserveBytes(0),
    commitReserveBytes(0),
    availablePhysicalBudgetBytes(0),
    availableCommitBudgetBytes(0),
    reason(PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable)
{
}

bool
getSystemMemoryStatus(SystemMemoryStatus& status)
{
  status = SystemMemoryStatus();

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
  const std::uint64_t reportedAvailableCommit =
    static_cast<std::uint64_t>(nativeStatus.ullAvailPageFile);
  if (status.commitLimitBytes > 0 &&
      reportedAvailableCommit <= status.commitLimitBytes)
    {
      status.committedBytes =
        status.commitLimitBytes-reportedAvailableCommit;
      status.availableCommitBytes =
        status.commitLimitBytes-status.committedBytes;
      status.availableCommitKnown = true;
    }

  return status.availablePhysicalKnown && status.availableCommitKnown;

#elif defined(__unix__) || defined(__unix) || defined(unix) || \
      (defined(__APPLE__) && defined(__MACH__))
#if defined(_SC_PHYS_PAGES)
  (void)sysconfBytes(_SC_PHYS_PAGES, status.totalPhysicalBytes);
#elif defined(_SC_AIX_REALMEM)
  {
    const long kibibytes = sysconf(_SC_AIX_REALMEM);
    if (kibibytes > 0)
      (void)checkedMultiply(static_cast<std::uint64_t>(kibibytes),
                            1024ULL, status.totalPhysicalBytes);
  }
#endif

#if defined(CTL_HW) && (defined(HW_MEMSIZE) || defined(HW_PHYSMEM64))
  if (status.totalPhysicalBytes == 0)
    {
      int mib[2] = { CTL_HW,
#if defined(HW_MEMSIZE)
                     HW_MEMSIZE
#else
                     HW_PHYSMEM64
#endif
                   };
      std::uint64_t size = 0;
      size_t length = sizeof(size);
      if (sysctl(mib, 2, &size, &length, NULL, 0) == 0)
        status.totalPhysicalBytes = size;
    }
#elif defined(CTL_HW) && (defined(HW_PHYSMEM) || defined(HW_REALMEM))
  if (status.totalPhysicalBytes == 0)
    {
      int mib[2] = { CTL_HW,
#if defined(HW_REALMEM)
                     HW_REALMEM
#else
                     HW_PHYSMEM
#endif
                   };
      unsigned int size = 0;
      size_t length = sizeof(size);
      if (sysctl(mib, 2, &size, &length, NULL, 0) == 0)
        status.totalPhysicalBytes = size;
    }
#endif

#if defined(_SC_AVPHYS_PAGES)
  status.availablePhysicalKnown =
    sysconfBytes(_SC_AVPHYS_PAGES, status.availablePhysicalBytes);
  if (status.availablePhysicalKnown && status.totalPhysicalBytes > 0)
    status.availablePhysicalBytes = std::min(
      status.availablePhysicalBytes, status.totalPhysicalBytes);
#endif

  // Portable Unix APIs do not expose a Windows-equivalent Commit Limit.
  // Callers therefore use the current physical-memory gate conservatively.
  return status.totalPhysicalBytes > 0;
#else
  return false;
#endif
}

bool
evaluatePaintMemoryAdmission(std::uint64_t depth,
                             std::uint64_t width,
                             std::uint64_t height,
                             std::uint32_t rawBytesPerVoxel,
                             std::uint32_t maskBytesPerVoxel,
                             const SystemMemoryStatus& status,
                             PaintMemoryAdmission& admission)
{
  admission = PaintMemoryAdmission();

  std::uint64_t planeVoxels = 0;
  if (depth == 0 || width == 0 || height == 0 ||
      rawBytesPerVoxel == 0 || maskBytesPerVoxel == 0 ||
      !checkedMultiply(width, height, planeVoxels) ||
      !checkedMultiply(depth, planeVoxels, admission.voxelCount) ||
      !checkedMultiply(admission.voxelCount, rawBytesPerVoxel,
                       admission.rawVolumeBytes) ||
      !checkedMultiply(admission.voxelCount, maskBytesPerVoxel,
                       admission.maskVolumeBytes) ||
      !checkedAdd(admission.rawVolumeBytes, admission.maskVolumeBytes,
                  admission.residentVolumeBytes))
    {
      admission.reason = PaintMemoryAdmissionReason::ArithmeticOverflow;
      return false;
    }
  if (!calculatePaintCommitReserve(status.totalPhysicalBytes,
                                   admission.commitReserveBytes))
    {
      admission.reason = PaintMemoryAdmissionReason::ArithmeticOverflow;
      return false;
    }

  const std::uint64_t transientVolumeBytes =
    std::max(admission.rawVolumeBytes, admission.maskVolumeBytes);
  std::uint64_t peakWithoutFixedReserve = 0;
  // Keep room for one whole-volume transient plus bounded Paint/GraphCut work.
  if (!checkedAdd(admission.residentVolumeBytes, transientVolumeBytes,
                  peakWithoutFixedReserve) ||
      !checkedAdd(peakWithoutFixedReserve, kOperationalReserveBytes,
                  admission.estimatedPeakBytes))
    {
      admission.reason = PaintMemoryAdmissionReason::ArithmeticOverflow;
      return false;
    }

  if (admission.rawVolumeBytes >
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
      admission.maskVolumeBytes >
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
      admission.reason = PaintMemoryAdmissionReason::AddressSpaceLimit;
      return true;
    }

  if (status.totalPhysicalBytes == 0 ||
      !status.availablePhysicalKnown)
    {
      admission.reason = PaintMemoryAdmissionReason::MemoryStatusUnavailable;
      return true;
    }

  // These reserves remain unused by the heap so Windows and a shared-memory
  // integrated GPU retain headroom while textures and driver staging grow.
  if (!calculatePaintMemoryReserves(
        status.totalPhysicalBytes,
        admission.systemReserveBytes,
        admission.integratedGpuReserveBytes,
        admission.largeVolumeThresholdBytes))
    {
      admission.reason = PaintMemoryAdmissionReason::ArithmeticOverflow;
      return false;
    }

  std::uint64_t combinedReserveBytes = 0;
  if (!checkedAdd(admission.systemReserveBytes,
                  admission.integratedGpuReserveBytes,
                  combinedReserveBytes))
    {
      admission.reason = PaintMemoryAdmissionReason::ArithmeticOverflow;
      return false;
    }

  admission.availablePhysicalBudgetBytes = subtractReserve(
    status.availablePhysicalBytes, combinedReserveBytes);
  if (status.availableCommitKnown)
    admission.availableCommitBudgetBytes = subtractReserve(
      status.availableCommitBytes, admission.commitReserveBytes);

  if (admission.largeVolumeThresholdBytes == 0 ||
      admission.residentVolumeBytes >= admission.largeVolumeThresholdBytes)
    {
      admission.reason = PaintMemoryAdmissionReason::LargeVolume;
      return true;
    }
  if (admission.estimatedPeakBytes >
      admission.availablePhysicalBudgetBytes)
    {
      admission.reason = PaintMemoryAdmissionReason::InsufficientPhysicalMemory;
      return true;
    }
  if (status.availableCommitKnown &&
      admission.estimatedPeakBytes > admission.availableCommitBudgetBytes)
    {
      admission.reason = PaintMemoryAdmissionReason::InsufficientCommit;
      return true;
    }

  admission.useInMemory = true;
  admission.reason = PaintMemoryAdmissionReason::InMemoryApproved;
  return true;
}

bool
calculatePaintAlgorithmRequiredBytes(std::uint64_t depth,
                                     std::uint64_t width,
                                     std::uint64_t height,
                                     std::uint64_t bytesPerVoxel,
                                     std::uint64_t fixedOverheadBytes,
                                     std::uint64_t& voxelCount,
                                     std::uint64_t& requiredBytes)
{
  voxelCount = 0;
  requiredBytes = 0;
  if (depth == 0 || width == 0 || height == 0 ||
      (bytesPerVoxel == 0 && fixedOverheadBytes == 0))
    return false;

  std::uint64_t planeVoxels = 0;
  std::uint64_t perVoxelBytes = 0;
  if (!checkedMultiply(width, height, planeVoxels) ||
      !checkedMultiply(depth, planeVoxels, voxelCount) ||
      !checkedMultiply(voxelCount, bytesPerVoxel, perVoxelBytes) ||
      !checkedAdd(perVoxelBytes, fixedOverheadBytes, requiredBytes))
    {
      voxelCount = 0;
      requiredBytes = 0;
      return false;
    }

  return true;
}

bool
queryNativePaintMemoryStatus(SystemMemoryStatus& status, void *context)
{
  (void)context;
  return getSystemMemoryStatus(status);
}

PaintAlgorithmMemoryAdmission
evaluatePaintAlgorithmMemoryAdmission(std::uint64_t depth,
                                      std::uint64_t width,
                                      std::uint64_t height,
                                      std::uint64_t bytesPerVoxel,
                                      std::uint64_t fixedOverheadBytes,
                                      PaintMemoryStatusProvider provider,
                                      void *providerContext)
{
  PaintAlgorithmMemoryAdmission admission;
  if (depth == 0 || width == 0 || height == 0 ||
      (bytesPerVoxel == 0 && fixedOverheadBytes == 0))
    {
      admission.reason = PaintAlgorithmMemoryAdmissionReason::InvalidRequest;
      return admission;
    }

  if (!calculatePaintAlgorithmRequiredBytes(
        depth, width, height, bytesPerVoxel, fixedOverheadBytes,
        admission.voxelCount, admission.requiredBytes))
    {
      admission.reason =
        PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow;
      return admission;
    }
  if (admission.requiredBytes >
      static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
      admission.reason = PaintAlgorithmMemoryAdmissionReason::AddressSpaceLimit;
      return admission;
    }

  if (!provider)
    provider = queryNativePaintMemoryStatus;

  SystemMemoryStatus status;
  if (!provider(status, providerContext) ||
      !status.availablePhysicalKnown || status.totalPhysicalBytes == 0)
    {
      admission.reason =
        PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable;
      return admission;
    }

  if (!calculatePaintCommitReserve(status.totalPhysicalBytes,
                                   admission.commitReserveBytes))
    {
      admission.reason =
        PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow;
      return admission;
    }

  std::uint64_t unusedLargeVolumeThreshold = 0;
  if (!calculatePaintMemoryReserves(
        status.totalPhysicalBytes,
        admission.systemReserveBytes,
        admission.integratedGpuReserveBytes,
        unusedLargeVolumeThreshold))
    {
      admission.reason =
        PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow;
      return admission;
    }

  std::uint64_t combinedReserveBytes = 0;
  if (!checkedAdd(admission.systemReserveBytes,
                  admission.integratedGpuReserveBytes,
                  combinedReserveBytes))
    {
      admission.reason =
        PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow;
      return admission;
    }

  const std::uint64_t availablePhysicalBytes = std::min(
    status.availablePhysicalBytes, status.totalPhysicalBytes);
  admission.physicalMemoryChecked = true;
  admission.availablePhysicalBudgetBytes = subtractReserve(
    availablePhysicalBytes, combinedReserveBytes);

  const bool validCommitStatus =
    status.availableCommitKnown && status.commitLimitBytes > 0 &&
    status.availableCommitBytes <= status.commitLimitBytes;
#if defined(_WIN32)
  // A Windows allocation can fail when the system Commit Limit is exhausted
  // even if the working-set counter still reports free physical pages.
  if (!validCommitStatus)
    {
      admission.reason =
        PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable;
      return admission;
    }
#endif
  if (validCommitStatus)
    {
      admission.commitMemoryChecked = true;
      admission.availableCommitBudgetBytes = subtractReserve(
        status.availableCommitBytes, admission.commitReserveBytes);
    }

  if (admission.requiredBytes > admission.availablePhysicalBudgetBytes)
    {
      admission.reason =
        PaintAlgorithmMemoryAdmissionReason::InsufficientPhysicalMemory;
      return admission;
    }
  if (admission.commitMemoryChecked &&
      admission.requiredBytes > admission.availableCommitBudgetBytes)
    {
      admission.reason = PaintAlgorithmMemoryAdmissionReason::InsufficientCommit;
      return admission;
    }

  admission.approved = true;
  admission.reason = PaintAlgorithmMemoryAdmissionReason::Approved;
  return admission;
}

std::size_t
getMemorySize()
{
  SystemMemoryStatus status;
  if (!getSystemMemoryStatus(status))
    return 0;
  const std::uint64_t maximum =
    static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max());
  return static_cast<std::size_t>(std::min(status.totalPhysicalBytes, maximum));
}
