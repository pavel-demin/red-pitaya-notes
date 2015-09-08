
`timescale 1 ns / 1 ps

module axis_trigger #
(
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,

  output wire                        trig_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid
);

  reg int_data_reg;

  always @(posedge aclk)
  begin
    if(s_axis_tvalid)
    begin
      int_data_reg <= s_axis_tdata[0];
    end
  end

  assign s_axis_tready = 1'b1;
  assign trig_data = s_axis_tvalid & s_axis_tdata[0] & ~int_data_reg;

endmodule
