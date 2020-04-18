
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

  reg int_tready_reg, int_tready_next;
  reg int_dout_reg, int_dout_next;
  reg int_enbl_reg, int_enbl_next;
  reg [31:0] int_cntr_reg, int_cntr_next;
  reg [127:0] int_data_reg, int_data_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_tready_reg <= 1'b0;
      int_dout_reg <= 1'b0;
      int_enbl_reg <= 1'b0;
      int_cntr_reg <= 32'd0;
      int_data_reg <= 128'd0;
    end
    else
    begin
      int_tready_reg <= int_tready_next;
      int_dout_reg <= int_dout_next;
      int_enbl_reg <= int_enbl_next;
      int_cntr_reg <= int_cntr_next;
      int_data_reg <= int_data_next;
    end
  end

  always @*
  begin
    int_tready_next = int_tready_reg;
    int_dout_next = int_dout_reg;
    int_enbl_next = int_enbl_reg;
    int_cntr_next = int_cntr_reg;
    int_data_next = int_data_reg;

    if(~int_enbl_reg & s_axis_tvalid)
    begin
      int_tready_next = 1'b1;
      int_enbl_next = 1'b1;
      int_cntr_next = 32'd0;
      int_data_next = s_axis_tdata;
    end

    if(int_enbl_reg)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;

      if(int_cntr_reg == 32'd0)
      begin
        int_dout_next = 1'b1;
      end

      if(int_cntr_reg == int_data_reg[31:0])
      begin
        int_dout_next = 1'b0;
      end

      if(int_cntr_reg == int_data_reg[63:32])
      begin
        int_enbl_next = 1'b0;
      end
    end

    if(int_tready_reg)
    begin
      int_tready_next = 1'b0;
    end
  end

  assign s_axis_tready = int_tready_reg;
  assign poff = int_data_reg[95:64];
  assign level = int_data_reg[111:96];
  assign dout = int_dout_reg;

endmodule
