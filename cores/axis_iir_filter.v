
`timescale 1 ns / 1 ps

module axis_iir_filter
(
  // System signals
  input  wire        aclk,
  input  wire        aresetn,

  input  wire [79:0] cfg_data,

  // Slave side
  input  wire [15:0] s_axis_tdata,
  input  wire        s_axis_tvalid,
  output wire        s_axis_tready,

  // Master side
  output wire [15:0] m_axis_tdata,
  output wire        m_axis_tvalid,
  input  wire        m_axis_tready
);

  wire [47:0] int_p_wire [2:0];

  wire [15:0] int_data_wire;
  wire [5:0] int_valid_wire, int_ready_wire;

  genvar j;

  assign int_valid_wire[0] = s_axis_tvalid;
  assign s_axis_tready = int_ready_wire[0];

  assign int_data_wire = $signed(int_p_wire[2][47:25]) < $signed(cfg_data[63:48]) ? cfg_data[63:48] : $signed(int_p_wire[2][47:25]) > $signed(cfg_data[79:64]) ? cfg_data[79:64] : int_p_wire[2][40:25];

  generate
    for(j = 0; j < 5; j = j + 1)
    begin : BUFFERS
      output_buffer #(
        .DATA_WIDTH(0)
      ) output_buffer_inst (
        .aclk(aclk), .aresetn(aresetn),
        .in_valid(int_valid_wire[j]), .in_ready(int_ready_wire[j]),
        .out_valid(int_valid_wire[j+1]), .out_ready(int_ready_wire[j+1])
      );
    end
  endgenerate

  inout_buffer #(
    .DATA_WIDTH(16)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_data_wire), .in_valid(int_valid_wire[5]), .in_ready(int_ready_wire[5]),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

  DSP48E1 #(
    .ALUMODEREG(0), .CARRYINSELREG(0), .INMODEREG(0), .OPMODEREG(0),
    .CREG(0), .CARRYINREG(0), .MREG(1), .PREG(1)
  ) dsp_0 (
    .CLK(aclk),
    .RSTA(1'b0), .RSTB(1'b0),
    .RSTM(1'b0), .RSTP(1'b0),
    .CEA2(int_ready_wire[0]), .CEB2(int_ready_wire[0]),
    .CED(1'b0), .CEAD(1'b0),
    .CEM(int_ready_wire[1]), .CEP(int_ready_wire[2]),
    .ALUMODE(4'b0000), .CARRYINSEL(3'b000), .INMODE(5'b00000), .OPMODE(7'b0000101),
    .A({{(5){s_axis_tdata[15]}}, s_axis_tdata, 9'd0}),
    .B(cfg_data[15:0]),
    .CARRYIN(1'b0),
    .P(int_p_wire[0])
  );

  DSP48E1 #(
    .ALUMODEREG(0), .CARRYINSELREG(0), .INMODEREG(0), .OPMODEREG(0),
    .AREG(0), .ACASCREG(0), .BREG(0), .BCASCREG(0),
    .CREG(0), .CARRYINREG(0), .MREG(0), .PREG(1)
  ) dsp_1 (
    .CLK(aclk),
    .RSTP(1'b0),
    .CED(1'b0), .CEAD(1'b0),
    .CEP(int_ready_wire[3]),
    .ALUMODE(4'b0000), .CARRYINSEL(3'b000), .INMODE(5'b00000), .OPMODE(7'b0110101),
    .A(int_p_wire[1][45:16]),
    .B(cfg_data[31:16]),
    .C(int_p_wire[0]),
    .CARRYIN(1'b0),
    .P(int_p_wire[1])
  );

  DSP48E1 #(
    .ALUMODEREG(0), .CARRYINSELREG(0), .INMODEREG(0), .OPMODEREG(0),
    .AREG(0), .ACASCREG(0), .BREG(0), .BCASCREG(0),
    .CREG(0), .CARRYINREG(0), .MREG(0), .PREG(1)
  ) dsp_2 (
    .CLK(aclk),
    .RSTP(1'b0),
    .CED(1'b0), .CEAD(1'b0),
    .CEP(int_ready_wire[4]),
    .ALUMODE(4'b0000), .CARRYINSEL(3'b000), .INMODE(5'b00000), .OPMODE(7'b0110101),
    .A(int_p_wire[2][45:16]),
    .B(cfg_data[47:32]),
    .C(int_p_wire[1]),
    .CARRYIN(1'b0),
    .P(int_p_wire[2])
  );

endmodule
