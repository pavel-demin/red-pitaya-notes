
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

  // Slave side
  output wire                        s_axis_tready,
  input  wire                        s_axis_tvalid,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [PHASE_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_enbl_reg, int_enbl_next;

  wire int_tvalid_wire;

  assign int_tvalid_wire = int_enbl_reg & s_axis_tvalid;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(PHASE_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_enbl_reg <= int_enbl_next;
    end
  end

  always @*
  begin
    int_cntr_next = int_cntr_reg;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg)
    begin
      int_enbl_next = 1'b1;
    end

    if(int_tvalid_wire)
    begin
      int_cntr_next = int_cntr_reg + cfg_data;
    end
  end

  assign s_axis_tready = int_enbl_reg;
  assign m_axis_tdata = {{(AXIS_TDATA_WIDTH-PHASE_WIDTH){int_cntr_reg[PHASE_WIDTH-1]}}, int_cntr_reg};
  assign m_axis_tvalid = int_tvalid_wire;

endmodule
