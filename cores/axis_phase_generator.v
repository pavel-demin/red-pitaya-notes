
`timescale 1 ns / 1 ps

module axis_phase_generator #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer PHASE_WIDTH = 30
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [PHASE_WIDTH-1:0]      cfg_data,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [PHASE_WIDTH-1:0] int_cntr_reg;

  wire int_ready_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(PHASE_WIDTH){1'b0}};
    end
    else if(int_ready_wire)
    begin
      int_cntr_reg <= int_cntr_reg + cfg_data;
    end
  end

  output_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({{(AXIS_TDATA_WIDTH-PHASE_WIDTH){int_cntr_reg[PHASE_WIDTH-1]}}, int_cntr_reg}),
    .in_valid(1'b1), .in_ready(int_ready_wire),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

endmodule
