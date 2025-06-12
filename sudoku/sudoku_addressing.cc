#include "sudoku_addressing.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <random>
#include <thread>
#include <vector>

#include "internal/assembly.h"
#include "internal/conflicts.h"
#include "internal/consecutive_accesses.h"
#include "internal/constants.h"
#include "internal/refreshes.h"
#include "internal/utils.h"

namespace sudoku {

Addressing::Addressing(MemoryPoolConfig* memory_pool_config,
                       AddressingConfig* addressing_config)
    : Sudoku(nullptr, nullptr, memory_pool_config, addressing_config->type,
             addressing_config->fname_prefix, addressing_config->verbose,
             addressing_config->logging, addressing_config->debug),
      addressing_config_(addressing_config) {}

Addressing::Addressing(DRAMConfig* dram_config, MemoryConfig* memory_config,
                       MemoryPoolConfig* memory_pool_config,
                       AddressingConfig* addressing_config)
    : Sudoku(dram_config, memory_config, memory_pool_config,
             addressing_config->type, addressing_config->fname_prefix,
             addressing_config->verbose, addressing_config->logging,
             addressing_config->debug),
      addressing_config_(addressing_config) {}

void Addressing::StatSingleMemoryAccess() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".stat.single.memory.access.log";
  std::string log_name = "single_access_sink";

  SetupLogger(fname, log_name);
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  logger->info("idx,paddr,avg,med,min,max");

  addr_tuple* tuple = new addr_tuple;
  uint64_t* statistics = new uint64_t[4];
  for (size_t i = 0; i < SUDOKU_TEST_NUM_ITERATION; ++i) {
    GenerateRandomAddressTuple(tuple);
    StatAccessTimingSingleMemoryAccess(reinterpret_cast<uint64_t>(tuple->vaddr),
                                       statistics);
    logger->info("{},{},{},{},{},{}", i,
                 reinterpret_cast<void*>(tuple->paddr - PCI_OFFSET),
                 statistics[0], statistics[1], statistics[2], statistics[3]);
  }

  delete[] statistics;
  delete tuple;
}

void Addressing::StatPairedMemoryAccess() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".stat.paired.memory.access.log";
  std::string log_name = "paired_access_sink";

  SetupLogger(fname, log_name);
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  logger->info("idx,fpaddr,spaddr,avg,med,min,max");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* statistics = new uint64_t[4];
  for (size_t i = 0; i < SUDOKU_TEST_NUM_ITERATION; ++i) {
    GenerateTwoRandomAddressTuples(ftuple, stuple);
    StatAccessTimingPairedMemoryAccess(
        reinterpret_cast<uint64_t>(ftuple->vaddr),
        reinterpret_cast<uint64_t>(stuple->vaddr), statistics);
    logger->info("{},{},{},{},{},{},{}", i,
                 reinterpret_cast<void*>(ftuple->paddr - PCI_OFFSET),
                 reinterpret_cast<void*>(stuple->paddr - PCI_OFFSET),
                 statistics[0], statistics[1], statistics[2], statistics[3]);
  }

  delete[] statistics;
  delete ftuple;
  delete stuple;
}

void Addressing::CheckPairedMemoryAccess(Constraints& constraints) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);
  std::string fname = fname_prefix_ + ".check.paired.memory.access.log";
  std::string log_name = "check_paired_maccess_log";

  SetupLogger(fname, log_name);
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  std::ostringstream oss;
  for (const auto& function : constraints.diff_functions) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("diff_functions,{}", oss.str());
  oss.str("");
  for (const auto& function : constraints.same_functions) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("same_functions,{}", oss.str());

  logger->info("idx,fpaddr,spaddr,avg,med,min,max");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* statistics = new uint64_t[4];
  for (size_t i = 0; i < SUDOKU_TEST_NUM_ITERATION; ++i) {
    GenerateRandomAddressTuple(ftuple);
    while (!GenerateRandomAddressTupleWithConstraints(
        ftuple, stuple, constraints.same_functions,
        constraints.diff_functions)) {
      // retry until found
    }
    StatAccessTimingPairedMemoryAccess(
        reinterpret_cast<uint64_t>(ftuple->vaddr),
        reinterpret_cast<uint64_t>(stuple->vaddr), statistics);
    logger->info("{},{},{},{},{},{},{}", i,
                 reinterpret_cast<void*>(ftuple->paddr - PCI_OFFSET),
                 reinterpret_cast<void*>(stuple->paddr - PCI_OFFSET),
                 statistics[0], statistics[1], statistics[2], statistics[3]);
  }

  delete[] statistics;
  delete ftuple;
  delete stuple;
}

void Addressing::WatchRefreshSingleAccess() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".watch.refresh.single.log";
  std::string log_name = "watch_refresh_single_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  logger->info("idx,start,latency");

  addr_tuple* tuple = new addr_tuple;
  GenerateRandomAddressTuple(tuple);
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 2);
  MeasureRefreshSingleAccess(reinterpret_cast<uint64_t>(tuple->vaddr),
                             histogram);
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    logger->info("{},{},{}", i, histogram[i][0] - histogram[0][0],
                 histogram[i][1]);
  }
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);

  delete tuple;
}

void Addressing::StatRefIntervalSingleAccess(uint64_t threshold) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname =
      fname_prefix_ + ".stat.refresh.interval.single.access.log";
  std::string log_name = "stat_refresh_interval_single_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  logger->info("idx,paddr,avg,med,min,max");

  addr_tuple* tuple = new addr_tuple;
  uint64_t* statistics = new uint64_t[4];
  for (size_t i = 0; i < SUDOKU_TEST_NUM_ITERATION; ++i) {
    GenerateRandomAddressTuple(tuple);
    StatRefreshIntervalSingleAccess(reinterpret_cast<uint64_t>(tuple->vaddr),
                                    threshold, statistics);
    logger->info("{},{},{},{},{},{}", i,
                 reinterpret_cast<void*>(tuple->paddr - PCI_OFFSET),
                 statistics[0], statistics[1], statistics[2], statistics[3]);
  }

  delete[] statistics;
  delete tuple;
}

