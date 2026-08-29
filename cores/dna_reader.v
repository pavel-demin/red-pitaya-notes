
`timescale 1 ns / 1 ps

module dna_reader
(
  input  wire        aclk,
  input  wire        aresetn,

  output wire [56:0] dna_data
);

  localparam integer CNTR_WIDTH = 16;
  localparam integer DATA_WIDTH = 57;

  reg [CNTR_WIDTH-1:0] int_cntr_reg;
  reg [DATA_WIDTH-1:0] int_data_reg;
  reg int_read_reg, int_shift_reg;

  wire int_data_wire, int_last_wire;

  assign int_last_wire = int_cntr_reg >= 64*DATA_WIDTH;

  DNA_PORT dna_0 (
    .DOUT(int_data_wire),
    .CLK(int_cntr_reg[5]),
    .DIN(1'b0),
    .READ(int_read_reg),
    .SHIFT(int_shift_reg)
  );

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_data_reg <= {(DATA_WIDTH){1'b0}};
      int_read_reg <= 1'b0;
      int_shift_reg <= 1'b0;
    end
    else
    begin
      if(~|int_cntr_reg)
      begin
        int_read_reg <= 1'b1;
      end

      if(&int_cntr_reg[5:0])
      begin
        int_read_reg <= 1'b0;
        int_shift_reg <= 1'b1;
        int_data_reg <= {int_data_reg[DATA_WIDTH-2:0], int_data_wire};
      end

      if(int_last_wire)
      begin
        int_shift_reg <= 1'b0;
      end
      else
      begin
        int_cntr_reg <= int_cntr_reg + 1'b1;
      end
    end
  end

  assign dna_data = int_data_reg;

endmodule
