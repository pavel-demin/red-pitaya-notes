
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
  reg int_tvalid_reg, int_tvalid_next;

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
      int_tvalid_reg <= int_tvalid_next;
    end
  end

  always @*
  begin
    int_tvalid_next = int_tvalid_reg;

    if(int_tdata_reg != cfg_data)
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
