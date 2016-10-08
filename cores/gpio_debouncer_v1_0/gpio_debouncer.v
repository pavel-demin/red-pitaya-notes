
`timescale 1 ns / 1 ps

module gpio_debouncer #
(
  parameter integer DATA_WIDTH = 8,
  parameter integer CNTR_WIDTH = 22
)
(
  input  wire                  aclk,

  inout  wire [DATA_WIDTH-1:0] gpio_data,

  output wire [DATA_WIDTH-1:0] deb_data,
  output wire [DATA_WIDTH-1:0] raw_data
);

  reg [DATA_WIDTH-1:0] int_data_reg [2:0];
  reg [CNTR_WIDTH-1:0] int_cntr_reg [DATA_WIDTH-1:0];
  wire [DATA_WIDTH-1:0] int_data_wire;

  genvar j;

  generate
    for(j = 0; j < DATA_WIDTH; j = j + 1)
    begin : GPIO
      IOBUF gpio_iobuf (.O(int_data_wire[j]), .IO(gpio_data[j]), .I(1'b0), .T(1'b1));
      always @(posedge aclk)
      begin
        if(int_data_reg[2][j] == int_data_reg[1][j])
        begin
          int_cntr_reg[j] <= {(CNTR_WIDTH){1'b0}};
        end
        else
        begin
          int_cntr_reg[j] <= int_cntr_reg[j] + 1'b1;
          if(&int_cntr_reg[j]) int_data_reg[2][j] <= ~int_data_reg[2][j];
        end
      end
    end
  endgenerate

  always @(posedge aclk)
  begin
    int_data_reg[0] <= int_data_wire;
    int_data_reg[1] <= int_data_reg[0];
  end

  assign deb_data = int_data_reg[2];
  assign raw_data = int_data_reg[1];

endmodule
