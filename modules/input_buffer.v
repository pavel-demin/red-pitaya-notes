
`timescale 1 ns / 1 ps

module input_buffer #
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

  reg [DATA_WIDTH-1:0] int_data_reg;
  reg int_ready_reg;

  wire int_valid_wire = ~int_ready_reg | in_valid;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_ready_reg <= 1'b1;
    end
    else
    begin
      if(int_ready_reg)
      begin
        int_data_reg <= in_data;
      end
      if(int_valid_wire)
      begin
        int_ready_reg <= out_ready;
      end
    end
  end

  assign in_ready = int_ready_reg;

  assign out_data = int_ready_reg ? in_data : int_data_reg;
  assign out_valid = int_valid_wire;

endmodule
