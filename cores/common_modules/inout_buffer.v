
`timescale 1 ns / 1 ps

module inout_buffer #
(
  parameter integer DATA_WIDTH = 32
)
(
  input  wire                  aclk,
  input  wire                  aresetn,

  input  wire [DATA_WIDTH-1:0] in_data,
  input  wire                  in_valid,
  output wire                  in_ready,

  output wire [DATA_WIDTH-1:0] out_data,
  output wire                  out_valid,
  input  wire                  out_ready
);

  wire [DATA_WIDTH-1:0] int_data_wire;
  wire int_valid_wire, int_ready_wire;

  input_buffer #(
    .DATA_WIDTH(DATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(in_data), .in_valid(in_valid), .in_ready(in_ready),
    .out_data(int_data_wire), .out_valid(int_valid_wire), .out_ready(int_ready_wire)
  );

  output_buffer #(
    .DATA_WIDTH(DATA_WIDTH)
  ) buf_1 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_data_wire), .in_valid(int_valid_wire), .in_ready(int_ready_wire),
    .out_data(out_data), .out_valid(out_valid), .out_ready(out_ready)
  );

endmodule
