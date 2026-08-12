#include "framebufferbudget.h"

#include <iostream>
#include <limits>

namespace
{
int fail(const char *message)
{
  std::cerr << "FAILED: " << message << '\n';
  return 1;
}
}

int main()
{
  const std::uint64_t mib = 1024ULL*1024ULL;

  FramebufferBudget::Admission admission =
    FramebufferBudget::evaluate(1920, 1080, 100, 512ULL*mib, 16384);
  if (!admission.approved || admission.requiredBytes != 207360000ULL)
    return fail("1080p Mesh framebuffer should fit the budget");

  admission = FramebufferBudget::evaluate(3840, 2160, 100,
                                           512ULL*mib, 16384);
  if (admission.approved ||
      admission.reason != FramebufferBudget::RejectionReason::MemoryBudgetExceeded)
    return fail("4K Mesh framebuffer should be rejected before allocation");

  admission = FramebufferBudget::evaluate(16385, 100, 68,
                                           512ULL*mib, 16384);
  if (admission.reason !=
      FramebufferBudget::RejectionReason::HardwareDimensionLimit)
    return fail("hardware dimension limit was not enforced");

  admission = FramebufferBudget::evaluate(
    std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
    std::numeric_limits<std::uint64_t>::max(), 512ULL*mib,
    std::numeric_limits<int>::max());
  if (admission.reason != FramebufferBudget::RejectionReason::ArithmeticOverflow)
    return fail("framebuffer byte overflow was not rejected");

  admission = FramebufferBudget::evaluate(0, 1080, 68, 512ULL*mib, 16384);
  if (admission.reason != FramebufferBudget::RejectionReason::InvalidRequest)
    return fail("invalid dimensions were not rejected");

  std::cout << "Framebuffer budget smoke passed\n";
  return 0;
}
