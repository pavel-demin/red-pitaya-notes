
`timescale 1 ns / 1 ps

module axis_misc_writer #
(
  parameter integer S_AXIS_TDATA_WIDTH = 32,
  parameter integer M_AXIS_TDATA_WIDTH = 64,
  parameter integer CNTR_WIDTH = 16,
  parameter integer MISC_WIDTH = 16
)
(
  // System signals
  input  wire                          aclk,
  input  wire                          aresetn,

  input  wire [CNTR_WIDTH-1:0]         cfg_data,
  input  wire [MISC_WIDTH-1:0]         misc_data,

  // Slave side
  output wire                          s_axis_tready,
  input  wire [S_AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                          s_axis_tvalid,

  // Master side
  input  wire                          m_axis_tready,
  output wire [M_AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                          m_axis_tvalid
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg [CNTR_WIDTH-1:0] int_data_reg;
  reg int_enbl_reg, int_enbl_next;

  wire int_comp_wire, int_tvalid_wire, int_last_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_data_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_data_reg <= cfg_data;
      int_enbl_reg <= int_enbl_next;
    end
  end

  assign int_comp_wire = int_cntr_reg < int_data_reg;
  assign int_tvalid_wire = int_enbl_reg & s_axis_tvalid;
  assign int_last_wire = ~int_comp_wire;

  always @*
  begin
    int_cntr_next = int_cntr_reg;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg & int_comp_wire)
    begin
      int_enbl_next = 1'b1;
    end

    if(m_axis_tready & int_tvalid_wire & int_comp_wire)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end

    if(m_axis_tready & int_tvalid_wire & int_last_wire)
    begin
      int_cntr_next = {(CNTR_WIDTH){1'b0}};
    end
  end

  assign s_axis_tready = int_enbl_reg & m_axis_tready;

  assign m_axis_tdata = {misc_data, int_cntr_reg, s_axis_tdata};
  assign m_axis_tvalid = int_tvalid_wire;

endmodule
