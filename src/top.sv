`default_nettype none

module top (
    input var clk,
    input var [7:0] ui_in,
    output var [7:0] uo_out
);
    wire rst = ui_in[7];
    wire uclk = ui_in[6] ? ui_in[5] : clk;

    // UPDATE ME!!!! 130 for 10Mhz, 260 for 20Mhz, 1302 for 100MHz
    wire [15:0] prescale_base = 130;
    wire [15:0] prescale = ui_in[4] ? 1 : prescale_base;

    assign uo_out[7:1] = 0;

    sha_top wrapped (
        .clk(uclk),
        .rst,
        .rxd(ui_in[0]),
        .txd(uo_out[0]),
        .prescale
    );
endmodule

typedef enum logic [7:0] {
    IDLE,
    READ,
    BUSY,
    PROCESS_0,
    PROCESS_1,
    SEND
} State;

module sha_top (
    input var clk,
    input var rxd,
    input var rst,
    output var txd,
    input var [15:0] prescale
);
    // RX UART Data Signals
    wire [7:0] rx_axis_tdata;
    wire rx_axis_tvalid;
    reg rx_axis_tready, rx_axis_tready_n;
    wire rx_busy, overrun_error, frame_error;

    uart_rx RX (
        .clk,
        .rst,
        .rxd,
        .m_axis_tdata(rx_axis_tdata),
        .m_axis_tvalid(rx_axis_tvalid),
        .m_axis_tready(rx_axis_tready),
        .prescale,
        .busy(rx_busy),
        .overrun_error,
        .frame_error
    );

    // TX UART Data Signals
    reg [7:0] tx_axis_tdata, tx_axis_tdata_n;
    reg tx_axis_tvalid, tx_axis_tvalid_n;
    wire tx_axis_tready;
    wire tx_busy;

    uart_tx TX (
        .clk,
        .rst,
        .txd,
        .s_axis_tdata(tx_axis_tdata),
        .s_axis_tvalid(tx_axis_tvalid),
        .s_axis_tready(tx_axis_tready),
        .prescale,
        .busy(tx_busy)
    );

    reg init, init_n;
    reg next, next_n;
    reg [511:0] block, block_n;
    reg [127:0] data_o_capture, data_o_capture_n;

    wire ready;
    wire [159:0] digest;
    wire digest_valid;

    sha1_core sha_core_inst (
        .clk,
        .reset_n(~rst),
        .init,
        .next,
        .block,
        .ready,
        .digest,
        .digest_valid
    );

    State state, state_n;
    reg [7:0] length, length_n;
    reg [159:0] digest_capture, digest_capture_n;

    `define reset(var, val) \
        always_ff @(posedge clk) \
            if (rst) var <= val; \
            else var <= var``_n;

    `reset(rx_axis_tready, 0)
    `reset(tx_axis_tdata, 0)
    `reset(tx_axis_tvalid, 0)
    `reset(state, IDLE)
    `reset(length, 0)
    `reset(init, 0)
    `reset(next, 0)
    `reset(block, 0)
    `reset(digest_capture, 0)

    reg [4:0] free_counter;
    always_ff @(posedge clk) free_counter <= free_counter + 1;
    reg [8:0] lfsr;
    always_ff @(posedge clk)
        if (rst) lfsr <= 1;
        else lfsr <= {lfsr[7:0], lfsr[7] ^ lfsr[4] ^ lfsr[3]};

    always_comb begin
        state_n = state;
        rx_axis_tready_n = rx_axis_tready;
        tx_axis_tdata_n = tx_axis_tdata;
        tx_axis_tvalid_n = tx_axis_tvalid;

        length_n = length;

        init_n = init;
        next_n = next;
        block_n = block;

        digest_capture_n = digest_capture;

        case (state)
            IDLE: begin
                rx_axis_tready_n = 1;
                length_n = 8'd64;
                state_n = READ;
                init_n = 0;
            end

            READ: begin
                rx_axis_tready_n = 1;
                if ((length > 0) & rx_axis_tvalid) begin
                    block_n[(length*8)-1-:8] = rx_axis_tdata;
                    rx_axis_tready_n = 0;
                    length_n = length - 1;
                    state_n = READ;
                end else if (length == 0) begin
                    state_n = PROCESS_0;
                    if (free_counter == 0) begin
                        block_n[lfsr] ^= 1;
                    end
                end

            end

            PROCESS_0: begin
                init_n  = 1;
                state_n = PROCESS_1;
            end

            PROCESS_1: begin
                init_n  = 0;
                state_n = BUSY;
            end

            BUSY: begin
                if (ready) begin
                    state_n = SEND;
                    digest_capture_n = digest;
                    length_n = 20;
                end
            end

            SEND: begin
                tx_axis_tvalid_n = 0;
                if (tx_axis_tready && ~tx_axis_tvalid && length > 0 && ~tx_busy) begin
                    tx_axis_tdata_n = digest_capture[(length*8)-1-:8];
                    tx_axis_tvalid_n = 1;
                    length_n = length - 1;
                end else if (length == 0) begin
                    tx_axis_tvalid_n = 0;
                    state_n = IDLE;
                end
            end

            default: state_n = IDLE;
        endcase
    end
endmodule
