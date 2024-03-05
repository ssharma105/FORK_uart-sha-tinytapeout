`default_nettype none `timescale 100ns / 100ns

typedef enum logic [7:0] {
    IDLE,
    READ_0,
    READ_1,
    READ_2,
    READ_3,
    BUSY,
    PROCESS_0,
    PROCESS_1,
    PROCESS_2,
    PROCESS_3,
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
    assign uo_out[7:1] = '0;

    reg load_i; reg load_i_n;
    reg new_text_i; reg new_text_i_n;
    wire ready_o;
    reg [127:0] data_i; reg [127:0] data_i_n;
    wire [127:0] data_o;
    
    md5 md5_inst(
        clk,
        .reset(~rst),
        load_i,
        ready_o,
        new_text_i,
        data_i,
        data_o
    );
            
    

    State state = IDLE;
    State state_n;

    reg [7:0] length = 0;
    reg [7:0] length_n;
    reg [7:0] total; reg[7:0] total_n;
    reg start_flag;
    reg start_flag_n;

    always_ff @(posedge uclk) begin
        if (rst) begin
            rx_axis_tready <= 0;
            tx_axis_tdata <= 0;
            tx_axis_tvalid <= 0;
            state <= IDLE;
            load_i <= 0;
            new_text_i <= 0;
            data_i <= 0;
            length <= 0;
            total <= 0;
        end else begin
            state <= state_n;
            rx_axis_tready <= rx_axis_tready_n;
            tx_axis_tdata <= tx_axis_tdata_n;
            tx_axis_tvalid <= tx_axis_tvalid_n;
            load_i <= load_i_n;
            new_text_i <= new_text_i_n;
            data_i <= data_i_n;
            length <= length_n;
            total <= total_n;
        end
    end

    always_comb begin
        state_n = state;
        rx_axis_tready_n = rx_axis_tready;
        tx_axis_tdata_n = tx_axis_tdata;
        tx_axis_tvalid_n = tx_axis_tvalid;
        data_i_n = data_i;
        new_text_i_n = new_text_i;
        load_i_n = load_i;
        length_n = length;     
        total_n = total;
        case (state)
            IDLE: begin
                rx_axis_tready_n = 1;
                length_n = 8'd16; 
                state_n = READ_0;
            end

            READ_0: begin
                if(total == 0) begin
                    total_n = rx_axis_tdata;
                    state_n = IDLE;
                end
                else if(length > 0) begin
                    if (rx_axis_tvalid) begin
                        rx_axis_tready_n = 0;
                        data_i_n[(length*8)-1-:8] = rx_axis_tdata;
                        length_n = length - 1;
                    end
                end else if (length == 0) begin 
                    state_n = PROCESS_0;
                    load_i_n = 1;
                    total_n = total - 1;
                end 
            end
            
            PROCESS_0: begin
                load_i_n = 0;
                if(total == 0) begin 
                    state_n = PROCESS_1;
                end else begin
                    state_n = IDLE;
                end
            end

            BUSY: begin
                load_i_n=0;
            end

            SEND_OUT: begin
            end

            default: state_n = IDLE;
        endcase
    end
endmodule
