#define DRISHTI_VOLUMEOPERATIONS_MEMORY_PROFILE_ONLY
#include "volumeoperations.h"

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
  unsigned int calls;
  SystemMemoryStatus status;

  FakeMemoryProvider() : calls(0) {}
};

bool provideMemoryStatus(SystemMemoryStatus& status, void *context)
{
  FakeMemoryProvider *provider =
    static_cast<FakeMemoryProvider *>(context);
  ++provider->calls;
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

int fail(const std::string& message)
{
  std::cerr << "FAILED: " << message << std::endl;
  return 1;
}
}

int main()
{
  if (paintVolumeMaximumMaskLabel() != 65535 ||
      !paintVolumeOffsetLabelRangeFits(0, 65535) ||
      paintVolumeOffsetLabelRangeFits(0, 65536) ||
      !paintVolumeOffsetLabelRangeFits(65530, 5) ||
      paintVolumeOffsetLabelRangeFits(65530, 6) ||
      paintVolumeOffsetLabelRangeFits(65536, 0))
    return fail("16-bit mask-label boundary checks are incorrect");

  PaintVolumeUniqueLabelTracker labelTracker;
  if (!labelTracker.add(0) ||
      !labelTracker.add(7) ||
      !labelTracker.add(7) ||
      !labelTracker.add(65535) ||
      labelTracker.add(65536) ||
      labelTracker.uniqueLabelCount() != 2 ||
      labelTracker.maximumLabel() != 65535)
    return fail("bounded unique-label tracking is incorrect");

  const PaintVolumeAlgorithmMemoryProfile connected =
    paintVolumeAlgorithmMemoryProfile(
      PaintVolumeAlgorithm::ConnectedComponents);
  const PaintVolumeAlgorithmMemoryProfile distance =
    paintVolumeAlgorithmMemoryProfile(
      PaintVolumeAlgorithm::DistanceTransform);
  const PaintVolumeAlgorithmMemoryProfile thickness =
    paintVolumeAlgorithmMemoryProfile(
      PaintVolumeAlgorithm::LocalThickness);
  const PaintVolumeAlgorithmMemoryProfile watershed =
    paintVolumeAlgorithmMemoryProfile(
      PaintVolumeAlgorithm::Watershed);

  if (connected.bytesPerVoxel != 192 ||
      connected.fixedOverheadBytes != 64*kMiB ||
      connected.distanceTaskBytesPerScanline != 0 ||
      connected.visibilityTaskBytesPerSlice != 512 ||
      distance.bytesPerVoxel != 32 ||
      distance.fixedOverheadBytes != 64*kMiB ||
      distance.distanceTaskBytesPerScanline != 512 ||
      distance.visibilityTaskBytesPerSlice != 0 ||
      thickness.bytesPerVoxel != 64 ||
      thickness.fixedOverheadBytes != 64*kMiB ||
      thickness.distanceTaskBytesPerScanline != 512 ||
      thickness.visibilityTaskBytesPerSlice != 0 ||
      watershed.bytesPerVoxel != 160 ||
      watershed.fixedOverheadBytes != 128*kMiB ||
      watershed.distanceTaskBytesPerScanline != 512 ||
      watershed.visibilityTaskBytesPerSlice != 0)
    return fail("a whole-volume algorithm memory profile changed unexpectedly");

  FakeMemoryProvider ample;
  ample.status = knownMemory(64*kGiB, 56*kGiB,
                             128*kGiB, 96*kGiB);

  const std::uint64_t voxelCount = 10ULL*20ULL*30ULL;
  const PaintAlgorithmMemoryAdmission connectedAdmission =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::ConnectedComponents,
      10, 20, 30, provideMemoryStatus, &ample);
  if (!connectedAdmission.approved ||
      connectedAdmission.voxelCount != voxelCount ||
      connectedAdmission.requiredBytes !=
        voxelCount*connected.bytesPerVoxel+
        connected.fixedOverheadBytes+
        10ULL*connected.visibilityTaskBytesPerSlice)
    return fail("connected-component profile estimate is incorrect");

  const PaintAlgorithmMemoryAdmission distanceAdmission =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::DistanceTransform,
      10, 20, 30, provideMemoryStatus, &ample);
  const PaintAlgorithmMemoryAdmission thicknessAdmission =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::LocalThickness,
      10, 20, 30, provideMemoryStatus, &ample);
  const PaintAlgorithmMemoryAdmission watershedAdmission =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::Watershed,
      10, 20, 30, provideMemoryStatus, &ample);
  if (!distanceAdmission.approved || !thicknessAdmission.approved ||
      !watershedAdmission.approved ||
      distanceAdmission.requiredBytes !=
        voxelCount*distance.bytesPerVoxel+distance.fixedOverheadBytes+
        10ULL*20ULL*distance.distanceTaskBytesPerScanline+30ULL*1024ULL ||
      thicknessAdmission.requiredBytes !=
        voxelCount*thickness.bytesPerVoxel+thickness.fixedOverheadBytes+
        10ULL*20ULL*thickness.distanceTaskBytesPerScanline+30ULL*1024ULL ||
      watershedAdmission.requiredBytes !=
        voxelCount*watershed.bytesPerVoxel+watershed.fixedOverheadBytes+
        10ULL*20ULL*watershed.distanceTaskBytesPerScanline+30ULL*1024ULL)
    return fail("distance/local-thickness/watershed estimates are incorrect");

  FakeMemoryProvider shape;
  shape.status = ample.status;
  const PaintAlgorithmMemoryAdmission narrowShape =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::DistanceTransform,
      100, 100, 1, provideMemoryStatus, &shape);
  const PaintAlgorithmMemoryAdmission broadShape =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::DistanceTransform,
      10, 10, 100, provideMemoryStatus, &shape);
  if (!narrowShape.approved || !broadShape.approved ||
      narrowShape.voxelCount != broadShape.voxelCount ||
      narrowShape.requiredBytes <= broadShape.requiredBytes)
    return fail("distance-transform scanline shape overhead was not modeled");

  const PaintAlgorithmMemoryAdmission narrowConnectedShape =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::ConnectedComponents,
      10000, 1, 1, provideMemoryStatus, &shape);
  const PaintAlgorithmMemoryAdmission broadConnectedShape =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::ConnectedComponents,
      1, 100, 100, provideMemoryStatus, &shape);
  if (!narrowConnectedShape.approved || !broadConnectedShape.approved ||
      narrowConnectedShape.voxelCount != broadConnectedShape.voxelCount ||
      narrowConnectedShape.requiredBytes <= broadConnectedShape.requiredBytes)
    return fail("connected-component visibility-task overhead was not modeled");

  FakeMemoryProvider physicalBound;
  physicalBound.status = knownMemory(16*kGiB, 5*kGiB,
                                     64*kGiB, 48*kGiB);
  const PaintAlgorithmMemoryAdmission physicalRejection =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::ConnectedComponents,
      320, 320, 320, provideMemoryStatus, &physicalBound);
  if (physicalRejection.approved ||
      physicalRejection.reason !=
        PaintAlgorithmMemoryAdmissionReason::InsufficientPhysicalMemory)
    return fail("connected components ignored the physical-memory gate");

  FakeMemoryProvider commitBound;
  commitBound.status = knownMemory(128*kGiB, 120*kGiB,
                                   64*kGiB, 10*kGiB);
  const PaintAlgorithmMemoryAdmission commitRejection =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::Watershed,
      512, 512, 512, provideMemoryStatus, &commitBound);
  if (commitRejection.approved ||
      commitRejection.reason !=
        PaintAlgorithmMemoryAdmissionReason::InsufficientCommit)
    return fail("watershed ignored the Windows Commit-style gate");

  FakeMemoryProvider overflow;
  const PaintAlgorithmMemoryAdmission overflowRejection =
    evaluatePaintVolumeAlgorithmMemoryAdmission(
      PaintVolumeAlgorithm::Watershed,
      std::numeric_limits<std::uint64_t>::max(), 2, 1,
      provideMemoryStatus, &overflow);
  if (overflowRejection.approved ||
      overflowRejection.reason !=
        PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow ||
      overflow.calls != 0)
    return fail("overflow was not rejected before the memory query");

  if (ample.calls != 4 || physicalBound.calls != 1 ||
      commitBound.calls != 1 || shape.calls != 4)
    return fail("memory status was not queried exactly once per decision");

  std::cout << "Volume operations memory admission smoke passed" << std::endl;
  return 0;
}
