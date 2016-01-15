
`timescale 1 ns / 1 ps

module pulse_generator
(
  input  wire        aclk,
  input  wire        aresetn,

  input  wire [95:0] cfg,

  output wire        out
);

  reg int_out_reg, int_out_next;
  reg [31:0] int_cntr_reg, int_cntr_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_out_reg <= 1'b0;
      int_cntr_reg <= 32'd0;
    end
    else
    begin
      int_out_reg <= int_out_next;
      int_cntr_reg <= int_cntr_next;
    end
  end

  always @*
  begin
    int_out_next = int_out_reg;
    int_cntr_next = int_cntr_reg;

    if(int_cntr_reg == cfg[31:0])
    begin
      int_out_next = 1'b1;
    end

    if(int_cntr_reg == cfg[63:32])
    begin
      int_out_next = 1'b0;
    end

    if(int_cntr_reg < cfg[95:64])
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end
    else
    begin
      int_cntr_next = 32'd0;
    end
  end

  assign out = int_out_reg;

endmodule
