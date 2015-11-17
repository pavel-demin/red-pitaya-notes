
`timescale 1 ns / 1 ps

module axis_interpolator #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [CNTR_WIDTH-1:0]       cfg_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [AXIS_TDATA_WIDTH-1:0] int_tdata_reg, int_tdata_next;
  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_enbl_reg, int_enbl_next;

  wire int_comp_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_tdata_reg <= int_tdata_next;
      int_cntr_reg <= int_cntr_next;
      int_enbl_reg <= int_enbl_next;
    end
  end

  assign int_comp_wire = int_cntr_reg > 0;

  always @*
  begin
    int_tdata_next = int_tdata_reg;
    int_cntr_next = int_cntr_reg;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg)
    begin
      int_enbl_next = 1'b1;
    end

    if(int_enbl_reg & s_axis_tvalid & ~int_comp_wire)
    begin
      int_tdata_next = s_axis_tdata;
      int_cntr_next = cfg_data;
    end

    if(int_enbl_reg & m_axis_tready & int_comp_wire)
    begin
      int_cntr_next = int_cntr_reg - 1'b1;
    end
  end

  assign s_axis_tready = int_enbl_reg & ~int_comp_wire;
  assign m_axis_tdata = int_comp_wire ? int_tdata_reg : s_axis_tdata;
  assign m_axis_tvalid = int_comp_wire ? 1'b1 : s_axis_tvalid;

endmodule
