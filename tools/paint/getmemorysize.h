#ifndef GETMEMORYSIZE_H
#define GETMEMORYSIZE_H

/*
 * Original getMemorySize implementation:
 * David Robert Nadeau, http://NadeauSoftware.com/
 * Creative Commons Attribution 3.0 Unported License.
 */

#include <cstddef>
#include <cstdint>
#include <memory>

#include "../../common/src/memoryreservation.h"


struct SystemMemoryStatus
{
  std::uint64_t totalPhysicalBytes;
  std::uint64_t availablePhysicalBytes;
  std::uint64_t commitLimitBytes;
  std::uint64_t committedBytes;
  std::uint64_t availableCommitBytes;
  bool availablePhysicalKnown;
  bool availableCommitKnown;

  SystemMemoryStatus();
};

enum class PaintMemoryAdmissionReason
{
  InMemoryApproved,
  MemoryStatusUnavailable,
  AddressSpaceLimit,
  LargeVolume,
  InsufficientPhysicalMemory,
  InsufficientCommit,
  ArithmeticOverflow
};

struct PaintMemoryAdmission
{
  bool useInMemory;
  std::uint64_t voxelCount;
  std::uint64_t rawVolumeBytes;
  std::uint64_t maskVolumeBytes;
  std::uint64_t residentVolumeBytes;
  std::uint64_t estimatedPeakBytes;
  std::uint64_t systemReserveBytes;
  std::uint64_t integratedGpuReserveBytes;
  std::uint64_t commitReserveBytes;
  std::uint64_t availablePhysicalBudgetBytes;
  std::uint64_t availableCommitBudgetBytes;
  std::uint64_t largeVolumeThresholdBytes;
  PaintMemoryAdmissionReason reason;

  PaintMemoryAdmission();
};

enum class PaintAlgorithmMemoryAdmissionReason
{
  Approved,
  InvalidRequest,
  MemoryStatusUnavailable,
  AddressSpaceLimit,
  InsufficientPhysicalMemory,
  InsufficientCommit,
  ArithmeticOverflow
};

struct PaintAlgorithmMemoryAdmission
{
  bool approved;
  bool physicalMemoryChecked;
  bool commitMemoryChecked;
  std::uint64_t voxelCount;
  std::uint64_t requiredBytes;
  std::uint64_t systemReserveBytes;
  std::uint64_t integratedGpuReserveBytes;
  std::uint64_t commitReserveBytes;
  std::uint64_t availablePhysicalBudgetBytes;
  std::uint64_t availableCommitBudgetBytes;
  PaintAlgorithmMemoryAdmissionReason reason;
  std::shared_ptr<ProcessMemoryReservation> reservation;

  PaintAlgorithmMemoryAdmission();
};

// A provider returns a point-in-time memory snapshot.  The context argument
// lets tests and callers supply state without changing process-global hooks.
using PaintMemoryStatusProvider = bool (*)(SystemMemoryStatus& status,
                                           void *context);

bool getSystemMemoryStatus(SystemMemoryStatus& status);

bool evaluatePaintMemoryAdmission(std::uint64_t depth,
                                  std::uint64_t width,
                                  std::uint64_t height,
                                  std::uint32_t rawBytesPerVoxel,
                                  std::uint32_t maskBytesPerVoxel,
                                  const SystemMemoryStatus& status,
                                  PaintMemoryAdmission& admission);

bool reservePaintVolumeMemory(
  const PaintMemoryAdmission& admission,
  std::shared_ptr<ProcessMemoryReservation>& reservation);

bool calculatePaintAlgorithmRequiredBytes(std::uint64_t depth,
                                          std::uint64_t width,
                                          std::uint64_t height,
                                          std::uint64_t bytesPerVoxel,
                                          std::uint64_t fixedOverheadBytes,
                                          std::uint64_t& voxelCount,
                                          std::uint64_t& requiredBytes);

bool queryNativePaintMemoryStatus(SystemMemoryStatus& status, void *context);

// Only incremental algorithm allocations belong in bytesPerVoxel and
// fixedOverheadBytes.  The already-resident raw volume and mask must not be
// counted again.  Windows requires both physical and Commit headroom; other
// platforms use the physical gate when no equivalent Commit metric exists.
// Passing a null provider selects the native system query.
PaintAlgorithmMemoryAdmission evaluatePaintAlgorithmMemoryAdmission(
  std::uint64_t depth,
  std::uint64_t width,
  std::uint64_t height,
  std::uint64_t bytesPerVoxel,
  std::uint64_t fixedOverheadBytes,
  PaintMemoryStatusProvider provider = nullptr,
  void *providerContext = nullptr);

bool reservePaintAlgorithmMemory(PaintAlgorithmMemoryAdmission& admission);

// Retained for source compatibility with older callers.
std::size_t getMemorySize();

#endif
