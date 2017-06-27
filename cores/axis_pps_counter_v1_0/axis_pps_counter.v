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

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_enbl_reg, int_enbl_next;
  reg [2:0] int_data_reg;

  wire int_edge_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
      int_data_reg <= 3'd0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_enbl_reg <= int_enbl_next;
      int_data_reg <= {int_data_reg[1:0], pps_data};
    end
  end

  assign int_edge_wire = ~int_data_reg[2] & int_data_reg[1];

  always @*
  begin
    int_cntr_next = int_cntr_reg + 1'b1;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg & int_edge_wire)
    begin
      int_enbl_next = 1'b1;
    end

    if(int_edge_wire)
    begin
      int_cntr_next = {(CNTR_WIDTH){1'b0}};
    end
  end

  assign m_axis_tdata = {{(AXIS_TDATA_WIDTH-CNTR_WIDTH){1'b0}}, int_cntr_reg};
  assign m_axis_tvalid = int_enbl_reg & int_edge_wire;

endmodule