void Addressing::WatchRefreshPairedAccessCoarse() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".watch.refresh.coarse.log";
  std::string log_name = "watch_coarse_refresh_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  logger->info("idx,start,latency");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  GenerateTwoRandomAddressTuples(ftuple, stuple);
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 3);
  MeasureRefreshPairedAccessCoarse(reinterpret_cast<uint64_t>(ftuple->vaddr),
                                   reinterpret_cast<uint64_t>(stuple->vaddr),
                                   histogram);
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    logger->info("{},{},{}", i, histogram[i][0] - histogram[0][0],
                 histogram[i][1]);
  }
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);

  delete ftuple;
  delete stuple;
}

void Addressing::StatRefIntervalPairedAccessCoarse(uint64_t threshold) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".stat.refresh.coarse.log";
  std::string log_name = "stat_refresh_coarse_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  logger->info("idx,fpaddr,spaddr,avg,med,min,max");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* statistics = new uint64_t[4];
  for (size_t i = 0; i < SUDOKU_TEST_NUM_ITERATION; ++i) {
    GenerateTwoRandomAddressTuples(ftuple, stuple);
    StatRefreshIntervalPairedAccessCoarse(
        reinterpret_cast<uint64_t>(ftuple->vaddr),
        reinterpret_cast<uint64_t>(stuple->vaddr), threshold, statistics);
    logger->info("{},{},{},{},{},{},{}", i,
                 reinterpret_cast<void*>(ftuple->paddr - PCI_OFFSET),
                 reinterpret_cast<void*>(stuple->paddr - PCI_OFFSET),
                 statistics[0], statistics[1], statistics[2], statistics[3]);
  }

  delete[] statistics;
  delete ftuple;
  delete stuple;
}

void Addressing::CheckRefIntervalPairedAccessCoarse(Constraints& constraints,
                                                    uint64_t threshold) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".check.refresh.coarse.log";
  std::string log_name = "check_refresh_coarse_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  std::ostringstream oss;
  for (const auto& function : constraints.diff_functions) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("diff_functions,{}", oss.str());
  oss.str("");
  for (const auto& function : constraints.same_functions) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("same_functions,{}", oss.str());
  logger->info("idx,fpaddr,spaddr,avg,med,min,max");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* statistics = new uint64_t[4];
  for (size_t i = 0; i < SUDOKU_TEST_NUM_ITERATION; ++i) {
    GenerateRandomAddressTuple(ftuple);
    while (!GenerateRandomAddressTupleWithConstraints(
        ftuple, stuple, constraints.same_functions,
        constraints.diff_functions)) {
      // retry until found
    }
    StatRefreshIntervalPairedAccessCoarse(
        reinterpret_cast<uint64_t>(ftuple->vaddr),
        reinterpret_cast<uint64_t>(stuple->vaddr), threshold, statistics);
    logger->info("{},{},{},{},{},{},{}", i,
                 reinterpret_cast<void*>(ftuple->paddr - PCI_OFFSET),
                 reinterpret_cast<void*>(stuple->paddr - PCI_OFFSET),
                 statistics[0], statistics[1], statistics[2], statistics[3]);
  }

  delete ftuple;
  delete stuple;
}

void Addressing::WatchRefreshPairedAccessFine() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".watch.refresh.fine.log";
  std::string log_name = "watch_fine_refresh_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  logger->info("idx,start,first_latency,second_latency");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  GenerateTwoRandomAddressTuples(ftuple, stuple);
  uint64_t** histogram = AllocateHistogram(SUDOKU_REFRESH_NUM_ITERATION, 3);
  MeasureRefreshPairedAccessFine(reinterpret_cast<uint64_t>(ftuple->vaddr),
                                 reinterpret_cast<uint64_t>(stuple->vaddr),
                                 histogram);
  for (size_t i = 0; i < SUDOKU_REFRESH_NUM_ITERATION; ++i) {
    logger->info("{},{},{},{}", i, histogram[i][0] - histogram[0][0],
                 histogram[i][1], histogram[i][2]);
  }
  FreeHistogram(histogram, SUDOKU_REFRESH_NUM_ITERATION);

  delete ftuple;
  delete stuple;
}

void Addressing::StatRefIntervalPairedAccessFine(uint64_t threshold) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".stat.refresh.fine.log";
  std::string log_name = "stat_refresh_fine_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  logger->info("idx,fpaddr,spaddr,avg,med,min,max");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* statistics = new uint64_t[4];
  for (size_t i = 0; i < SUDOKU_TEST_NUM_ITERATION; ++i) {
    GenerateTwoRandomAddressTuples(ftuple, stuple);
    StatRefreshIntervalPairedAccessFine(
        reinterpret_cast<uint64_t>(ftuple->vaddr),
        reinterpret_cast<uint64_t>(stuple->vaddr), threshold, statistics);
    logger->info("{},{},{},{},{},{},{}", i,
                 reinterpret_cast<void*>(ftuple->paddr - PCI_OFFSET),
                 reinterpret_cast<void*>(stuple->paddr - PCI_OFFSET),
                 statistics[0], statistics[1], statistics[2], statistics[3]);
  }

  delete[] statistics;
  delete ftuple;
  delete stuple;
}

