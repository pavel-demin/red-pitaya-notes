
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

  reg [AXIS_TDATA_WIDTH-1:0] int_tdata_reg;
  reg int_tvalid_reg;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_tvalid_reg <= 1'b0;
    end
    else
    begin
      int_tdata_reg <= cfg_data;
      int_tvalid_reg <= (int_tdata_reg != cfg_data) | (int_tvalid_reg & ~m_axis_tready);
    end
  end

  assign m_axis_tdata = int_tdata_reg;
  assign m_axis_tvalid = int_tvalid_reg;

endmodule
