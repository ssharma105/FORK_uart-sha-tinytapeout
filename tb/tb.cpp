#include "preamble.h"

#include "Vtop.h"

#include <verilated_vcd_c.h>

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