void Addressing::CheckRefIntervalPairedAccessFine(Constraints& constraints,
                                                  uint64_t threshold) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".check.refresh.fine.log";
  std::string log_name = "check_refresh_fine_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  std::ostringstream oss;
  for (const auto& function : constraints.diff_functions) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("diff_functions,{}", oss.str());
  oss.str("");
  for (const auto& function : constraints.same_functions) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("same_functions,{}", oss.str());
  logger->info("idx,fpaddr,spaddr,avg,med,min,max");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* statistics = new uint64_t[4];
  for (size_t i = 0; i < SUDOKU_TEST_NUM_ITERATION; ++i) {
    GenerateRandomAddressTuple(ftuple);
    while (!GenerateRandomAddressTupleWithConstraints(
        ftuple, stuple, constraints.same_functions,
        constraints.diff_functions)) {
      // retry until found
    }
    StatRefreshIntervalPairedAccessFine(
        reinterpret_cast<uint64_t>(ftuple->vaddr),
        reinterpret_cast<uint64_t>(stuple->vaddr), threshold, statistics);
    logger->info("{},{},{},{},{},{},{}", i,
                 reinterpret_cast<void*>(ftuple->paddr - PCI_OFFSET),
                 reinterpret_cast<void*>(stuple->paddr - PCI_OFFSET),
                 statistics[0], statistics[1], statistics[2], statistics[3]);
  }

  delete ftuple;
  delete stuple;
}

void Addressing::StatReadReadAccess(uint64_t length) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".stat.rdrd.log";
  std::string log_name = "stat_rdrd_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  logger->info("idx,fpaddr,spaddr,avg,med,min,max");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* faddrs = new uint64_t[length];
  uint64_t* saddrs = new uint64_t[length];
  uint64_t* statistics = new uint64_t[4];

  uint64_t trials = 0;
  while (trials < SUDOKU_NUM_EFFECTIVE_TRIAL) {
    GenerateTwoRandomAddressTuples(ftuple, stuple);
    std::vector<uint64_t> offsets = GenerateRowBufferHitSequences(length);
    for (size_t j = 0; j < length; ++j) {
      faddrs[j] = PhysToVirt(
          pool_, (((reinterpret_cast<uint64_t>(ftuple->paddr) - PCI_OFFSET) ^
                   offsets[j]) +
                  PCI_OFFSET));
    }
    offsets = GenerateRowBufferHitSequences(length);
    for (size_t j = 0; j < length; ++j) {
      saddrs[j] = PhysToVirt(
          pool_, (((reinterpret_cast<uint64_t>(stuple->paddr) - PCI_OFFSET) ^
                   offsets[j]) +
                  PCI_OFFSET));
    }

    bool pass = true;
    for (size_t j = 0; j < length; ++j) {
      if (faddrs[j] == 0 || saddrs[j] == 0) {
        pass = false;
        break;
      }
    }

    if (pass) {
      trials++;
      StatReadReadLatency(faddrs, saddrs, length, statistics);
      logger->info("{},{},{},{},{},{},{}", trials,
                   reinterpret_cast<void*>(ftuple->paddr - PCI_OFFSET),
                   reinterpret_cast<void*>(stuple->paddr - PCI_OFFSET),
                   statistics[0], statistics[1], statistics[2], statistics[3]);
    }
  }

  delete ftuple;
  delete stuple;
  delete[] faddrs;
  delete[] saddrs;
  delete[] statistics;
}

void Addressing::CheckReadReadAccess(Constraints& constraints,
                                     uint64_t length) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".stat.rdrd.log";
  std::string log_name = "stat_rdrd_sink";
  SetupLogger(fname, log_name);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  std::ostringstream oss;
  for (const auto& function : constraints.diff_functions) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("diff_functions,{}", oss.str());
  oss.str("");
  for (const auto& function : constraints.same_functions) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("same_functions,{}", oss.str());
  logger->info("idx,fpaddr,spaddr,avg,med,min,max");

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* faddrs = new uint64_t[length];
  uint64_t* saddrs = new uint64_t[length];
  uint64_t* statistics = new uint64_t[4];

  uint64_t trials = 0;
  while (trials < SUDOKU_NUM_EFFECTIVE_TRIAL) {
    GenerateRandomAddressTuple(ftuple);
    while (!GenerateRandomAddressTupleWithConstraints(
        ftuple, stuple, constraints.same_functions,
        constraints.diff_functions)) {
      // retry until found
    }
    std::vector<uint64_t> offsets = GenerateRowBufferHitSequences(length);
    for (size_t j = 0; j < length; ++j) {
      faddrs[j] = PhysToVirt(
          pool_, (((reinterpret_cast<uint64_t>(ftuple->paddr) - PCI_OFFSET) ^
                   offsets[j]) +
                  PCI_OFFSET));
    }
    offsets = GenerateRowBufferHitSequences(length);
    for (size_t j = 0; j < length; ++j) {
      saddrs[j] = PhysToVirt(
          pool_, (((reinterpret_cast<uint64_t>(stuple->paddr) - PCI_OFFSET) ^
                   offsets[j]) +
                  PCI_OFFSET));
    }

    bool pass = true;
    for (size_t j = 0; j < length; ++j) {
      if (faddrs[j] == 0 || saddrs[j] == 0) {
        pass = false;
        break;
      }
    }

    if (pass) {
      trials++;
      StatReadReadLatency(faddrs, saddrs, length, statistics);
      logger->info("{},{},{},{},{},{},{}", trials,
                   reinterpret_cast<void*>(ftuple->paddr - PCI_OFFSET),
                   reinterpret_cast<void*>(stuple->paddr - PCI_OFFSET),
                   statistics[0], statistics[1], statistics[2], statistics[3]);
    }
  }

  delete ftuple;
  delete stuple;
  delete[] faddrs;
  delete[] saddrs;
  delete[] statistics;
}

