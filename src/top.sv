`default_nettype none

module top (
    input var clk,
    input var [7:0] ui_in,
    output var [7:0] uo_out
);
  second ensure_second_file ();
  assign uo_out = ui_in;
endmodule
