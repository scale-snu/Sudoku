# Sudoku: Decomposing DRAM Address Mapping into Component Functions

**Sudoku** is a software-based tool for decomposing DRAM address mapping into component functions. 
Sudoku runs on Linux and supports Intel Core and AMD Ryzen processors with both DDR4 and DDR5 DRAMs.

We have tested Sudoku on the following systems:
| Processor                                       | Microcode | Motherboard  | Tested DDR                |
| ----------------------------------------------- | --------- | ------------ | ------------------------- |
| Intel Core i9-12900K (12th Alder Lake)          | 0x38      | ASUS Z690-A  | 32GB DDR4-3200 2Rx8 UDIMM |
| Intel Core i9-12900K (12th Alder Lake)          | 0x38      | ASUS Z690-F  | 32GB DDR4-3200 2Rx8 UDIMM |
| Intel Core i9-14900K (14th Raptor Lake Refresh) | 0x12C     | MSI B760M    | 32GB DDR5-4800 2Rx8 UDIMM |
| AMD Ryzen 9 7950X (Zen 4)                       | 0xA601206 | ASRock X670E | 32GB DDR5-4800 2Rx8 UDIMM |

Sudoku provides the following key features:
* [reverse_functions](./sudoku/reverse_functions.cc): Reverse-engineering DRAM addressing functions using row buffer conflicts (refer to [DRAMA](https://github.com/isec-tugraz/drama))
* [identify_bits](./sudoku/identify_bits.cc): Identifying DRAM row and column bits from given DRAM addressing functions using row buffer conflicts
* [validate_mapping](./sudoku/validate_mapping.cc): Validating DRAM address mapping by checking system's injectivity
* [decompose_functions](./sudoku/decompose_functions.cc): Decomposing DRAM address mapping into component functions

Also, for get statistics of paired memory accesses, auto-refreshes, and consecutive memory accesses, Sudoku provides the following testing binaries:
* [watch_conflicts](./sudoku/testing/watch_conflicts.cc)
* [watch_refreshes](./sudoku/testing/watch_refreshes.cc)
* [watch_consecutive_accesses](./sudoku/testing/watch_consecutive_accesses.cc)

## Environment setup

Sudoku requires large memory coverage via **1 GB hugepages**. 
Suggested hugepage count is over half of the system memory (to consider MSB bits of system's physical address).
We enable the desired number of hugepages as follows:

```bash
# Change the grub file
sudo vi /etc/default/grub
GRUB_CMDLINE_LINUX_DEFAULT="iomem=relaxed quiet splash default_hugepagesz=1G hugepagesz=1G hugepages={num-hugepages}"

# Update grub
sudo update-grub

# Reboot the system to enable modified grub setting
sudo reboot
```

Sudoku measure timings using \texttt{RDTSC} instruction, requiring fixed core frequency for measurement accuracy. 
We fix the processor's frequency to base clock frequency using cpupower.

```bash
sudo cpupower frequency-set -d 3.2GHz
sudo cpupower frequency-set -u 3.2GHz
```

Or, you can disable processor's DVFS in the BIOS.

## Build Sudoku

Sudoku requires precise timing threshold for correct functionality. 
We provide tested constants through header files. 
Please pass the correct compiler flag to link the correct constants.

```bash
vi CMakeLists.txt
# Please modify the below option.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D{DESIRED_ARCH_WITH_DDR_TYPE}")
# Supported flag:
#   -DCOMPILE_ALDER_LAKE_DDR4
#   -DCOMPILE_ALDER_LAKE_DDR5
#   -DCOMPILE_RAPTOR_LAKE
#   -DCOMPILE_ZEN_4_DDR5

mkdir -p build && cd build
cmake ..
cmake --build . --parallel
```

## Use Sudoku
### (Optional) Reverse-engineering DRAM addressing functions

Sudoku provides the code for reverse-engineering DRAM addressing functions. 
We employ [DRAMA](https://github.com/isec-tugraz/drama)'s brute-forcing method.

```bash
sudo numactl -C {core} -m {memory} ./reverse_addressing 
    -o {fname_prefix} -p {num_pages} -t {ddr_type} -n {num_dimms} \
    -s {dimm_size} -r {num_ranks} -w {dq_width} -d -v -l
```

### Identifying DRAM row and column bits

```bash
sudo numactl -C {core} -m {memory} ./identify_bits 
    -o {fname_prefix} -p {num_pages} -t {ddr_type} -n {num_dimms} \
    -s {dimm_size} -r {num_ranks} -w {dq_width} \
    -f {functions_separated_by_commas} \
    -d -v -l
```

### Validating DRAM address mapping

```bash
sudo numactl -C {core} -m {memory} ./validate_mapping
    -o {fname_prefix} -p {num_pages} -t {ddr_type} -n {num_dimms} \
    -s {dimm_size} -r {num_ranks} -w {dq_width} \
    -f {functions_separated_by_commas} \
    -R {row_bits} -C {column_bits} \
    -d -v -l
```

### Decomposing DRAM address mapping into component functions

```bash
sudo numactl -C {core} -m {memory} ./decompose_functions
    -o {fname_prefix} -p {num_pages} -t {ddr_type} -n {num_dimms} \
    -s {dimm_size} -r {num_ranks} -w {dq_width} \
    -f {functions_separated_by_commas} \
    -R {row_bits} -C {column_bits} \
    -d -v -l
```

## License

This project is licensed under the MIT License (see [LICENSE](./LICENSE)).

## Contact

Minbok Wi (minbok.wi@scale.snu.ac.kr)
