`timescale 1 ns / 1 ps

module axis_pps_counter #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input wire                         pps_data,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg;
  reg int_enbl_reg;
  reg [1:0] int_data_reg;

  wire int_edge_wire, int_pps_wire;

  xpm_cdc_single #(
    .DEST_SYNC_FF(4),
    .INIT_SYNC_FF(0),
    .SRC_INPUT_REG(0),
    .SIM_ASSERT_CHK(0)
  ) cdc_0 (
    .src_in(pps_data),
    .src_clk(),
    .dest_out(int_pps_wire),
    .dest_clk(aclk)
  );

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
      int_data_reg <= 2'd0;
    end
    else
    begin
      int_cntr_reg <= int_edge_wire ? {(CNTR_WIDTH){1'b0}} : int_cntr_reg + 1'b1;
      int_enbl_reg <= int_edge_wire ? 1'b1 : int_enbl_reg;
      int_data_reg <= {int_data_reg[0], ~int_pps_wire};
    end
  end

  assign int_edge_wire = int_data_reg[1] & ~int_data_reg[0];

  assign m_axis_tdata = {{(AXIS_TDATA_WIDTH-CNTR_WIDTH){1'b0}}, int_cntr_reg};
  assign m_axis_tvalid = int_enbl_reg & int_edge_wire;

endmodule
