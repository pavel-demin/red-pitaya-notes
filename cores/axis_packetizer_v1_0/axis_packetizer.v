
`timescale 1 ns / 1 ps

module axis_counter #
(
  parameter integer CNTR_WIDTH = 20,
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [CNTR_WIDTH-1:0]       cfg_data,

  // Slave side
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg [AXIS_TDATA_WIDTH-1:0] int_tdata_reg, int_tdata_next;
  reg int_flag_reg, int_flag_next;

  wire int_flag_wire;

  assign int_flag_wire = int_flag_reg & (int_cntr_reg < cfg_data);

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_tdata_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_flag_reg <= 1'b0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_tdata_reg <= int_tdata_next;
      int_flag_reg <= int_flag_next;
    end
  end

  always @*
  begin
    int_cntr_next = int_cntr_reg;

    int_tdata_next = s_axis_tdata;

    if(~int_flag_reg)
    begin
      int_flag_next = 1'b1;
    end

    if(int_flag_wire)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end
  end

  assign m_axis_tdata = int_tdata_reg;
  assign m_axis_tvalid = int_flag_wire;

endmodule
