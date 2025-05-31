#include <getopt.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "internal/constants.h"
#include "sudoku.h"
#include "sudoku_addressing.h"

using namespace sudoku;

static const char help_msg[] =
    R"([?] Usage: 
    $ sudo numactl -C [core] -m [memory] ./reverse_functions [OPTIONS]

    Options:
      --output,  -o [STR]   Output filename prefix
      --pages,   -p [INT]   Number of OS memory pages to allocate
      --type,    -t [STR]   DDR type (ddr4 or ddr5)
      --num,     -n [INT]   Number of DRAM modules
      --size,    -s [INT]   Size of DRAM module in GB
      --rank,    -r [INT]   Number of ranks per DRAM module
      --width,   -w [INT]   DQ width of DRAM (8, 16, or 32)

      --debug,   -d         Enable debug output
      --verbose, -v         Enable verbose mode
      --log,     -l         Enable logging
      --help,    -h         Show this help message
)";

void PrintHelp(const std::string& msg = "") {
  if (!msg.empty()) {
    spdlog::error("{}", msg);
    spdlog::info("Use --help or -h to see usage.");
  } else {
    spdlog::info("{}", help_msg);
  }
}

int main(int argc, char* argv[]) {
  std::string fname_prefix = "default", type = "ddr4";
  uint64_t num_pages = 19, page_size = 1ULL * 1024ULL * 1024ULL * 1024ULL,
           granularity = (1ULL << CACHELINE_OFFSET), num_dimms = 1,
           module_size = 32ULL * 1024ULL * 1024ULL * 1024ULL, num_ranks = 2,
           dq_width = 8;
  DDRType ddr_type = DDRType::DDR4;
  bool debug = false, verbose = false, logging = false;

  // check sudo privilege
  if (getuid() != 0) {
    spdlog::error("reverse_functions requires sudo privilege.");
    exit(EXIT_FAILURE);
  }

  // parse argument
  static struct option long_options[] = {
      {"output", optional_argument, 0, 'o'},
      {"pages", optional_argument, 0, 'p'},
      {"type", optional_argument, 0, 't'},
      {"num", optional_argument, 0, 'n'},
      {"size", optional_argument, 0, 's'},
      {"rank", optional_argument, 0, 'r'},
      {"width", optional_argument, 0, 'w'},
      {"debug", optional_argument, 0, 'd'},
      {"verbose", optional_argument, 0, 'v'},
      {"log", optional_argument, 0, 'l'},
      {"help", optional_argument, 0, 'h'},
      {0, 0, 0, 0},
  };
  if (argc < 2) {
    PrintHelp("");
    exit(EXIT_FAILURE);
  } else {
    int opt, idx;
    while ((opt = getopt_long(argc, argv, "o:p:t:n:s:r:w:dvlh", long_options,
                              &idx)) != -1) {
      switch (opt) {
        case 'o':
          if (optarg) {
            fname_prefix = std::string(optarg);
          }
          break;
        case 'p':
          num_pages = strtoull(optarg, NULL, 10);
          break;
        case 't':
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
  sudoku->ReverseAddressingFunctions();
  sudoku->Finalize();

  delete sudoku;
  delete memory_pool_config;
  delete memory_config;
  delete dram_config;
  delete addressing_config;

  return 0;
}
