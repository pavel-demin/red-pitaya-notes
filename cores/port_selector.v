
`timescale 1 ns / 1 ps

module port_selector #
(
  parameter integer DOUT_WIDTH = 32
)
(
  input  wire                    cfg,
  input  wire [2*DOUT_WIDTH-1:0] din,
  output wire [DOUT_WIDTH-1:0]   dout
);

  assign dout = cfg ? din[2*DOUT_WIDTH-1:DOUT_WIDTH] : din[DOUT_WIDTH-1:0];

endmodule
