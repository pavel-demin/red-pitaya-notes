
`timescale 1 ns / 1 ps

module axis_counter #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [CNTR_WIDTH-1:0]       cfg_data,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_enbl_reg, int_enbl_next;

  wire int_comp_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_enbl_reg <= int_enbl_next;
    end
  end

  assign int_comp_wire = int_cntr_reg < cfg_data;

  always @*
  begin
    int_cntr_next = int_cntr_reg;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg & int_comp_wire)
    begin
      int_enbl_next = 1'b1;
    end

    if(int_enbl_reg & int_comp_wire)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end

    if(int_enbl_reg & ~int_comp_wire)
    begin
      int_enbl_next = 1'b0;
    end
  end

  assign m_axis_tdata = {{(AXIS_TDATA_WIDTH-CNTR_WIDTH){1'b0}}, int_cntr_reg};
  assign m_axis_tvalid = int_enbl_reg;

endmodule
