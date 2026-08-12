#ifndef ITKMEMORYADMISSION_H
#define ITKMEMORYADMISSION_H

#include "../../../tools/paint/getmemorysize.h"

#include <QString>

#include <cstdint>
#include <stdexcept>

enum class ItkMemoryWorkload
{
  BinaryThinning,
  ConnectedComponents,
  DistanceMap,
  EdgePreserving,
  Smoothing,
  VesselEnhancingDiffusion
};

struct ItkMemoryProfile
{
  const char *operation;
  std::uint64_t bytesPerVoxel;
  std::uint64_t fixedOverheadBytes;
};

inline ItkMemoryProfile
itkMemoryProfile(ItkMemoryWorkload workload)
{
  const std::uint64_t mib = 1024ULL*1024ULL;
  // These are incremental peak estimates for the plugin-owned source volume,
  // ITK input/output images, filter work buffers and bounded runtime overhead.
  // Diffusion filters retain several scalar/vector/tensor images concurrently,
  // so their profiles deliberately leave substantially more headroom.
  switch (workload)
    {
    case ItkMemoryWorkload::BinaryThinning:
      return { "Binary thinning", 32ULL, 64ULL*mib };
    case ItkMemoryWorkload::ConnectedComponents:
      return { "Connected components", 48ULL, 64ULL*mib };
    case ItkMemoryWorkload::DistanceMap:
      return { "Distance map", 48ULL, 64ULL*mib };
    case ItkMemoryWorkload::EdgePreserving:
      return { "Edge-preserving filter", 128ULL, 128ULL*mib };
    case ItkMemoryWorkload::Smoothing:
      return { "Smoothing filter", 96ULL, 128ULL*mib };
    case ItkMemoryWorkload::VesselEnhancingDiffusion:
      return { "Vessel-enhancing diffusion", 256ULL, 256ULL*mib };
    }

  return { "ITK operation", 0ULL, 0ULL };
}

inline PaintAlgorithmMemoryAdmission
evaluateItkMemoryAdmission(ItkMemoryWorkload workload,
                           std::uint64_t depth,
                           std::uint64_t width,
                           std::uint64_t height,
                           PaintMemoryStatusProvider provider = nullptr,
                           void *providerContext = nullptr)
{
  const ItkMemoryProfile profile = itkMemoryProfile(workload);
  return evaluatePaintAlgorithmMemoryAdmission(
    depth, width, height,
    profile.bytesPerVoxel, profile.fixedOverheadBytes,
    provider, providerContext);
}

inline QString
itkMemoryAmount(std::uint64_t bytes)
{
  const std::uint64_t mib = 1024ULL*1024ULL;
  const std::uint64_t roundedMiB = bytes/mib + (bytes%mib != 0 ? 1ULL : 0ULL);
  return QStringLiteral("%1 MiB").arg(static_cast<qulonglong>(roundedMiB));
}

inline QString
itkMemoryBudget(bool checked, std::uint64_t bytes)
{
  return checked ? itkMemoryAmount(bytes) : QStringLiteral("unavailable");
}

inline QString
itkMemoryAdmissionMessage(ItkMemoryWorkload workload,
                          const PaintAlgorithmMemoryAdmission& admission)
{
  QString reason;
  switch (admission.reason)
    {
    case PaintAlgorithmMemoryAdmissionReason::InvalidRequest:
      reason = QStringLiteral("The volume dimensions are invalid.");
      break;
    case PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable:
      reason = QStringLiteral("Current system-memory status is unavailable.");
      break;
    case PaintAlgorithmMemoryAdmissionReason::AddressSpaceLimit:
      reason = QStringLiteral("The request exceeds this process address space.");
      break;
    case PaintAlgorithmMemoryAdmissionReason::InsufficientPhysicalMemory:
      reason = QStringLiteral("There is not enough physical-memory headroom.");
      break;
    case PaintAlgorithmMemoryAdmissionReason::InsufficientCommit:
      reason = QStringLiteral("There is not enough Windows Commit headroom.");
      break;
    case PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow:
      reason = QStringLiteral("The working-memory estimate overflowed.");
      break;
    case PaintAlgorithmMemoryAdmissionReason::Approved:
      reason = QStringLiteral("The request was approved.");
      break;
    }

  const ItkMemoryProfile profile = itkMemoryProfile(workload);
  const QString required =
    admission.reason == PaintAlgorithmMemoryAdmissionReason::InvalidRequest ||
    admission.reason == PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow ?
    QStringLiteral("unrepresentable") : itkMemoryAmount(admission.requiredBytes);

  return QStringLiteral("%1 memory admission was rejected. Required: %2; "
                        "physical budget: %3; Commit budget: %4. %5")
    .arg(QString::fromLatin1(profile.operation),
         required,
         itkMemoryBudget(admission.physicalMemoryChecked,
                         admission.availablePhysicalBudgetBytes),
         itkMemoryBudget(admission.commitMemoryChecked,
                         admission.availableCommitBudgetBytes),
         reason);
}

inline void
requireItkMemoryAdmission(ItkMemoryWorkload workload,
                          std::uint64_t depth,
                          std::uint64_t width,
                          std::uint64_t height,
                          PaintMemoryStatusProvider provider = nullptr,
                          void *providerContext = nullptr)
{
  const PaintAlgorithmMemoryAdmission admission =
    evaluateItkMemoryAdmission(workload, depth, width, height,
                               provider, providerContext);
  if (!admission.approved)
    throw std::runtime_error(
      itkMemoryAdmissionMessage(workload, admission).toStdString());
}

#endif