void Addressing::ReverseAddressingFunctions() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);
  std::string fname = fname_prefix_ + ".reverse.addressing.log";
  std::string log_name = "reverse_addressing_log";
  SetupLogger(fname, log_name);
  uint64_t num_functions_to_find = GetNumFunctions();
  // Collect same bank, different row address pairs exploiting row buffer
  // conflicts
  CollectSameBankPairs(log_name);
  FilterSameBankPairs(log_name);

  // "ZenHammer: Rowhammer Attacks on AMD Zen-based Platforms," SEC, 2024
  if (PCI_OFFSET) {
    // FilterSameBankPairs(log_name);  // for AMD processors (additional step)
    SlideOffsets(PCI_OFFSET);
  }

  // Derive addressing functions (brute-forcing method) and optimize functions
  // using Gaussian elimination.
  addressing_functions_ = DeriveFunctions(sbdr_pairs_, log_name);

  if (debug_) {
    // stores raw data (same bank, different row pairs)
    std::string debug_fname = fname_prefix_ + ".drama.raw.csv";
    std::string debug_name = "drama_raw_output";
    SetupLogger(debug_fname, debug_name);

    auto dbg_logger = spdlog::get(debug_name);
    dbg_logger->set_pattern("%v");

    std::ostringstream oss;
    for (const auto& set : sbdr_pairs_) {
      for (const auto& addr : set) {
        oss << reinterpret_cast<void*>(addr.paddr) << ",";
      }
      oss << "\n";
    }

    dbg_logger->info("{}", oss.str());
  }

  // Checking the derived functions
  if (addressing_functions_.size() != num_functions_to_find) {
    auto logger = spdlog::get(log_name);
    logger->error("{}Deriving DRAM addressing functions failed. Please retry.",
                  color_red);
    logger->error(
        "  Found number of functions: {}, Expected number of functions: {}{}",
        addressing_functions_.size(), num_functions_to_find, color_reset);
  } else {
    auto logger = spdlog::get(log_name);
    logger->set_pattern("%v");
    logger->info("{}Found functions:", color_green);
    for (auto& function : addressing_functions_) {
      logger->info("  {}{}{}", color_green, reinterpret_cast<void*>(function),
                   color_reset);
    }
  }
  // shutdown
  spdlog::shutdown();
}

void Addressing::IdentifyBits(std::vector<uint64_t> functions) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);
  std::string fname = fname_prefix_ + ".identify.bits.log";
  std::string log_name = "identify_bits_log";
  SetupLogger(fname, log_name);
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  // Classify physical address bits into covered and uncovered bits.
  uint64_t covered_bit_mask = 0, uncovered_bit_mask = 0;
  for (const auto& function : functions) {
    covered_bit_mask |= function;
  }
  uncovered_bit_mask = (((1ULL << max_bits_) - 1) ^ covered_bit_mask) ^
                       ((1ULL << CACHELINE_OFFSET) - 1);
  for (size_t i = CACHELINE_OFFSET; i < max_bits_; ++i) {
    if (covered_bit_mask & (1ULL << i)) {
      covered_bits_.push_back(i);
    } else {
      uncovered_bits_.push_back(i);
    }
  }
  // Merge and update sets (if there are duplicated bits in two different
  //  functions/sets, merge those two functions/sets into one set. Then, repeat
  //  until there are no duplicates btw two diff sets/functions.)
  std::vector<uint64_t> disjoint_sets = MergeFunctionsToDisjointSets(functions);
  // Check unused physical address bits
  CheckUnusedBits(uncovered_bit_mask, log_name);
  // Check used physical address bits
  CheckUsedBits(disjoint_sets, log_name);

  // Optimize (reduce) row_functions_ and column_functions_ using Gaussian
  //  elimination and upate row/bit masks.
  row_functions_ = ReduceFunctions(row_functions_);
  column_functions_ = ReduceFunctions(column_functions_);
  // Extract row bits from row_functions (MSB bit in each function)
  std::vector<uint64_t> msbs, lsbs;
  for (const auto& row : row_functions_) {
    for (uint64_t i = (max_bits_ - 1); i >= CACHELINE_OFFSET; --i) {
      if ((1ULL << i) & row) {
        msbs.push_back(1ULL << i);
        break;
      }
    }
  }
  row_functions_.clear();
  row_functions_.insert(row_functions_.end(), msbs.begin(), msbs.end());
  std::sort(row_functions_.begin(), row_functions_.end());
  row_functions_.erase(
      std::unique(row_functions_.begin(), row_functions_.end()),
      row_functions_.end());

  uint64_t mask = 0;
  for (const auto& bit : row_functions_) {
    mask |= bit;
  }
  row_bits_ = mask;
  // Extract column bits from column_functions (LSB bit in each function)
  mask = 0;
  for (const auto& column : column_functions_) {
    for (uint64_t i = CACHELINE_OFFSET; i < max_bits_; ++i) {
      if ((1ULL << i) & column) {
        lsbs.push_back(1ULL << i);
        break;
      }
    }
  }
  column_functions_.clear();
  column_functions_.insert(column_functions_.end(), lsbs.begin(), lsbs.end());
  std::sort(column_functions_.begin(), column_functions_.end());
  column_functions_.erase(
      std::unique(column_functions_.begin(), column_functions_.end()),
      column_functions_.end());
  for (const auto& bit : column_functions_) {
    mask |= bit;
  }
  column_bits_ = mask;
  // There might be unidentified bits in this step. These unidentified bits
  //  are covered in the ValidateAddressMapping() function.
  logger->info("{}Found bits: ", color_green);
  logger->info("  row_bits,{}", reinterpret_cast<void*>(row_bits_));
  logger->info("  column_bits,{}{}", reinterpret_cast<void*>(column_bits_),
               color_reset);
}

