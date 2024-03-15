
`timescale 1 ns / 1 ps

module dds #
(
  parameter NEGATIVE_SINE = "FALSE"
)
(
  input  wire        aclk,
  input  wire        aresetn,

  input  wire [31:0] pinc,

  output wire [47:0] dout
);

  reg [31:0] int_cntr_reg;
  reg [23:0] int_cos_reg, int_sin_reg;
  reg [10:0] int_addr_reg;
  reg [1:0] int_sign_reg [2:0];

  wire [47:0] int_p_wire [2:0];
  wire [31:0] int_cntr_wire;
  wire [29:0] int_cos_wire, int_sin_wire;
  wire [22:0] int_lut_wire [1:0];
  wire [17:0] int_corr_wire;
  wire [2:0] int_pbd_wire;

  assign int_cos_wire = int_sign_reg[2][0] ? {7'h7f, -int_lut_wire[0]} : {7'h00, int_lut_wire[0]};
  assign int_sin_wire = int_sign_reg[2][1] ? {7'h7f, -int_lut_wire[1]} : {7'h00, int_lut_wire[1]};

  assign int_corr_wire = {int_p_wire[0][34:18], int_p_wire[0][17] | int_pbd_wire[0]};

  generate
    if(NEGATIVE_SINE == "TRUE")
    begin : NEGATIVE
      assign int_cntr_wire = int_cntr_reg - pinc;
    end
    else
    begin : POSITIVE
      assign int_cntr_wire = int_cntr_reg + pinc;
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= 32'd0;
      int_cos_reg <= 24'd0;
      int_sin_reg <= 24'd0;
      int_addr_reg <= 11'd0;
      int_sign_reg[0] <= 2'd0;
      int_sign_reg[1] <= 2'd0;
      int_sign_reg[2] <= 2'd0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_wire;
      int_cos_reg <= int_cos_wire[23:0];
      int_sin_reg <= int_sin_wire[23:0];
      int_addr_reg <= int_cntr_reg[29:19];
      int_sign_reg[0] <= {int_cntr_reg[31], int_cntr_reg[30]};
      int_sign_reg[1] <= {int_sign_reg[0][1], int_sign_reg[0][1] ^ int_sign_reg[0][0]};
      int_sign_reg[2] <= int_sign_reg[1];
    end
  end

  xpm_memory_dprom #(
    .MEMORY_PRIMITIVE("block"),
    .MEMORY_SIZE(47104),
    .ADDR_WIDTH_A(11),
    .ADDR_WIDTH_B(11),
    .READ_DATA_WIDTH_A(23),
    .READ_DATA_WIDTH_B(23),
    .READ_LATENCY_A(2),
    .READ_LATENCY_B(2),
    .MEMORY_INIT_PARAM(""),
    .MEMORY_INIT_FILE("dds.mem")
  ) rom_0 (
    .clka(aclk),
    .clkb(aclk),
    .rsta(1'b0),
    .rstb(1'b0),
    .ena(1'b1),
    .enb(1'b1),
    .regcea(1'b1),
    .regceb(1'b1),
    .addra(int_sign_reg[0][0] ? ~int_addr_reg : int_addr_reg),
    .addrb(int_sign_reg[0][0] ? int_addr_reg : ~int_addr_reg),
    .douta(int_lut_wire[0]),
    .doutb(int_lut_wire[1])
  );

  DSP48E1 #(
    .ALUMODEREG(0), .CARRYINSELREG(0), .INMODEREG(0), .OPMODEREG(0),
    .BREG(0), .BCASCREG(0), .CREG(0), .CARRYINREG(0),
    .USE_PATTERN_DETECT("PATDET"), .PATTERN(48'hfffffffe0000), .MASK(48'hfffffffe0000)
  ) dsp_0 (
    .CLK(aclk),
    .RSTA(1'b0), .RSTM(1'b0), .RSTP(1'b0),
    .CEA2(1'b1), .CED(1'b0), .CEAD(1'b0), .CEM(1'b1), .CEP(1'b1),
    .ALUMODE(4'b0000), .CARRYINSEL(3'b000), .INMODE(5'b00000), .OPMODE(7'b0110101),
    .A({{(12){~int_cntr_reg[18]}}, int_cntr_reg[17:0]}),
    .B(18'd3217),
    .C(48'hffff),
    .CARRYIN(1'b0),
    .PATTERNBDETECT(int_pbd_wire[0]),
    .P(int_p_wire[0])
  );

  DSP48E1 #(
    .ALUMODEREG(0), .CARRYINSELREG(0), .INMODEREG(0), .OPMODEREG(0),
    .CARRYINREG(0),
    .USE_PATTERN_DETECT("PATDET"), .PATTERN(48'hffffff000000), .MASK(48'hffffff000000)
  ) dsp_1 (
    .CLK(aclk),
    .RSTA(1'b0), .RSTB(1'b0), .RSTC(1'b0), .RSTM(1'b0), .RSTP(1'b0),
    .CEA2(1'b1), .CEB2(1'b1), .CEC(1'b1), .CED(1'b0), .CEAD(1'b0), .CEM(1'b1), .CEP(1'b1),
    .ALUMODE(4'b0011), .CARRYINSEL(3'b000), .INMODE(5'b00000), .OPMODE(7'b0110101),
    .A(int_sin_wire),
    .B(int_corr_wire),
    .C({int_cos_reg, 24'h7fffff}),
    .CARRYIN(1'b0),
    .PATTERNBDETECT(int_pbd_wire[1]),
    .P(int_p_wire[1])
  );

  DSP48E1 #(
    .ALUMODEREG(0), .CARRYINSELREG(0), .INMODEREG(0), .OPMODEREG(0),
    .CARRYINREG(0),
    .USE_PATTERN_DETECT("PATDET"), .PATTERN(48'hffffff000000), .MASK(48'hffffff000000)
  ) dsp_2 (
    .CLK(aclk),
    .RSTA(1'b0), .RSTB(1'b0), .RSTC(1'b0), .RSTM(1'b0), .RSTP(1'b0),
    .CEA2(1'b1), .CEB2(1'b1), .CEC(1'b1), .CED(1'b0), .CEAD(1'b0), .CEM(1'b1), .CEP(1'b1),
    .ALUMODE(4'b0000), .CARRYINSEL(3'b000), .INMODE(5'b00000), .OPMODE(7'b0110101),
    .A(int_cos_wire),
    .B(int_corr_wire),
    .C({int_sin_reg, 24'h7fffff}),
    .CARRYIN(1'b0),
    .PATTERNBDETECT(int_pbd_wire[2]),
    .P(int_p_wire[2])
  );

  assign dout = {int_p_wire[2][47:25], int_p_wire[2][24] | int_pbd_wire[2], int_p_wire[1][47:25], int_p_wire[1][24] | int_pbd_wire[1]};

endmodule
