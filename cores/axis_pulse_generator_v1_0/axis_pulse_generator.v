
`timescale 1 ns / 1 ps

module axis_pulse_generator
(
  input  wire        aclk,
  input  wire        aresetn,

  // Slave side
  output wire        s_axis_tready,
  input  wire [63:0] s_axis_tdata,
  input  wire        s_axis_tvalid,

  // Master side
  input  wire        m_axis_tready,
  output wire [15:0] m_axis_tdata,
  output wire        m_axis_tvalid
);

  reg [31:0] int_cntr_reg, int_cntr_next;

  wire int_enbl_wire, int_tready_wire, int_tvalid_wire;

  assign int_enbl_wire = |int_cntr_reg;
  assign int_tready_wire = ~int_enbl_wire;
  assign int_tvalid_wire = int_tready_wire & s_axis_tvalid;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= 32'd0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
    end
  end

  always @*
  begin
    int_cntr_next = int_cntr_reg;

    if(int_tvalid_wire)
    begin
      int_cntr_next = s_axis_tdata[63:32];
    end

    if(int_enbl_wire)
    begin
      int_cntr_next = int_cntr_reg - 1'b1;
    end
  end

  assign s_axis_tready = int_tready_wire;
  assign m_axis_tdata = s_axis_tdata[15:0];
  assign m_axis_tvalid = int_tvalid_wire;

endmodule
