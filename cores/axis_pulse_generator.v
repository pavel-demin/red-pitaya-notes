
`timescale 1 ns / 1 ps

module axis_pulse_generator
(
  input  wire        aclk,
  input  wire        aresetn,

  // Slave side
  input  wire [63:0] s_axis_tdata,
  input  wire        s_axis_tvalid,
  output wire        s_axis_tready,

  // Master side
  output wire [15:0] m_axis_tdata,
  output wire        m_axis_tvalid,
  input  wire        m_axis_tready
);

  reg [31:0] int_cntr_reg;

  wire int_enbl_wire, int_tready_wire;

  assign int_enbl_wire = |int_cntr_reg;
  assign int_tready_wire = ~int_enbl_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= 32'd0;
    end
    else if(int_enbl_wire)
    begin
      int_cntr_reg <= int_cntr_reg - 1'b1;
    end
    else if(s_axis_tvalid)
    begin
      int_cntr_reg <= s_axis_tdata[63:32];
    end
  end

  assign s_axis_tready = int_tready_wire;

  assign m_axis_tdata = s_axis_tdata[15:0];
  assign m_axis_tvalid = int_tready_wire & s_axis_tvalid;

endmodule
