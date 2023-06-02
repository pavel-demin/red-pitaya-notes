
`timescale 1 ns / 1 ps

module pulse_generator #
(
  parameter         CONTINUOUS = "FALSE"
)
(
  input  wire        aclk,
  input  wire        aresetn,

  input  wire [95:0] cfg,

  output wire        dout
);

  reg int_dout_reg, int_dout_next;
  reg [31:0] int_cntr_reg, int_cntr_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_dout_reg <= 1'b0;
      int_cntr_reg <= 32'd0;
    end
    else
    begin
      int_dout_reg <= int_dout_next;
      int_cntr_reg <= int_cntr_next;
    end
  end

  always @*
  begin
    int_dout_next = int_dout_reg;

    if(int_cntr_reg == cfg[31:0])
    begin
      int_dout_next = 1'b1;
    end

    if(int_cntr_reg == cfg[63:32])
    begin
      int_dout_next = 1'b0;
    end
  end

  generate
    if(CONTINUOUS == "TRUE")
    begin : CONTINUE
      always @*
      begin
        int_cntr_next = int_cntr_reg;

        if(int_cntr_reg < cfg[95:64])
        begin
          int_cntr_next = int_cntr_reg + 1'b1;
        end
        else
        begin
          int_cntr_next = 32'd0;
        end
      end
    end
    else
    begin : STOP
      always @*
      begin
        int_cntr_next = int_cntr_reg;

        if(int_cntr_reg < cfg[95:64])
        begin
          int_cntr_next = int_cntr_reg + 1'b1;
        end
      end
    end
  endgenerate

  assign dout = int_dout_reg;

endmodule
