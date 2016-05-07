
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
  reg int_tvalid_reg, int_tvalid_next;
  wire [AXIS_TDATA_WIDTH-1:0] int_tdata_wire;

  assign int_tdata_wire = cfg_flag ? cfg_data1 : cfg_data0;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_tvalid_reg <= 1'b0;
    end
    else
    begin
      int_tdata_reg <= int_tdata_wire;
      int_tvalid_reg <= int_tvalid_next;
    end
  end

  always @*
  begin
    int_tvalid_next = int_tvalid_reg;

    if(int_tdata_reg != int_tdata_wire)
    begin
      int_tvalid_next = 1'b1;
    end

    if(m_axis_tready & int_tvalid_reg)
    begin
      int_tvalid_next = 1'b0;
    end
  end

  assign m_axis_tdata = int_tdata_reg;
  assign m_axis_tvalid = int_tvalid_reg;

endmodule
