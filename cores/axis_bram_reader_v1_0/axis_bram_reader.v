
`timescale 1 ns / 1 ps

module axis_bram_reader #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 14
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [BRAM_ADDR_WIDTH-1:0]  cfg_data,

  // Master side
  input wire                         m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,
  output wire                        m_axis_tlast,

  // BRAM port
  output wire                        bram_porta_clk,
  output wire                        bram_porta_rst,
  output wire [BRAM_ADDR_WIDTH-1:0]  bram_porta_addr,
  input  wire [BRAM_DATA_WIDTH-1:0]  bram_porta_rddata,
  output wire                        bram_porta_we
);

  reg [BRAM_ADDR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_enbl_reg, int_enbl_next;

  wire [BRAM_ADDR_WIDTH-1:0] sum_cntr_wire;
  wire int_comp_wire, int_tlast_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_enbl_reg <= int_enbl_next;
    end
  end

  assign sum_cntr_wire = int_cntr_reg + 1'b1;
  assign int_comp_wire = int_cntr_reg < cfg_data;
  assign int_tlast_wire = int_cntr_reg == cfg_data;

  always @*
  begin
    int_cntr_next = int_cntr_reg;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg & int_comp_wire)
    begin
      int_enbl_next = 1'b1;
    end

    if(m_axis_tready & int_enbl_reg & int_comp_wire)
    begin
      int_cntr_next = sum_cntr_wire;
    end

    if(m_axis_tready & int_enbl_reg & int_tlast_wire)
    begin
      int_enbl_next = 1'b0;
    end
  end

  assign m_axis_tdata = bram_porta_rddata;
  assign m_axis_tvalid = int_enbl_reg;
  assign m_axis_tlast = int_enbl_reg & int_tlast_wire;

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_addr = m_axis_tready & int_enbl_reg ? sum_cntr_wire : int_cntr_reg;
  assign bram_porta_we = 1'b0;

endmodule
