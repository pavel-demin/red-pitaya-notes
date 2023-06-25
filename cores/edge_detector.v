
`timescale 1 ns / 1 ps

module edge_detector
(
  input  wire aclk,

  input  wire din,

  output wire dout
);

  reg [1:0] int_data_reg;

  always @(posedge aclk)
  begin
    int_data_reg <= {int_data_reg[0], din};
  end

  assign dout = ^int_data_reg;

endmodule
