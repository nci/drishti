#ifndef FRAMEBUFFERBUDGET_H
#define FRAMEBUFFERBUDGET_H

#include <cstdint>
#include <limits>

namespace FramebufferBudget
{
enum class RejectionReason
{
  Approved,
  InvalidRequest,
  HardwareDimensionLimit,
  ArithmeticOverflow,
  MemoryBudgetExceeded
};

struct Admission
{
  bool approved;
  std::uint64_t pixelCount;
  std::uint64_t requiredBytes;
  std::uint64_t budgetBytes;
  RejectionReason reason;

  Admission()
    : approved(false),
      pixelCount(0),
      requiredBytes(0),
      budgetBytes(0),
      reason(RejectionReason::InvalidRequest)
  {
  }
};

inline bool checkedMultiply(std::uint64_t first,
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

// bytesPerPixel is the sum of all color and depth/stencil attachments that
// coexist for this framebuffer set.  The caller supplies the stricter of the
// texture and renderbuffer dimension limits reported by the active context.
inline Admission evaluate(int width,
                          int height,
                          std::uint64_t bytesPerPixel,
                          std::uint64_t budgetBytes,
                          int maximumDimension)
{
  Admission admission;
  admission.budgetBytes = budgetBytes;

  if (width <= 0 || height <= 0 || bytesPerPixel == 0 ||
      budgetBytes == 0 || maximumDimension <= 0)
    return admission;

  if (width > maximumDimension || height > maximumDimension)
    {
      admission.reason = RejectionReason::HardwareDimensionLimit;
      return admission;
    }

  if (!checkedMultiply(static_cast<std::uint64_t>(width),
                       static_cast<std::uint64_t>(height),
                       admission.pixelCount) ||
      !checkedMultiply(admission.pixelCount, bytesPerPixel,
                       admission.requiredBytes))
    {
      admission.pixelCount = 0;
      admission.requiredBytes = 0;
      admission.reason = RejectionReason::ArithmeticOverflow;
      return admission;
    }

  if (admission.requiredBytes > budgetBytes)
    {
      admission.reason = RejectionReason::MemoryBudgetExceeded;
      return admission;
    }

  admission.approved = true;
  admission.reason = RejectionReason::Approved;
  return admission;
}
}

#endif
