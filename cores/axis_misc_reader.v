
`timescale 1 ns / 1 ps

module axis_misc_reader #
(
  parameter integer S_AXIS_TDATA_WIDTH = 40,
  parameter integer M_AXIS_TDATA_WIDTH = 32,
  parameter integer MISC_WIDTH = 8
)
(
  // System signals
  input  wire                          aclk,
  input  wire                          aresetn,

  output wire [MISC_WIDTH-1:0]         misc_data,

  // Slave side
  output wire                          s_axis_tready,
  input  wire [S_AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                          s_axis_tvalid,

  // Master side
  input  wire                          m_axis_tready,
  output wire [M_AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                          m_axis_tvalid
);

  reg int_enbl_reg;
  reg [MISC_WIDTH-1:0] int_misc_reg;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_enbl_reg <= 1'b1;
      if(s_axis_tvalid & s_axis_tready)
      begin
        int_misc_reg <= s_axis_tdata[S_AXIS_TDATA_WIDTH-1:S_AXIS_TDATA_WIDTH-MISC_WIDTH];
      end
    end
  end

  assign s_axis_tready = int_enbl_reg & m_axis_tready;

  assign misc_data = int_misc_reg;

  assign m_axis_tdata = s_axis_tdata[M_AXIS_TDATA_WIDTH-1:0];
  assign m_axis_tvalid = int_enbl_reg & s_axis_tvalid;

endmodule
