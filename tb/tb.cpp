#include "preamble.h"

#include "Vtop.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <random>

#include <verilated_vcd_c.h>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include "sha1.h"

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
    usize idx = 0;

    static constexpr u32 timeout = 16 * 10 * 2;

    std::array<u8, 20> expected;

   public:
    bool good = true;

    void set_expected(std::array<u8, 20> exp) {
        good = true;
        idx = 0;
        expected = exp;
    }

    // returns true when timeout
    auto resume(const Vtop& dut) -> bool {
        if (auto b = r.resume(dut)) {
            auto exp = expected.at(idx);
            if (*b != exp) {
                good = false;
                fmt::println(
                    "FAIL at {}: recv 0x{:02X}, exp 0x{:02X}", sim_time, *b, exp
                );
            }
            idx++;
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

void send_byte_(auto&& step, Vtop& dut, u8 data) {
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

constexpr auto max_data_len = 55;
void do_test(
    std::random_device& rd,
    auto&& step,
    auto&& send_byte,
    read_tb& r,
    std::string_view data
) {
    assert(data.size() <= max_data_len);

    auto sha = SHA1{};
    sha.update(std::string{data});
    r.set_expected(sha.final());

    for (char ch : data) {
        send_byte(ch);
    }
    send_byte(0x80);

    auto num_padding = 64 - 1 - 4 - data.size();
    assert(num_padding > 0);
    for (auto _ = 0; _ < num_padding; ++_) {
        send_byte(0x00);
    }

    auto num_bits = data.size() * 8;
    send_byte(num_bits >> 24);
    send_byte(num_bits >> 16);
    send_byte(num_bits >> 8);
    send_byte(num_bits >> 0);

    step(200);
    while (!step()) {}

    fmt::println("{0:=^{1}} {2} {0:=^{1}}", "", 29, r.good ? "PASS" : "FAIL");
    fmt::println("data (len {:2})\nat {}", data.size(), sim_time);

    auto data_ = std::span(data);
    const auto bpr = 16_usize;
    while (!data_.empty()) {
        const auto len = std::min(bpr, data_.size());
        fmt::println("{::02X}", data_.first(len));
        data_ = data_.subspan(len);
    }
    fmt::println("{0:-^{1}}", "", 29 * 2 + 4 + 2);

    step(std::uniform_int_distribution(0, 1000)(rd));
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
    auto send_byte = [&](u8 byte) { send_byte_(step, dut, byte); };

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

    auto rd = std::random_device{};

    auto test = [&](std::string_view str) {
        do_test(rd, step, send_byte, r, str);
    };
    test("abc");
    test("def");
    test("123");
    test("abcdefghijklmnopqrstuvwxyz");
    test("");

    auto len_dist = std::uniform_int_distribution<u64>{0, max_data_len};
    auto char_dist = std::uniform_int_distribution<u8>{0x00, 0xff};
    for (auto _ = 0; _ < 20; ++_) {
        auto len = len_dist(rd);
        auto str = std::string(len, '\0');
        for (auto i = 0; i < len; ++i) {
            str[i] = char_dist(rd);
        }
        test(str);
    }
}