bool Addressing::ValidateAddressMapping() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);
  bool pass = false;
  std::string fname = fname_prefix_ + ".validate.address.mapping.log";
  std::string log_name = "validate_address_mapping_log";
  SetupLogger(fname, log_name);
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");
  // Generate disjoint sets from the given functions
  std::vector<uint64_t> disjoint_sets =
      MergeFunctionsToDisjointSets(addressing_functions_);
  std::vector<uint64_t> incomplete_sets;
  // Check mapping system's injectivity
  pass = CheckInjectivity(disjoint_sets, incomplete_sets);
  if (!pass) {
    logger->info("[-] There are incomplete disjoint sets.");
    std::ostringstream oss;
    for (auto& function : incomplete_sets) {
      oss << reinterpret_cast<void*>(function) << ",";
    }
    logger->error("{}", oss.str());

    // Extra step for resolving unidentified bits for row or column bits.
    ResolveAddressMapping(incomplete_sets, log_name);
    pass = CheckInjectivity(disjoint_sets, incomplete_sets);
    if (pass) {
      logger->info(
          "[+] Modified DRAM address mapping system is now injective.");
    } else {
      logger->info(
          "{}[-] Cannot resolve the input functions, row and column bits. "
          "Please retry or refind the functions and bits.{}",
          color_red, color_reset);
    }
  } else {
    logger->info(
        "[+] input addressing functions, row bits, and column bits satisfies "
        "the injectivity.");
  }

  if (pass) {
    std::ostringstream oss;
    for (auto& function : addressing_functions_) {
      oss << reinterpret_cast<void*>(function) << ",";
    }
    logger->info("{}Validated DRAM address mapping:", color_green);
    logger->info("  functions:{}", oss.str());
    logger->info("  row_bits:{}", reinterpret_cast<void*>(row_bits_));
    logger->info("  column_bits:{}{}", reinterpret_cast<void*>(column_bits_),
                 color_reset);
  }
  return pass;
}

void Addressing::DecomposeUsingRefreshes() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".decompose.refresh.log";
  std::string log_name = "decompose_refresh_sink";

  SetupLogger(fname, log_name);
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  std::function<uint64_t(uint64_t, uint64_t, uint64_t)> refresh_oracle;
  if (type_ == "DDR4" || type_ == "ddr4") {
    refresh_oracle = AverageRefreshIntervalPairedAccessFine;
  } else if (type_ == "DDR5" || type_ == "ddr5") {
    refresh_oracle = AverageRefreshIntervalPairedAccessCoarse;
  } else {
    // unsupported (lpddr5/x)
    refresh_oracle = AverageRefreshIntervalPairedAccessCoarse;
  }

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  for (const auto& function : addressing_functions_) {
    logger->info("[+] Check refresh intervals of function {}",
                 reinterpret_cast<void*>(function));
    std::vector<uint64_t> other_functions(addressing_functions_);
    other_functions.erase(
        std::remove(other_functions.begin(), other_functions.end(), function),
        other_functions.end());
    std::vector<uint64_t> diff_functions = {function};

    // Checking for each function
    uint64_t trials = 0, normal_interval_score = 0, reduced_interval_score = 0;
    while (trials++ < SUDOKU_NUM_EFFECTIVE_TRIAL) {
      GenerateRandomAddressTuple(ftuple);
      while (!GenerateRandomAddressTupleWithConstraints(
          ftuple, stuple, other_functions, diff_functions)) {
        // retry until found
      }

      // derive refresh intervals
      uint64_t interval = refresh_oracle(
          reinterpret_cast<uint64_t>(ftuple->vaddr),
          reinterpret_cast<uint64_t>(stuple->vaddr), REFRESH_CYCLE_LOWER_BOUND);

      // For Intel processors, reduced refresh intervals are observed in DIMM
      // and rank functions. In contrast, for AMD processors, normal refresh
      // intervals are observed in channel and sub-channel functions.
#if defined(COMPILE_ZEN_4)
      if (interval < REGULAR_REFRESH_INTERVAL_THRESHOLD) {
        ++reduced_interval_score;
      } else {
        ++normal_interval_score;
      }
#else
      if (interval < REGULAR_REFRESH_INTERVAL_THRESHOLD && interval > 1000) {
        ++reduced_interval_score;
      } else {
        ++normal_interval_score;
      }
#endif
    }
    logger->info("Functions: {}, tREFI: {}, tREFI/2: {}",
                 reinterpret_cast<void*>(function), normal_interval_score,
                 reduced_interval_score);

    // need to update!
    if (reduced_interval_score > SUDOKU_TRIAL_SUCCESS_SCORE) {
      // AMD: reduced_refresh_intervals: sub-channel, DIMM, and rank
      // Intel with DDR4: reduced_refresh_intervals: channel, DIMM, and rank
      // Intel with DDR5: reduced_refresh_intervals: channel, sub-channel, 
      //    and bank address (a single)
      rank_functions_.push_back(function);
    }
  }
  /*
  std::ostringstream oss;
  for (const auto& function : rank_functions_) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
#if defined(COMPILE_ZEN_4)
  logger->info("{}[+] Insert to rank, dimm, and sub-channel functions: {}{}",
	       color_green, oss.str(), color_reset);
#elif defined(COMPILE_ALDER_LAKE_DDR4)
  logger->info("{}[+] Insert to rank functions: {}{}", color_green, oss.str(),
               color_reset);
#else
  logger->info("{}[+] Insert to channel, sub-channel, and bank address functions: {}{}",
          color_green, oss.str(), color_reset);
#endif
  */
  delete ftuple;
  delete stuple;
}

