
`timescale 1 ns / 1 ps

module axis_fifo #
(
  parameter integer S_AXIS_TDATA_WIDTH = 32,
  parameter integer M_AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                          aclk,

  // Slave side
  output wire                          s_axis_tready,
  input  wire [S_AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                          s_axis_tvalid,

  // Master side
  input  wire                          m_axis_tready,
  output wire [M_AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                          m_axis_tvalid,

  // FIFO_WRITE port
  input  wire                          fifo_write_full,
  output wire [S_AXIS_TDATA_WIDTH-1:0] fifo_write_data,
  output wire                          fifo_write_wren,

  // FIFO_READ port
  input  wire                          fifo_read_empty,
  input  wire [M_AXIS_TDATA_WIDTH-1:0] fifo_read_data,
  output wire                          fifo_read_rden
);

  assign m_axis_tdata = fifo_read_empty ? {(M_AXIS_TDATA_WIDTH){1'b0}} : fifo_read_data;
  assign m_axis_tvalid = 1'b1;

  assign s_axis_tready = 1'b1;

  assign fifo_read_rden = m_axis_tready;

  assign fifo_write_data = s_axis_tdata;
  assign fifo_write_wren = s_axis_tvalid;

endmodule
