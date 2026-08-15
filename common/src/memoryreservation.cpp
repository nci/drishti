#include "memoryreservation.h"

#include <limits>
#include <mutex>

namespace
{
std::mutex g_reservationMutex;
std::uint64_t g_reservedBytes = 0;
}

ProcessMemoryReservation::ProcessMemoryReservation()
  : m_bytes(0)
{
}

ProcessMemoryReservation::~ProcessMemoryReservation()
{
  release();
}

ProcessMemoryReservation::ProcessMemoryReservation(
  ProcessMemoryReservation&& other) noexcept
  : m_bytes(other.m_bytes)
{
  other.m_bytes = 0;
}

ProcessMemoryReservation&
ProcessMemoryReservation::operator=(ProcessMemoryReservation&& other) noexcept
{
  if (this == &other)
    return *this;
  release();
  m_bytes = other.m_bytes;
  other.m_bytes = 0;
  return *this;
}

bool
ProcessMemoryReservation::acquire(
  std::uint64_t bytes,
  std::uint64_t availablePhysicalBudgetBytes,
  bool commitMemoryChecked,
  std::uint64_t availableCommitBudgetBytes)
{
  if (bytes == 0 || m_bytes != 0)
    return false;

  if (commitMemoryChecked &&
      availableCommitBudgetBytes < bytes)
    return false;

  std::lock_guard<std::mutex> lock(g_reservationMutex);
  if (g_reservedBytes > availablePhysicalBudgetBytes ||
      bytes > availablePhysicalBudgetBytes-g_reservedBytes)
    return false;
  if (commitMemoryChecked &&
      (g_reservedBytes > availableCommitBudgetBytes ||
       bytes > availableCommitBudgetBytes-g_reservedBytes))
    return false;
  if (bytes > std::numeric_limits<std::uint64_t>::max()-g_reservedBytes)
    return false;

  g_reservedBytes += bytes;
  m_bytes = bytes;
  return true;
}

void
ProcessMemoryReservation::release()
{
  if (m_bytes == 0)
    return;

  std::lock_guard<std::mutex> lock(g_reservationMutex);
  if (m_bytes <= g_reservedBytes)
    g_reservedBytes -= m_bytes;
  else
    g_reservedBytes = 0;
  m_bytes = 0;
}

bool ProcessMemoryReservation::active() const { return m_bytes != 0; }
std::uint64_t ProcessMemoryReservation::bytes() const { return m_bytes; }

std::uint64_t
ProcessMemoryReservation::processReservedBytes()
{
  std::lock_guard<std::mutex> lock(g_reservationMutex);
  return g_reservedBytes;
}
