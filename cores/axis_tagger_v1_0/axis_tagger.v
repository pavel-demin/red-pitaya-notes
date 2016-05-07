
`timescale 1 ns / 1 ps

module axis_tagger #
(
  parameter integer AXIS_TDATA_WIDTH = 256
)
(
  // System signals
  input  wire                        aclk,

  input  wire                        tag_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg int_data_reg, int_flag_reg;

  always @(posedge aclk)
  begin
    int_data_reg <= tag_data;
    if(tag_data & ~int_data_reg)
    begin
      int_flag_reg <= 1'b1;
    end
    else if(s_axis_tvalid)
    begin
      int_flag_reg <= 1'b0;
    end
  end

  assign s_axis_tready = m_axis_tready;
  assign m_axis_tdata = {s_axis_tdata[255:209], int_flag_reg, s_axis_tdata[207:0]};
  assign m_axis_tvalid = s_axis_tvalid;

endmodule