void Addressing::DecomposeUsingConsecutiveAccesses() {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  std::string fname = fname_prefix_ + ".decompose.rdrd.log";
  std::string log_name = "decompose_rdrd_sink";

  SetupLogger(fname, log_name);
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  std::function<uint64_t(uint64_t*, uint64_t*, size_t)>
      consecutive_access_oracle = AverageReadReadLatency;

  std::vector<std::pair<uint64_t, uint64_t>> rdrd_latencies;

  addr_tuple* ftuple = new addr_tuple;
  addr_tuple* stuple = new addr_tuple;
  uint64_t* faddrs = new uint64_t[CONSECUTIVE_LENGTH];
  uint64_t* saddrs = new uint64_t[CONSECUTIVE_LENGTH];
  for (const auto& function : addressing_functions_) {
    logger->info("[+] Check consecutive memory accesses of function {}",
                 reinterpret_cast<void*>(function));
    std::vector<uint64_t> other_functions(addressing_functions_);
    other_functions.erase(
        std::remove(other_functions.begin(), other_functions.end(), function),
        other_functions.end());
    std::vector<uint64_t> diff_functions = {function};

    // Checking for each function
    uint64_t trials = 0, average_rdrd_latency = 0;
    while (trials < SUDOKU_NUM_EFFECTIVE_TRIAL) {
      // Generate two base addresses satisfying the constraints
      GenerateRandomAddressTuple(ftuple);
      while (!GenerateRandomAddressTupleWithConstraints(
          ftuple, stuple, other_functions, diff_functions)) {
        // retry until found
      }

      // We use different offsets for each read stream
      std::vector<uint64_t> offsets =
          GenerateRowBufferHitSequences(CONSECUTIVE_LENGTH);
      for (size_t i = 0; i < CONSECUTIVE_LENGTH; ++i) {
        faddrs[i] = PhysToVirt(
            pool_, (((reinterpret_cast<uint64_t>(ftuple->paddr) - PCI_OFFSET) ^
                     offsets[i]) +
                    PCI_OFFSET));
      }
      offsets = GenerateRowBufferHitSequences(CONSECUTIVE_LENGTH);
      for (size_t i = 0; i < CONSECUTIVE_LENGTH; ++i) {
        saddrs[i] = PhysToVirt(
            pool_, (((reinterpret_cast<uint64_t>(stuple->paddr) - PCI_OFFSET) ^
                     offsets[i]) +
                    PCI_OFFSET));
      }

      bool pass = true;
      for (size_t i = 0; i < CONSECUTIVE_LENGTH; ++i) {
        if (faddrs[i] == 0 || saddrs[i] == 0) {
          pass = false;
          break;
        }
      }

      if (pass) {
        trials++;
        uint64_t rdrd_latency =
            consecutive_access_oracle(faddrs, saddrs, CONSECUTIVE_LENGTH);
        average_rdrd_latency += rdrd_latency;
      }
    }
    average_rdrd_latency /= trials;
    logger->info("Functions: {}, Avg RDRD latency: {}",
                 reinterpret_cast<void*>(function), average_rdrd_latency);
    rdrd_latencies.push_back({function, average_rdrd_latency});
  }

  // sort
  std::sort(rdrd_latencies.begin(), rdrd_latencies.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
  /*
  std::ostringstream oss;
  // First N high rdrd latency functions are rank/dimm and bank address
  // functions.
  uint64_t num_high_rdrd_latency = 2; // bank address
  uint64_t num_functions = rdrd_latencies.size();
  for (size_t i = 0; i < num_functions; ++i) {
    uint64_t f = rdrd_latencies[num_functions - i - 1].first;
    if (std::find(rank_functions_.begin(), rank_functions_.end(), f) ==
        rank_functions_.end()) {
      bank_address_functions_.push_back(f);
      if (bank_address_functions_.size() == num_high_rdrd_latency) {
	break;
      }
    }
  }

  for (const auto& function : bank_address_functions_) {
    oss << reinterpret_cast<void*>(function) << ",";
  }
  logger->info("{}[+] Insert to bank address functions: {}{}", color_green,
               oss.str(), color_reset);
  */

  delete ftuple;
  delete stuple;
  delete[] faddrs;
  delete[] saddrs;
}

/* DRAMA implementation */
// "DRAMA: Exploiting DRAM Addressing for Cross-CPU Attacks," SEC, 2016
bool Addressing::EnoughSameBankPairs() {
  uint64_t num_banks, count = 0;
  // empirical values
  num_banks = GetNumBanks(memory_config_) / 2;
  for (uint64_t i = 0; i < sbdr_pairs_.size(); ++i) {
    if (sbdr_pairs_[i].size() >= DRAMA_MINIMUM_SET_SIZE) {
      count++;
    }
  }
  return count == num_banks;
}

// "DRAMA: Exploiting DRAM Addressing for Cross-CPU Attacks," SEC, 2016
void Addressing::CollectSameBankPairs(std::string log_name) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);
  std::vector<uint64_t> used_addresses;
  addr_tuple* generated = new addr_tuple;

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  logger->info("[+] Collect Same Bank, Different Row Pairs");
  while (!EnoughSameBankPairs()) {
    GenerateRandomAddressTuple(generated);
    if (std::find(used_addresses.begin(), used_addresses.end(),
                  reinterpret_cast<uint64_t>(generated->vaddr)) !=
        used_addresses.end()) {
      continue;
    }

    bool found = false;
    used_addresses.push_back(reinterpret_cast<uint64_t>(generated->vaddr));
    for (uint64_t i = 0; i < sbdr_pairs_.size(); ++i) {
      addr_tuple base = sbdr_pairs_[i][0];
      uint64_t latency = AverageAccessTimingPairedMemoryAccess(
          reinterpret_cast<uint64_t>(base.vaddr),
          reinterpret_cast<uint64_t>(generated->vaddr));
      if (latency > SBDR_LOWER_BOUND && latency < SBDR_UPPER_BOUND) {
        logger->info("Insert address {} to set {} with latency {} cycles.",
                     reinterpret_cast<void*>(generated->paddr - PCI_OFFSET), i,
                     latency);
        sbdr_pairs_[i].push_back(*generated);
        found = true;
        break;
      }
    }

    if (!found) {
      sbdr_pairs_.push_back({*generated});
    }
  }

  // filter
  for (auto s = sbdr_pairs_.begin(); s != sbdr_pairs_.end(); ++s) {
    if (s->size() < DRAMA_MINIMUM_SET_SIZE) {
      sbdr_pairs_.erase(s);
      s--;
    }
  }

  delete generated;
}
/* end of DRAMA */

