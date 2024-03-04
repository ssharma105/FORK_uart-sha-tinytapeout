`default_nettype none `timescale 100ns / 100ns

typedef enum logic [7:0] {
    IDLE,
    READ_0,
    READ_1,
    READ_2,
    READ_3,
    BUSY,
    PROCESS,
    SEND_OUT
} State;

module top (
    input var clk,
    input var [7:0] ui_in,
    output var [7:0] uo_out
);
    reg [15:0] prescale = ui_in[4] ? 16'd1 : 16'd165;

    wire rst = ui_in[7];
    wire clk_select = ui_in[6];
    wire uclk = clk_select ? ui_in[5] : clk;

    // RX UART Data Signals
    wire [7:0] rx_axis_tdata;
    wire rx_axis_tvalid;
    reg rx_axis_tready, rx_axis_tready_n;
    wire rx_busy, overrun_error, frame_error;

    uart_rx RX (
        .clk(uclk),
        .rst,
        .rxd(ui_in[0]),
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
        .clk(uclk),
        .rst,
        .txd(uo_out[0]),
        .s_axis_tdata(tx_axis_tdata),
        .s_axis_tvalid(tx_axis_tvalid),
        .s_axis_tready(tx_axis_tready),
        .prescale,
        .busy(tx_busy)
    );

    reg start = 0;
    reg start_n;
    reg [31:0] data_in = 0;
    reg [31:0] data_in_n;
    reg data_last;
    reg data_last_n;
    reg data_valid;
    reg data_valid_n;
    wire data_in_ready;
    wire [511:0] hash;
    wire out_valid;
    sha sha_inst (
        .clk_i(uclk),
        .rst_i(rst),
        .start,
        .data_in,
        .data_last,
        .data_valid,
        .data_in_ready,
        .hash,
        .out_valid
    );

    State state = IDLE;
    State state_n;

    reg [31:0] length = 0;
    reg [31:0] length_n;
    reg start_flag; reg start_flag_n;

    always_ff @(posedge uclk) begin
        if (rst) begin
            rx_axis_tready <= 0;
            tx_axis_tdata <= 0;
            tx_axis_tvalid <= 0;
            start <= 0;
            data_in <= 0;
            data_last <= 0;
            data_valid <= 0;
            length <= 0;
            state <= IDLE;
        end else begin
            state <= state_n;
            rx_axis_tready <= rx_axis_tready_n;
            tx_axis_tdata <= tx_axis_tdata_n;
            tx_axis_tvalid <= tx_axis_tvalid_n;
            start <= start_n;
            data_in <= data_in_n;
            data_last <= data_last_n;
            data_valid <= data_valid_n;
            length <= length_n;
            start_flag <= start_flag_n;
        end
    end

    always_comb begin
        state_n = state;
        rx_axis_tready_n = rx_axis_tready;
        tx_axis_tdata_n = tx_axis_tdata;
        tx_axis_tvalid_n = tx_axis_tvalid;
        start_n = start;
        data_in_n = data_in;
        data_last_n = data_last;
        data_valid_n = data_valid;
        length_n = length;
        start_flag_n = start_flag;
        
        case (state)
            IDLE: begin
                rx_axis_tready_n = 1;
                data_valid_n = 0;
                state_n = READ_0;
            end

            READ_0: begin
                if (rx_axis_tvalid) begin
                    rx_axis_tready_n = 0;
                    data_in_n[31:24] = rx_axis_tdata;
                    state_n = READ_1;
                end else begin
                    rx_axis_tready_n = 1;
                end
            end

            READ_1: begin
                if (rx_axis_tvalid) begin
                    rx_axis_tready_n = 0;
                    data_in_n[23:16] = rx_axis_tdata;
                    state_n = READ_2;
                end else begin
                    rx_axis_tready_n = 1;
                end
            end

            READ_2: begin
                if (rx_axis_tvalid) begin
                    rx_axis_tready_n = 0;
                    data_in_n[15:8] = rx_axis_tdata;
                    state_n = READ_3;
                end else begin
                    rx_axis_tready_n = 1;
                end
            end

            READ_3: begin
                if (rx_axis_tvalid) begin
                    rx_axis_tready_n = 0;
                    data_in_n[7:0] = rx_axis_tdata;
                    state_n = PROCESS;
                end else begin
                    rx_axis_tready_n = 1;
                end
            end

            PROCESS: begin
                if (length == 0 && ~start_flag) begin
                    length_n = data_in;
                    start_flag_n = 1;
                end else if (length == 0 && start_flag) begin
                    data_valid_n = 1;
                    data_last_n = 1;
                    state_n = BUSY;
                end else begin
                    if (data_in_ready) begin
                        data_valid_n = 1;
                        length_n = length - 1;
                        state_n = IDLE;
                    end else begin
                        state_n = PROCESS;
                    end
                end
            end

            BUSY: begin
                data_valid_n = 0;
                data_last_n = 0;

                if (out_valid) begin
                    $display("OUTPUT HASH AVAILABLE");
                    state_n = SEND_OUT;
                    length_n = 64; // reuse length registers here.
                end else begin
                    state_n = BUSY;
                end
            end

            SEND_OUT: begin
                if(tx_axis_tready) begin
                    tx_axis_tdata_n = hash[(length*8)-1-:8];
                    tx_axis_tvalid_n = 1;
                    length_n = length - 1;
                end else begin  
                    tx_axis_tvalid_n = 0;
                end

                if (length == 0) begin
                    state_n = IDLE;
                end 
            end

            
            default: state_n = IDLE;
        endcase
    end
endmodule
