#include "preamble.h"

#include "Vtop.h"

#include <iomanip>
#include <iostream>
#include <optional>

#include <verilated_vcd_c.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

u32 sim_time = 0;

class byte_reader {
    enum class state : u8 { start, data, stop };
    u32 start_time;
    state s = state::start;
    u8 bit = 0;
    u8 val = 0;

    static constexpr u32 steps_per_bit = 16;

   public:
    auto resume(const Vtop& dut) -> std::optional<u8> {
        switch (s) {
            case state::start:
                if (!(dut.uo_out & 1)) {
                    s = state::data;
                    bit = 0;
                    val = 0;
                    start_time = sim_time;
                }
                return {};
            case state::data:
                if (sim_time < start_time + (bit + 1) * steps_per_bit) {
                    return {};
                }
                val = dut.uo_out << 7 | val >> 1;
                bit++;
                if (bit >= 8) {
                    s = state::stop;
                }
                return {};
            case state::stop:
                if (sim_time < start_time + 10 * steps_per_bit) {
                    return {};
                }
                // stop bit should be set
                assert(dut.uo_out & 1);
                s = state::start;
                return val;
        }
    }
};

class read_tb {
    byte_reader r;
    u32 last_read_pos = -1;

    static constexpr u32 timeout = 16 * 10 * 2;

   public:
    // returns true when timeout
    auto resume(const Vtop& dut) -> bool {
        if (auto b = r.resume(dut)) {
            fmt::println("===== data: 0x{:02X} =====", *b);
            last_read_pos = sim_time;
        }

        if (last_read_pos != -1 && sim_time - last_read_pos > timeout) {
            return true;
        }

        return false;
    }
};

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

    read_tb r;

    auto step = [&](u32 n = 1) -> bool {
        auto timeout = false;
        for (auto _ = 0; _ < n; ++_) {
            dut.clk ^= 1;
            dut.eval();
            trace.dump(sim_time++);
            dut.clk ^= 1;
            dut.eval();
            trace.dump(sim_time++);
            timeout |= r.resume(dut);
        }

        return timeout;
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

    auto send_word = [&](u32 word) {
        send_byte(step, dut, (word >> 24) & 0xff);
        send_byte(step, dut, (word >> 16) & 0xff);
        send_byte(step, dut, (word >> 8) & 0xff);
        send_byte(step, dut, (word >> 0) & 0xff);
    };

    // send total
    send_byte(step, dut, 0x04);
    // send part one
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    // send part two
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    // send part three
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    // send part four
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    send_word(0x6161'6161);
    
    step(20000);
    //while (!step()) {}
}
