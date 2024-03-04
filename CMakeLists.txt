cmake_minimum_required(VERSION 3.12)
project(tb)
set (CMAKE_CXX_STANDARD 20)
include(FetchContent)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

find_package(verilator)
if (NOT verilator_FOUND)
  message(FATAL_ERROR "Verilator was not found. Either install it, or set the VERILATOR_ROOT environment variable")
endif()

add_executable(run_tb tb/tb.cpp)

target_link_libraries(run_tb
  PRIVATE
    fmt::fmt
)

verilate(
  run_tb
  COVERAGE
  TRACE
  INCLUDE_DIRS "../make_tracing_c"
  VERILATOR_ARGS -x-assign fast
  TOP_MODULE top
  SOURCES
    src/top.sv
    src/f_permutation.v
    src/keccak.v
    src/padder.v
    src/padder1.v
    src/rconst.v
    src/round.v
    src/sha.sv
    src/uart.v
    src/uart_rx.v
    src/uart_tx.v
)
