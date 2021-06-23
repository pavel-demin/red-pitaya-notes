
`timescale 1 ns / 1 ps

module axis_maxabs_finder #
(
  parameter integer AXIS_TDATA_WIDTH = 16,
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

  reg [AXIS_TDATA_WIDTH-1:0] int_max_reg, int_max_next;
  reg [AXIS_TDATA_WIDTH-1:0] int_tdata_reg, int_tdata_next;
  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_tvalid_reg, int_tvalid_next;

  wire [AXIS_TDATA_WIDTH-1:0] int_abs_wire;
  wire int_comp_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_max_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_tvalid_reg <= 1'b0;
    end
    else
    begin
      int_max_reg <= int_max_next;
      int_tdata_reg <= int_tdata_next;
      int_cntr_reg <= int_cntr_next;
      int_tvalid_reg <= int_tvalid_next;
    end
  end

  assign int_comp_wire = int_cntr_reg < cfg_data;
  assign int_abs_wire = s_axis_tdata[AXIS_TDATA_WIDTH-1] ? ~s_axis_tdata : s_axis_tdata;

  always @*
  begin
    int_max_next = int_max_reg;
    int_tdata_next = int_tdata_reg;
    int_cntr_next = int_cntr_reg;
    int_tvalid_next = int_tvalid_reg;

    if(s_axis_tvalid & int_comp_wire)
    begin
      int_max_next = int_abs_wire > int_max_reg ? int_abs_wire : int_max_reg;
      int_cntr_next = int_cntr_reg + 1'b1;
    end

    if(s_axis_tvalid & ~int_comp_wire)
    begin
      int_max_next = {(AXIS_TDATA_WIDTH){1'b0}};
      int_tdata_next = int_max_reg;
      int_cntr_next = {(CNTR_WIDTH){1'b0}};
      int_tvalid_next = 1'b1;
    end

    if(m_axis_tready & int_tvalid_reg)
    begin
      int_tvalid_next = 1'b0;
    end
  end

  assign s_axis_tready = 1'b1;
  assign m_axis_tdata = int_tdata_reg;
  assign m_axis_tvalid = int_tvalid_reg;

endmodule
