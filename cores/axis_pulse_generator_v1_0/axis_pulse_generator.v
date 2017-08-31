
`timescale 1 ns / 1 ps

module axis_pulse_generator
(
  input  wire         aclk,
  input  wire         aresetn,

  // Slave side
  output wire         s_axis_tready,
  input  wire [127:0] s_axis_tdata,
  input  wire         s_axis_tvalid,

  output wire [31:0]  poff,
  output wire         sync,
  output wire         dout
);

  reg int_tready_reg, int_tready_next;
  reg int_sync_reg, int_sync_next;
  reg int_dout_reg, int_dout_next;
  reg [31:0] int_cntr_reg, int_cntr_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_tready_reg <= 1'b0;
      int_sync_reg <= 1'b0;
      int_dout_reg <= 1'b0;
      int_cntr_reg <= 32'd0;
    end
    else
    begin
      int_tready_reg <= int_tready_next;
      int_sync_reg <= int_sync_next;
      int_dout_reg <= int_dout_next;
      int_cntr_reg <= int_cntr_next;
    end
  end

  always @*
  begin
    int_tready_next = int_tready_reg;
    int_sync_next = int_sync_reg;
    int_dout_next = int_dout_reg;
    int_cntr_next = int_cntr_reg;

    if(s_axis_tvalid)
    begin
      if(int_cntr_reg == s_axis_tdata[31:0])
      begin
        int_sync_next = 1'b1;
        int_dout_next = 1'b1;
      end

      if(int_cntr_reg == s_axis_tdata[63:32])
      begin
        int_dout_next = 1'b0;
      end

      if(int_cntr_reg < s_axis_tdata[95:64])
      begin
        int_cntr_next = int_cntr_reg + 1'b1;
      end
      else
      begin
        int_tready_next = 1'b1;
      end

      if(int_tready_reg)
      begin
        int_cntr_next = 32'd0;
        int_tready_next = 1'b0;
      end

      if(int_sync_reg)
      begin
        int_sync_next = 1'b0;
      end
    end
  end

  assign s_axis_tready = int_tready_reg;
  assign poff = s_axis_tdata[127:96];
  assign sync = int_sync_reg;
  assign dout = int_dout_reg;

endmodule
