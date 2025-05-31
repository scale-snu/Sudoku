#ifndef SUDOKU_SUDOKU_ADDRESSING_H
#define SUDOKU_SUDOKU_ADDRESSING_H

#include <cstdint>
#include <vector>

#include "sudoku.h"

namespace sudoku {

/// @brief Configuration struct for reverse-engineering DRAM address mapping
/// function
struct AddressingConfig {
  std::string type;
  std::string fname_prefix;
  bool verbose;
  bool debug;
  bool logging;

  AddressingConfig()
      : type("ddr4"),
        fname_prefix("default"),
        verbose(false),
        debug(false),
        logging(false) {}

  AddressingConfig(std::string f, bool v, bool d, bool l)
      : type("ddr4"), fname_prefix(f), verbose(v), debug(d), logging(l) {}

  AddressingConfig(std::string t, std::string f, bool v, bool d, bool l)
      : type(t), fname_prefix(f), verbose(v), debug(d), logging(l) {}
};

/// @brief Constraints for address generation
struct Constraints {
  std::vector<uint64_t> same_functions;
  std::vector<uint64_t> diff_functions;
  uint64_t row_mask;
  uint64_t column_mask;

  Constraints(std::vector<uint64_t> sf, std::vector<uint64_t> df, uint64_t r,
              uint64_t c)
      : same_functions(sf), diff_functions(df), row_mask(r), column_mask(c) {}
};

/// @brief class for reverse-engineering DRAM address mapping functions
class Addressing : public Sudoku {
 public:
  Addressing() = delete;
  Addressing(const Addressing&) = delete;
  Addressing& operator=(const Addressing&) = delete;

  Addressing(MemoryPoolConfig* memory_pool_config,
             AddressingConfig* addressing_config);
  Addressing(DRAMConfig* dram_config, MemoryConfig* memory_config,
             MemoryPoolConfig* memory_pool_config,
             AddressingConfig* addressing_config);
  virtual ~Addressing() = default;

  // Testing functions (conflicts)
  void StatSingleMemoryAccess();  // latency: avg,med,min,max
  void StatPairedMemoryAccess();  // latency: avg,med,min,max
  void CheckPairedMemoryAccess(Constraints& constraints);

  // Testing functions (refreshes)
  void WatchRefreshSingleAccess();  // watch periodic latency spikes
  void StatRefIntervalSingleAccess(
      uint64_t threshold);  // interval: avg,med,min,max

  void WatchRefreshPairedAccessCoarse();  // watch periodic latency spikes
  void StatRefIntervalPairedAccessCoarse(uint64_t threshold);
  void CheckRefIntervalPairedAccessCoarse(Constraints& constraints,
                                          uint64_t threshold);

  void WatchRefreshPairedAccessFine();  // watch periodic latency spikes
  void StatRefIntervalPairedAccessFine(uint64_t threshold);
  void CheckRefIntervalPairedAccessFine(Constraints& constraints,
                                        uint64_t threshold);

  // Testing functions (consecutive memory accesses)
  // For now, we only check the consecutive reads (read-read)
  void StatReadReadAccess(uint64_t length);
  void CheckReadReadAccess(Constraints& constraints, uint64_t length);

  // (optional) Reverse-engineering DRAM addressing functions
  // "DRAMA: Exploiting DRAM Addressing for Cross-CPU Attacks," SEC, 2016
  void ReverseAddressingFunctions();

  // Step 1. Identifying row and column bits. First, classify physical address
  //  bits into 1) uncovered/unused bits and 2) covered/used bits. Then, we
  //  check the uncovered bits and covered bits for identifying row and column
  //  bits by detecting row buffer conflicts.
  void IdentifyBits(std::vector<uint64_t> functions);

  // Step 2. Verifying whether the derived functions satisfy the one-to-one
  //  mapping property of the addressing mapping system. We use the rank-nullity
  //  theoremn by checking whether the number of functions equals to the number
  //  of bits in each set.
  bool ValidateAddressMapping();

  // Step 3. Decomposing (breaking down) DRAM addressing functions to
  //  component-level functions.
  void DecomposeUsingRefreshes();
  void DecomposeUsingConsecutiveAccesses();

 private:
  // From "DRAMA: Exploiting DRAM Addressing for Cross-CPU Attacks," SEC, 2016
  // Brute-forcing to collect row buffer conflicts address pairs
  bool EnoughSameBankPairs();
  void CollectSameBankPairs(std::string log_name);
  // From "DRAMA: Exploiting DRAM Addressing for Cross-CPU Attacks," SEC, 2016
  // Brute-forcing for deriving functions then, using Gaussian Elimination to
  // reduce functions
  std::vector<uint64_t> DeriveFunctions(
      std::vector<std::vector<addr_tuple>> pairs, std::string log_name);

  // From "ZenHammer: Rowhammer Attacks on AMD Zen-based Platforms," SEC, 2024
  // Offset PCI_OFFSET for lower physical address space (just subtract PCI
  // address region from physical addresses)
  void SlideOffsets(uint64_t offset);
  // Additionally check and filter-out outlier when collecting same-bank,
  // different row address pairs
  void FilterSameBankPairs(std::string log_name);

  std::vector<uint64_t> MergeFunctionsToDisjointSets(
      std::vector<uint64_t> functions);
  void CheckUnusedBits(uint64_t bitmask, std::string log_name);
  void CheckUsedBits(std::vector<uint64_t> disjoint_sets, std::string log_name);

  bool CheckInjectivity(std::vector<uint64_t> disjoint_sets,
                        std::vector<uint64_t>& incomplete_sets);
  void ResolveAddressMapping(std::vector<uint64_t> incomplete_sets,
                             std::string log_name);

 private:
  AddressingConfig* addressing_config_;
  std::vector<std::vector<addr_tuple>> address_pairs_;
  std::vector<std::vector<addr_tuple>> sbdr_pairs_;
  std::vector<uint64_t> covered_bits_;
  std::vector<uint64_t> uncovered_bits_;
};

}  // namespace sudoku

#endif  // SUDOKU_SUDOKU_ADDRESSING_H
