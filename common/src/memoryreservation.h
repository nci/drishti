#ifndef DRISHTI_MEMORYRESERVATION_H
#define DRISHTI_MEMORYRESERVATION_H

#include <cstdint>

// Process-local reservation for allocations that can compete with the
// renderer, Import and Paint worksets.  The reservation is deliberately
// independent of the OS memory counters: callers provide the latest budgets
// and this class closes the check/acquire race between concurrent tasks.
class ProcessMemoryReservation
{
 public:
  ProcessMemoryReservation();
  ~ProcessMemoryReservation();

  ProcessMemoryReservation(const ProcessMemoryReservation&) = delete;
  ProcessMemoryReservation& operator=(const ProcessMemoryReservation&) = delete;

  ProcessMemoryReservation(ProcessMemoryReservation&& other) noexcept;
  ProcessMemoryReservation& operator=(ProcessMemoryReservation&& other) noexcept;

  bool acquire(std::uint64_t bytes,
               std::uint64_t availablePhysicalBudgetBytes,
               bool commitMemoryChecked = false,
               std::uint64_t availableCommitBudgetBytes = 0);
  void release();
  bool active() const;
  std::uint64_t bytes() const;

  static std::uint64_t processReservedBytes();

 private:
  std::uint64_t m_bytes;
};

#endif
