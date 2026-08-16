
`timescale 1 ns / 1 ps

module axis_variant #
(
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire                        cfg_flag,

  input  wire [AXIS_TDATA_WIDTH-1:0] cfg_data0,
  input  wire [AXIS_TDATA_WIDTH-1:0] cfg_data1,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg  int_init_reg;

  wire [AXIS_TDATA_WIDTH-1:0] int_data_wire;

  assign int_data_wire = cfg_flag ? cfg_data1 : cfg_data0;

  always @(posedge aclk)
  begin
    int_init_reg <= ~aresetn;
  end

  output_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_data_wire), .in_valid(int_init_reg | (m_axis_tdata != int_data_wire)), .in_ready(),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );


endmodule
