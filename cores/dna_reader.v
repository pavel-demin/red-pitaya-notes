
`timescale 1 ns / 1 ps

module dna_reader
(
  input  wire        aclk,
  input  wire        aresetn,

  output wire [56:0] dna_data
);

  localparam integer CNTR_WIDTH = 16;
  localparam integer DATA_WIDTH = 57;

  reg int_enbl_reg, int_enbl_next;
  reg int_read_reg, int_read_next;
  reg int_shift_reg, int_shift_next;
  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg [DATA_WIDTH-1:0] int_data_reg, int_data_next;

  wire int_comp_wire, int_data_wire;

  assign int_comp_wire = int_cntr_reg < 64*DATA_WIDTH;

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
      int_enbl_reg <= 1'b0;
      int_read_reg <= 1'b0;
      int_shift_reg <= 1'b0;
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_data_reg <= {(DATA_WIDTH){1'b0}};
    end
    else
    begin
      int_enbl_reg <= int_enbl_next;
      int_read_reg <= int_read_next;
      int_shift_reg <= int_shift_next;
      int_cntr_reg <= int_cntr_next;
      int_data_reg <= int_data_next;
    end
  end

  always @*
  begin
    int_enbl_next = int_enbl_reg;
    int_read_next = int_read_reg;
    int_shift_next = int_shift_reg;
    int_cntr_next = int_cntr_reg;
    int_data_next = int_data_reg;

    if(~int_enbl_reg & int_comp_wire)
    begin
      int_enbl_next = 1'b1;
      int_read_next = 1'b1;
    end

    if(int_enbl_reg)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end

    if(&int_cntr_reg[5:0])
    begin
      int_read_next = 1'b0;
      int_shift_next = 1'b1;
      int_data_next = {int_data_reg[DATA_WIDTH-2:0], int_data_wire};
    end

    if(~int_comp_wire)
    begin
      int_enbl_next = 1'b0;
      int_shift_next = 1'b0;
    end
  end

  assign dna_data = int_data_reg;

endmodule
