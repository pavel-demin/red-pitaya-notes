
`timescale 1 ns / 1 ps

module axis_variable #
(
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [AXIS_TDATA_WIDTH-1:0] cfg_data,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg  int_init_reg;

  always @(posedge aclk)
  begin
    int_init_reg <= ~aresetn;
  end

  output_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(cfg_data), .in_valid(int_init_reg | (m_axis_tdata != cfg_data)), .in_ready(),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

endmodule
