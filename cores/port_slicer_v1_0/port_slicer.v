
`timescale 1 ns / 1 ps

module port_slicer #
(
  parameter integer DIN_WIDTH = 32,
  parameter integer DIN_FROM = 31,
  parameter integer DIN_TO = 0
)
(
  input  wire [DIN_WIDTH-1:0]     din,
  output wire [DIN_FROM-DIN_TO:0] dout
);

  assign dout = din[DIN_FROM:DIN_TO];

endmodule
