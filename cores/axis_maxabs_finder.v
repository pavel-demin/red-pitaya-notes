
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

  reg [AXIS_TDATA_WIDTH-1:0] int_max_reg;
  reg [CNTR_WIDTH-1:0] int_cntr_reg;

  wire [AXIS_TDATA_WIDTH-1:0] int_abs_wire, int_max_wire;
  wire int_last_wire;

  assign int_last_wire = int_cntr_reg >= cfg_data;
  assign int_abs_wire = s_axis_tdata[AXIS_TDATA_WIDTH-1] ? ~s_axis_tdata + 1'b1 : s_axis_tdata;
  assign int_max_wire = int_abs_wire > int_max_reg ? int_abs_wire : int_max_reg;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_max_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
    end
    else if(s_axis_tvalid & s_axis_tready)
    begin
      if(int_last_wire)
      begin
        int_max_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
        int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      end
      else
      begin
        int_max_reg <= int_max_wire;
        int_cntr_reg <= int_cntr_reg + 1'b1;
      end
    end
  end

  output_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_max_wire), .in_valid(s_axis_tvalid & int_last_wire), .in_ready(s_axis_tready),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

endmodule
