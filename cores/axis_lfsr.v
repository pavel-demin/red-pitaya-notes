
`timescale 1 ns / 1 ps

module axis_lfsr #
(
  parameter integer AXIS_TDATA_WIDTH = 64,
  parameter         HAS_TREADY = "FALSE"
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [AXIS_TDATA_WIDTH-1:0] int_lfsr_reg;

  wire int_ready_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_lfsr_reg <= 64'h85fac8a1658d6f0d;
    end
    else if(HAS_TREADY != "TRUE" || int_ready_wire)
    begin
      int_lfsr_reg <= {int_lfsr_reg[62:0], int_lfsr_reg[62] ~^ int_lfsr_reg[61]};
    end
  end

  output_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_lfsr_reg), .in_valid(1'b1), .in_ready(int_ready_wire),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

endmodule
