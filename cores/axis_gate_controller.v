
`timescale 1 ns / 1 ps

module axis_gate_controller
(
  input  wire         aclk,
  input  wire         aresetn,

  // Slave side
  output wire         s_axis_tready,
  input  wire [127:0] s_axis_tdata,
  input  wire         s_axis_tvalid,

  output wire [31:0]  poff,
  output wire [15:0]  level,
  output wire         dout
);

  reg [63:0] int_cntr_reg;
  reg [48:0] int_data_reg;
  reg [31:0] int_poff_reg;
  reg [15:0] int_level_reg;
  reg int_dout_reg;

  wire [48:0] int_data_wire;
  wire int_enbl_wire;

  assign int_data_wire = int_enbl_wire ? int_data_reg : s_axis_tdata[112:64];
  assign int_enbl_wire = |int_cntr_reg;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= 64'd0;
      int_data_reg <= 49'd0;
      int_poff_reg <= 32'd0;
      int_level_reg <= 16'd0;
      int_dout_reg <= 1'b0;
    end
    else
    begin
      if(int_enbl_wire)
      begin
        int_cntr_reg <= int_cntr_reg - 1'b1;
      end
      else if(s_axis_tvalid)
      begin
        int_cntr_reg <= s_axis_tdata[63:0];
        int_data_reg <= s_axis_tdata[112:64];
      end
      int_poff_reg <= int_data_wire[31:0];
      int_level_reg <= int_data_wire[47:32];
      int_dout_reg <= int_data_wire[48] & (int_enbl_wire | s_axis_tvalid);
    end
  end

  assign s_axis_tready = ~int_enbl_wire & aresetn;

  assign poff = int_poff_reg;
  assign level = int_level_reg;
  assign dout = int_dout_reg;

endmodule
