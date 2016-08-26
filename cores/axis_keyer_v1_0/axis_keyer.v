
`timescale 1 ns / 1 ps

module axis_keyer #
(
  parameter integer AXIS_TDATA_WIDTH = 16
)
(
  // System signals
  input  wire                        aclk,

  input  wire                        key_flag,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  assign m_axis_tdata = key_flag ? {1'b0, {(AXIS_TDATA_WIDTH-1){1'b1}}} : {(AXIS_TDATA_WIDTH){1'b0}};
  assign m_axis_tvalid = 1'b1;

endmodule
