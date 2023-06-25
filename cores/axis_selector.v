
`timescale 1 ns / 1 ps

module axis_selector #
(
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire                        cfg_data,

  // Slave side
  input  wire [AXIS_TDATA_WIDTH-1:0] s00_axis_tdata,
  input  wire                        s00_axis_tvalid,
  output wire                        s00_axis_tready,

  input  wire [AXIS_TDATA_WIDTH-1:0] s01_axis_tdata,
  input  wire                        s01_axis_tvalid,
  output wire                        s01_axis_tready,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,
  input  wire                        m_axis_tready
);

  wire [AXIS_TDATA_WIDTH-1:0] int_data_wire;
  wire int_valid_wire, int_ready_wire;

  assign int_data_wire = cfg_data ? s01_axis_tdata : s00_axis_tdata;
  assign int_valid_wire = cfg_data ? s01_axis_tvalid : s00_axis_tvalid;

  assign s00_axis_tready = ~cfg_data & int_ready_wire;
  assign s01_axis_tready = cfg_data & int_ready_wire;

  inout_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_data_wire), .in_valid(int_valid_wire), .in_ready(int_ready_wire),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

endmodule
