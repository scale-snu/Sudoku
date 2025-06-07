#include <getopt.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <string>

#include "../internal/constants.h"
#include "../sudoku.h"
#include "../sudoku_addressing.h"

using namespace sudoku;

static const char help_msg[] =
    R"([?] Usage: 
    $ sudo numactl -C [core] -m [memory] ./watch_conflicts [OPTIONS]

    Options:
      --output,    -o [STR]     Output filename prefix
      --pages,     -p [INT]     Number of OS memory pages to allocate
      --mode,      -m [STR]     Mode (supported: stat and check)
        stat:                   Get statistics of single and paired memory accesses
        check:                  Check the paired memory access latencies with given constraints

      --type,      -t [STR]     DDR type ([ddr4] or ddr5)
      --num,       -n [INT]     Number of DRAM modules
      --size,      -s [INT]     Size of DRAM module in GB
      --rank,      -r [INT]     Number of ranks per DRAM module
      --width,     -w [INT]     DQ width of DRAM ([8], 16, or 32)
      --same       -S [HEXes]   Constraints: same DRAM mapping functions in hex, separated by commas (for check mode)
      --diff,      -D [HEXes]   Constraints: diff DRAM mapping functions in hex, separated by commas (for check mode)
      --row,       -R [HEX]     DRAM row bits (for check mode)
      --column,    -C [HEX]     DRAM column bits (for check mode)

      --debug,     -d           Enable debug output
      --verbose,   -v           Enable verbose mode
      --log,       -l           Enable logging
      --help,      -h           Show this help message
)";

void PrintHelp(std::string msg) {
  if (!msg.empty()) {
    spdlog::error("{}", msg);
    spdlog::info("Use --help or -h to see usage.");
  } else {
    spdlog::info("{}", help_msg);
  }
}

int main(int argc, char* argv[]) {
  std::string fname_prefix = "default", type = "ddr4", mode = "stat";
  uint64_t num_pages = 19, page_size = 1ULL * 1024ULL * 1024ULL * 1024ULL,
           granularity = (1ULL << CACHELINE_OFFSET), num_dimms = 1,
           module_size = 32ULL * 1024ULL * 1024ULL * 1024ULL, num_ranks = 2,
           dq_width = 8, row_bits = 0, column_bits = 0;
  DDRType ddr_type = DDRType::DDR4;
  std::vector<uint64_t> same_functions = {};
  std::vector<uint64_t> diff_functions = {};
  bool debug = false, verbose = false, logging = false;

  // check sudo privilege
  if (getuid() != 0) {
    spdlog::error("watch_conflicts requires sudo privilege.");
    exit(EXIT_FAILURE);
  }

  // parse argument
  static struct option long_options[] = {{"output", optional_argument, 0, 'o'},
                                         {"pages", optional_argument, 0, 'p'},
                                         {"type", required_argument, 0, 't'},
                                         {"num", required_argument, 0, 'n'},
                                         {"size", required_argument, 0, 's'},
                                         {"rank", required_argument, 0, 'r'},
                                         {"width", required_argument, 0, 'w'},
                                         {"same", required_argument, 0, 'S'},
                                         {"diff", required_argument, 0, 'D'},
                                         {"row", required_argument, 0, 'R'},
                                         {"column", required_argument, 0, 'C'},
                                         {"debug", optional_argument, 0, 'd'},
                                         {"verbose", optional_argument, 0, 'v'},
                                         {"log", optional_argument, 0, 'l'},
                                         {"help", optional_argument, 0, 'h'},
                                         {0, 0, 0, 0}};
  if (argc < 2) {
    PrintHelp("");
  } else {
    int opt, idx;
    while ((opt = getopt_long(argc, argv, "o:p:t:m:n:s:r:w:S:D:R:C:dvlh",
                              long_options, &idx)) != -1) {
      switch (opt) {
        case 'o':
          if (optarg) {
            fname_prefix = std::string(optarg);
          }
          break;
        case 'p':
          num_pages = strtoull(optarg, NULL, 10);
          break;
        case 't': {
          type = std::string(optarg);
          if (type == "DDR4" || type == "ddr4") {
            ddr_type = DDRType::DDR4;
          } else if (type == "DDR5" || type == "ddr5") {
            ddr_type = DDRType::DDR5;
          } else {
            spdlog::error("Unsupported DDR type: {}", type);
            exit(EXIT_FAILURE);
          }
          break;
        }
        case 'm':
          if (optarg) {
            mode = std::string(optarg);
          }
          break;
        case 'n':
          num_dimms = strtoull(optarg, NULL, 10);
          break;
        case 's':
          module_size =
              strtoull(optarg, NULL, 10) * 1024ULL * 1024ULL * 1024ULL;
          break;
        case 'r':
          num_ranks = strtoull(optarg, NULL, 10);
          break;
        case 'w':
          dq_width = strtoull(optarg, NULL, 10);
          break;
        case 'S': {
          std::stringstream ss(optarg);
          std::string token;
          while (std::getline(ss, token, ',')) {
            same_functions.push_back(strtoull(token.c_str(), nullptr, 16));
          }
          break;
        }
        case 'D': {
          std::stringstream ss(optarg);
          std::string token;
          while (std::getline(ss, token, ',')) {
            diff_functions.push_back(strtoull(token.c_str(), nullptr, 16));
          }
          break;
        }
        case 'R':
          row_bits = strtoull(optarg, NULL, 16);
          break;
        case 'C':
          column_bits = strtoull(optarg, NULL, 16);
          break;
        case 'd':
          debug = true;
          break;
        case 'v':
          verbose = true;
          break;
        case 'l':
          logging = true;
          break;
        case 'h':
          PrintHelp("");
          exit(EXIT_SUCCESS);
        default:
          PrintHelp("");
          exit(EXIT_FAILURE);
      }
    }
  }

  AddressingConfig* addressing_config =
      new AddressingConfig(type, fname_prefix, verbose, debug, logging);
  MemoryPoolConfig* memory_pool_config = new MemoryPoolConfig(
      page_size, num_pages, granularity, true /* hugepage */);
  DRAMConfig* dram_config =
      new DRAMConfig(ddr_type, module_size, static_cast<uint16_t>(num_ranks),
                     static_cast<uint16_t>(dq_width));
  MemoryConfig* memory_config = new MemoryConfig(1, 1, num_dimms, dram_config);

  Addressing* sudoku = new Addressing(dram_config, memory_config,
                                      memory_pool_config, addressing_config);
  sudoku->Initialize();
  if (mode == "stat" || mode == "STAT") {
    // Stat -- single
    spdlog::info("[+] StatSingleMemoryAccess");
    sudoku->StatSingleMemoryAccess();
    // Stat -- paired
    spdlog::info("[+] StatPairedMemoryAccess");
    sudoku->StatPairedMemoryAccess();
  } else if (mode == "check" || mode == "CHECK") {
    // Check
    spdlog::info("[+] CheckPairedMemoryAccess");
    Constraints c =
        Constraints(same_functions, diff_functions, row_bits, column_bits);
    sudoku->CheckPairedMemoryAccess(c);
  } else {
    spdlog::error("[-] Unsupported mode: {}", mode);
    exit(EXIT_FAILURE);
  }
  sudoku->Finalize();

  delete sudoku;
  delete memory_pool_config;
  delete memory_config;
  delete dram_config;
  delete addressing_config;

  return 0;
}
