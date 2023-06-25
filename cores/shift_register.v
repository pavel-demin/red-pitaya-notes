
`timescale 1 ns / 1 ps

module shift_register #
(
  parameter integer DATA_WIDTH = 8
)
(
  input  wire                  aclk,

  input  wire [DATA_WIDTH-1:0] din,

  output wire [DATA_WIDTH-1:0] dout
);

  reg [DATA_WIDTH-1:0] int_data_reg [1:0];

  always @(posedge aclk)
  begin
    int_data_reg[0] <= din;
    int_data_reg[1] <= int_data_reg[0];
  end

  assign dout = int_data_reg[1];

endmodule
