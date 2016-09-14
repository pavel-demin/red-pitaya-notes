
`timescale 1 ns / 1 ps

module axis_gpio_reader #
(
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,

  inout  wire [AXIS_TDATA_WIDTH-1:0] gpio_data,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg  [AXIS_TDATA_WIDTH-1:0] int_data_reg [1:0];
  wire [AXIS_TDATA_WIDTH-1:0] int_data_wire;

  genvar j;

  generate
  for(j = 0; j < AXIS_TDATA_WIDTH; j = j + 1)
  begin : GPIO
    IOBUF gpio_iobuf (.O(int_data_wire[j]), .IO(gpio_data[j]), .I(1'b0), .T(1'b1));
  end
  endgenerate

  always @(posedge aclk)
  begin
    int_data_reg[0] <= int_data_wire;
    int_data_reg[1] <= int_data_reg[0];
  end

  assign m_axis_tdata = int_data_reg[1];
  assign m_axis_tvalid = 1'b1;

endmodule
