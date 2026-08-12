#ifndef IMPORTMEMORYADMISSION_H
#define IMPORTMEMORYADMISSION_H

#include <cstddef>
#include <cstdint>

struct ImportMemoryStatus
{
  std::uint64_t totalPhysicalBytes;
  std::uint64_t availablePhysicalBytes;
  std::uint64_t commitLimitBytes;
  std::uint64_t availableCommitBytes;
  bool availablePhysicalKnown;
  bool availableCommitKnown;
  bool commitRequired;

  ImportMemoryStatus();
};

enum class ImportMemoryAdmissionReason
{
  Approved,
  InvalidRequest,
  ArithmeticOverflow,
  AddressSpaceLimit,
  MemoryStatusUnavailable,
  InsufficientPhysicalMemory,
  InsufficientCommit
};

struct ImportMemoryAdmission
{
  bool approved;
  bool physicalMemoryChecked;
  bool commitMemoryChecked;
  std::uint64_t requiredBytes;
  std::uint64_t systemReserveBytes;
  std::uint64_t integratedGpuReserveBytes;
  std::uint64_t availablePhysicalBudgetBytes;
  std::uint64_t availableCommitBudgetBytes;
  ImportMemoryAdmissionReason reason;

  ImportMemoryAdmission();
};

using ImportMemoryStatusProvider = bool (*)(ImportMemoryStatus& status,
                                            void *context);

bool checkedImportMultiply(std::uint64_t first,
                           std::uint64_t second,
                           std::uint64_t& result);
bool checkedImportAdd(std::uint64_t first,
                      std::uint64_t second,
                      std::uint64_t& result);
bool checkedImportImageDecodeWorkingSet(std::uint64_t pixelCount,
                                        std::uint64_t codecSafetyBytes,
                                        std::uint64_t& requiredBytes);
bool queryImportMemoryStatus(ImportMemoryStatus& status, void *context);

// requiredBytes contains only incremental allocations for the next import
// stage. The point-in-time query already accounts for resident source data.
// Reserves keep Windows and shared-memory graphics responsive while the stage
// runs. Tests can inject a provider without changing process-global state.
ImportMemoryAdmission evaluateImportMemoryAdmission(
  std::uint64_t requiredBytes,
  ImportMemoryStatusProvider provider = nullptr,
  void *providerContext = nullptr);

#endif