std::vector<uint64_t> Addressing::MergeFunctionsToDisjointSets(
    std::vector<uint64_t> functions) {
  std::vector<uint64_t> merged_functions = std::move(functions);
  bool changed;

  do {
    changed = false;
    std::vector<bool> merged(merged_functions.size(), false);
    std::vector<uint64_t> curr_merged_functions;

    for (size_t i = 0; i < merged_functions.size(); ++i) {
      if (merged[i]) {
        continue;
      }
      uint64_t merged_value = merged_functions[i];
      merged[i] = true;

      for (size_t j = 0; j < merged_functions.size(); ++j) {
        if (merged[j]) {
          continue;
        }
        if (merged_value & merged_functions[j]) {
          merged_value |= merged_functions[j];
          merged[j] = true;
          changed = true;
        }
      }
      curr_merged_functions.push_back(merged_value);
    }
    merged_functions = std::move(curr_merged_functions);
  } while (changed);

  return merged_functions;
}

std::vector<uint64_t> Addressing::DeriveFunctions(
    std::vector<std::vector<addr_tuple>> sets, std::string log_name) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  logger->info("[+] Derive Functions from Sets");
  std::vector<uint64_t> functions;

  // below code came from TRRespass' GitHub
  // https://github.com/vusec/trrespass/blob/master/drama/src/rev-mc.c
  // (find_functions)
  for (uint64_t b = FUNCTION_MIN_NUM_BITS; b <= FUNCTION_MAX_NUM_BITS; ++b) {
    uint64_t function_mask = ((1 << b) - 1);
    uint64_t last_function_mask = (function_mask << (max_bits_ - b));
    function_mask <<= CACHELINE_OFFSET;

    while (function_mask != last_function_mask) {  // iterate over all mask
      if (((1ULL << CACHELINE_OFFSET) - 1) & function_mask) {
        function_mask = NextBitPermutation(function_mask);
        continue;
      }

      for (uint64_t i = 0; i < sets.size(); ++i) {
        std::vector<addr_tuple> curr = sets[i];
        uint64_t base_value = __builtin_parityl(curr[0].paddr & function_mask);
        for (uint64_t j = 1; j < curr.size(); ++j) {
          uint64_t value = __builtin_parityl(curr[j].paddr & function_mask);
          if (value != base_value) {
            goto next_mask;
          }
        }
      }

      logger->info("Insert function {} to possible functions",
                   reinterpret_cast<void*>(function_mask));
      functions.push_back(function_mask);

    next_mask:
      function_mask = NextBitPermutation(function_mask);
    }
  }
  functions = ReduceFunctions(functions);

  return functions;
}

void Addressing::CheckUnusedBits(uint64_t bitmask, std::string log_name) {
  // Check uncovered bits
  // unused bits always generates the same rank and same bank xor mask (from
  // derived addressing functions) therefore, we generate and use all possible
  // combinations.
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  logger->info("Check unused bits");
  addr_tuple* base = new addr_tuple;

  // Exhaustive testing to verify all possible bitmasks
  std::vector<uint64_t> masks = GenerateAllCombinations(bitmask);
  for (const auto& mask : masks) {
    uint64_t row_bit_score = 0, column_bit_score = 0, trials = 0,
             effective_trials = 0;
    while (trials++ < SUDOKU_MAX_NUM_TRIALS) {
      uint64_t vaddr = 0, paddr = 0, latency = 0;
      GenerateRandomAddressTuple(base);
      paddr = ((base->paddr - PCI_OFFSET) ^ mask) + PCI_OFFSET;
      vaddr = PhysToVirt(pool_, paddr);

      if (vaddr != 0) {
        effective_trials++;
        latency = AverageAccessTimingPairedMemoryAccess(
            reinterpret_cast<uint64_t>(base->vaddr), vaddr);

        if ((latency > SBDR_LOWER_BOUND) && (latency < SBDR_UPPER_BOUND)) {
          row_bit_score++;
        } else {
          column_bit_score++;
        }

        if (effective_trials >= SUDOKU_NUM_EFFECTIVE_TRIAL) {
          break;
        }
      }
    }

    if (trials >= SUDOKU_MAX_NUM_TRIALS) {
      logger->info("[ failed to identify ] {} exceeds the maximum attempts!",
                   reinterpret_cast<void*>(mask));
    } else if (row_bit_score > SUDOKU_TRIAL_SUCCESS_SCORE) {
      logger->info("[ inserted to row function ] {} with score {} / {}",
                   reinterpret_cast<void*>(mask), row_bit_score,
                   effective_trials);
      row_functions_.push_back(mask);
    } else if (column_bit_score > SUDOKU_TRIAL_SUCCESS_SCORE) {
      logger->info("[ inserted to column function ] {} with score {} / {}",
                   reinterpret_cast<void*>(mask), column_bit_score,
                   effective_trials);
      column_functions_.push_back(mask);
    } else {
      logger->info("[ outlier ] {} with score ({} + {}) / {}",
                   reinterpret_cast<void*>(mask), row_bit_score,
                   column_bit_score, trials);
    }
  }
  delete base;
}

