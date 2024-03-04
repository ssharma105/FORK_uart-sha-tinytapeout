#include "preamble.h"

#include "Vtop.h"

#include <verilated_vcd_c.h>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

constexpr auto pow_2(u8 num_bits) -> u32 { return 1 << num_bits; }

enum class opcode : u8 {
    add = 0,
    sub = 1,
    mul = 2,
    div = 3,
};

auto main() -> int {
    const auto max_sim_time = 1000;
    const auto waveform_file = "build/waveform.vcd";

    auto dut = Vtop{};

    Verilated::traceEverOn(true);
    auto trace = VerilatedVcdC{};
    dut.trace(&trace, 5);
    trace.open(waveform_file);

    auto sim_time = 0;

    for (auto i = 0; i < 0xff + 1; ++i) {
        dut.ui_in = i;

        dut.eval();

        assert(dut.uo_out == i);

        trace.dump(sim_time);
        sim_time++;
    }
}
