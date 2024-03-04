#include "preamble.h"

#include "Vtop.h"

#include <verilated_vcd_c.h>

struct input {
    bool prescale;
    bool user_clk;
    bool clk_select;
    bool reset;
    bool data;

    operator u8() {
        return data | prescale << 4 | user_clk << 5 | clk_select << 6
            | reset << 7;
    }
};

void set(u8& byte, u8 bit, bool val) {
    byte &= ~(1 << bit);
    byte |= (val ? 1 : 0) << bit;
}

void send_byte(auto&& step, Vtop& dut, u8 data) {
    // start bit
    set(dut.ui_in, 0, false);
    step(8);

    // data
    for (auto i = 0; i < 8; ++i) {
        set(dut.ui_in, 0, (data >> i) & 1);
        step(8);
    }

    // stop bit
    set(dut.ui_in, 0, true);
    step(8);
    set(dut.ui_in, 0, true);
    step(8);
}

auto main() -> int {
    const auto waveform_file = "build/waveform.vcd";

    auto dut = Vtop{};

    Verilated::traceEverOn(true);
    auto trace = VerilatedVcdC{};
    dut.trace(&trace, 5);
    trace.open(waveform_file);

    auto sim_time = 0;
    auto step = [&](u32 n = 1) {
        for (auto _ = 0; _ < n; ++_) {
            dut.clk ^= 1;
            dut.eval();
            trace.dump(sim_time++);
            dut.clk ^= 1;
            dut.eval();
            trace.dump(sim_time++);
        }
    };

    auto def = input{
        .prescale = 1,
        .clk_select = 0,
        .reset = 1,
        .data = 1,
    };

    dut.ui_in = def;
    step(10);

    def.reset = 0;
    dut.ui_in = def;

    step(20);

    // send length
    send_byte(step, dut, 0x00);
    send_byte(step, dut, 0x00);
    send_byte(step, dut, 0x00);
    send_byte(step, dut, 0x02);
    step(100);
    send_byte(step, dut, 0x11);
    send_byte(step, dut, 0x11);
    send_byte(step, dut, 0x11);
    send_byte(step, dut, 0x11);
    step(100);    
    send_byte(step, dut, 0x22);
    send_byte(step, dut, 0x22);
    send_byte(step, dut, 0x22);
    send_byte(step, dut, 0x22);
    step(400);
}
