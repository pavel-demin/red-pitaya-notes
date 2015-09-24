
`timescale 1 ns / 1 ps

module axis_trigger #
(
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,

  input  wire                        cfg_data,

  output wire                        trig_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid
);

  reg int_comp_reg;
  wire int_comp_wire;

  assign int_comp_wire = s_axis_tdata[0] == cfg_data;

  always @(posedge aclk)
  begin
    if(s_axis_tvalid)
    begin
      int_comp_reg <= int_comp_wire;
    end
  end

  assign s_axis_tready = 1'b1;

  assign trig_data = s_axis_tvalid & int_comp_wire & ~int_comp_reg;

endmodule