void Addressing::CheckUsedBits(std::vector<uint64_t> disjoint_sets,
                               std::string log_name) {
  // Check covered bits
  // Considering the derived addressing functions, we change the even number
  // of bits in a specific addressing functions to generate same hash output.
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  logger->info("Check used bits");
  addr_tuple* base = new addr_tuple;
  for (const auto& set : disjoint_sets) {
    logger->info("[ Check ] set: {}", reinterpret_cast<void*>(set));
    std::vector<uint64_t> involved_functions;
    for (const auto& function : addressing_functions_) {
      if (function & set) {
        involved_functions.push_back(function);
      }
    }

    // Exhaustive testing to verify all possible bitmasks
    std::vector<uint64_t> masks = GenerateAllCombinations(set);
    for (const auto& mask : masks) {
      uint64_t row_bit_score = 0, column_bit_score = 0, trials = 0,
               effective_trials = 0;
      // for the even number of bits in mask to generate the same hash value.
      if (__builtin_popcountll(mask) >= 4) continue;
      if (XORReductionWithMasks(involved_functions, mask) != 0) continue;
      while (trials++ < SUDOKU_MAX_NUM_TRIALS) {
        uint64_t vaddr = 0, paddr = 0, latency = 0;
        GenerateRandomAddressTuple(base);
        paddr = ((base->paddr - PCI_OFFSET) ^ mask) + PCI_OFFSET;
        vaddr = PhysToVirt(pool_, paddr);

        if (vaddr != 0) {
          effective_trials++;
          latency = AverageAccessTimingPairedMemoryAccess(
              reinterpret_cast<uint64_t>(base->vaddr), vaddr);

          if ((latency > SBDR_LOWER_BOUND) && (latency < SBDR_UPPER_BOUND)) {
            row_bit_score++;
          } else {
            column_bit_score++;
          }

          if (effective_trials >= SUDOKU_NUM_EFFECTIVE_TRIAL) {
            break;
          }
        }
      }

      if (trials > SUDOKU_MAX_NUM_TRIALS) {
        logger->info(
            "[ failed to identify ] {} exceeds the maximum attempts! {} / {}",
            reinterpret_cast<void*>(mask), (row_bit_score + column_bit_score),
            trials);
      } else if (row_bit_score > SUDOKU_TRIAL_SUCCESS_SCORE) {
        logger->info("[ inserted to row function ] {} with score {} / {}",
                     reinterpret_cast<void*>(mask), row_bit_score,
                     effective_trials);
        row_functions_.push_back(mask);
      } else if (column_bit_score > SUDOKU_TRIAL_SUCCESS_SCORE) {
        logger->info("[ inserted to column function ] {} with score {} / {}",
                     reinterpret_cast<void*>(mask), column_bit_score,
                     effective_trials);
        column_functions_.push_back(mask);
      } else {
        logger->info("[ outlier ] {} with score ({} + {}) / {}",
                     reinterpret_cast<void*>(mask), row_bit_score,
                     column_bit_score, trials);
      }
    }
  }
  delete base;
}

void Addressing::SlideOffsets(const uint64_t offset) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  for (auto& set : sbdr_pairs_) {
    for (auto& addr : set) {
      addr.paddr -= offset;
    }
  }
}

void Addressing::FilterSameBankPairs(std::string log_name) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);

  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  uint32_t idx = 0;
  logger->info("[+] Filter Same Bank Different Row Pairs");
  for (auto& set : sbdr_pairs_) {
    for (auto it = set.begin(); it != set.end();) {
      uint64_t score = 0;
      for (const auto& other : set) {
        if (it->vaddr == other.vaddr) {
          continue;
        }

        uint64_t latency = AverageAccessTimingPairedMemoryAccess(
            reinterpret_cast<uint64_t>(it->vaddr),
            reinterpret_cast<uint64_t>(other.vaddr));

        if (latency < SBDR_LOWER_BOUND) {
          score++;
        }
      }
      if (score > SUDOKU_FILTER_SCORE) {
        logger->info("Delete address {} from set {} (score: {} / {})",
                     reinterpret_cast<void*>(it->paddr - PCI_OFFSET), idx,
                     score, set.size());
        it = set.erase(it);
      } else {
        ++it;
      }
    }
    idx++;
  }
}

bool Addressing::CheckInjectivity(std::vector<uint64_t> disjoint_sets,
                                  std::vector<uint64_t>& incomplete_sets) {
  bool pass = false;
  incomplete_sets.clear();
  // for each disjoint set, we compare the rank and columns (nullspace).
  for (const auto& set : disjoint_sets) {
    uint64_t num_columns = 0, num_functions = 0;
    num_columns = __builtin_popcountll(set);
    for (const auto& function : addressing_functions_) {
      if (function & set) {
        num_functions++;
      }
    }

    num_functions += __builtin_popcountll(row_bits_ & set);
    num_functions += __builtin_popcountll(column_bits_ & set);
    if (num_functions != num_columns) {
      incomplete_sets.push_back(set);
    }
  }

  pass = (incomplete_sets.size() == 0) ? true : false;
  return pass;
}

void Addressing::ResolveAddressMapping(std::vector<uint64_t> incomplete_sets,
                                       std::string log_name) {
  PRINT_DEBUG_FUNCTION_NAME(debug_);
  auto logger = spdlog::get(log_name);
  logger->set_pattern("%v");

  std::sort(incomplete_sets.begin(), incomplete_sets.end());

  uint64_t num_row_bits_to_find = GetNumRowBits();
  uint64_t num_column_bits_to_find = GetNumColumnBits();
  uint64_t curr_num_row_bits = __builtin_popcountll(row_bits_);
  uint64_t curr_num_column_bits = __builtin_popcountll(column_bits_);

  // find column first
  while (num_column_bits_to_find != curr_num_column_bits) {
    bool added = false;
    for (size_t i = CACHELINE_OFFSET; i < max_bits_; ++i) {
      uint64_t tested_bit = (1ULL << i);

      if (tested_bit & column_bits_) continue;

      for (const auto& set : incomplete_sets) {
        if (tested_bit & set) {
          column_bits_ |= tested_bit;
          curr_num_column_bits++;
          added = true;
          logger->info("Insert bit {} to column_bits", i);
          break;
        }
      }

      if (added) {
        break;
      }
    }

    if (!added) {
      logger->error("Cannot find the appropriate bit for columns!");
      break;
    }
  }

  // find row
  while (num_row_bits_to_find != curr_num_row_bits) {
    bool added = false;
    for (size_t i = (max_bits_ - 1); i >= CACHELINE_OFFSET; --i) {
      uint64_t tested_bit = (1ULL << i);

      if (tested_bit & row_bits_) continue;

      for (const auto& set : incomplete_sets) {
        if (tested_bit & set) {
          row_bits_ |= tested_bit;
          curr_num_row_bits++;
          added = true;
          logger->info("Insert bit {} to row_bits", i);
          break;
        }
      }

      if (added) {
        break;
      }
    }

    if (!added) {
      logger->error("Cannot find the appropriate bit for rows!");
      break;
    }
  }
}

}  // namespace sudoku
