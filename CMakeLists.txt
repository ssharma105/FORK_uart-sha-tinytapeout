cmake_minimum_required(VERSION 3.12)
project(tb)
set (CMAKE_CXX_STANDARD 20)

find_package(verilator)
if (NOT verilator_FOUND)
  message(FATAL_ERROR "Verilator was not found. Either install it, or set the VERILATOR_ROOT environment variable")
endif()

add_executable(run_tb tb/tb.cpp)

verilate(run_tb COVERAGE TRACE
  INCLUDE_DIRS "../make_tracing_c"
  VERILATOR_ARGS -x-assign fast
  SOURCES src/top.sv src/second.sv)
