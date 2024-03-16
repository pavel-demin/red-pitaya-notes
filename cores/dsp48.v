
`timescale 1 ns / 1 ps

module dsp48 #
(
  parameter integer A_WIDTH = 24,
  parameter integer B_WIDTH = 16,
  parameter integer P_WIDTH = 24
)
(
  input  wire               CLK,

  input  wire [A_WIDTH-1:0] A,
  input  wire [B_WIDTH-1:0] B,

  output wire [P_WIDTH-1:0] P
);

  localparam integer SHIFT = A_WIDTH + B_WIDTH - P_WIDTH - 1;
  localparam integer ONES = SHIFT - 1;

  wire [47:0] int_p_wire;
  wire int_pbd_wire;

  DSP48E1 #(
    .ALUMODEREG(0), .CARRYINSELREG(0), .INMODEREG(0), .OPMODEREG(0),
    .CREG(0), .CARRYINREG(0), .MREG(1), .PREG(1),
    .USE_PATTERN_DETECT("PATDET"), .SEL_MASK("ROUNDING_MODE1")
  ) dsp_0 (
    .CLK(CLK),
    .RSTA(1'b0), .RSTB(1'b0), .RSTM(1'b0), .RSTP(1'b0),
    .CEA2(1'b1), .CEB2(1'b1), .CED(1'b0), .CEAD(1'b0), .CEM(1'b1), .CEP(1'b1),
    .ALUMODE(4'b0000), .CARRYINSEL(3'b000), .INMODE(5'b00000), .OPMODE(7'b0110101),
    .A({{(30-A_WIDTH){A[A_WIDTH-1]}}, A}),
    .B({{(18-B_WIDTH){B[B_WIDTH-1]}}, B}),
    .C({{(48-ONES){1'b0}}, {(ONES){1'b1}}}),
    .CARRYIN(1'b0),
    .PATTERNBDETECT(int_pbd_wire),
    .P(int_p_wire)
  );

  assign P = {int_p_wire[SHIFT+P_WIDTH-1:SHIFT+1], int_p_wire[SHIFT] | int_pbd_wire};

endmodule
