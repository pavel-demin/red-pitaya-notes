
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

  reg [AXIS_TDATA_WIDTH-1:0] int_tdata_reg;
  reg int_tvalid_reg;
  wire [AXIS_TDATA_WIDTH-1:0] int_tdata_wire;

  assign int_tdata_wire = cfg_flag ? cfg_data1 : cfg_data0;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_tvalid_reg <= 1'b0;
    end
    else if(~int_tvalid_reg | m_axis_tready)
    begin
      int_tdata_reg <= int_tdata_wire;
      int_tvalid_reg <= int_tdata_reg != int_tdata_wire;
    end
  end

  assign m_axis_tdata = int_tdata_reg;
  assign m_axis_tvalid = int_tvalid_reg;

endmodule
