
`timescale 1 ns / 1 ps

module xadc_bram
(
  // XADC inputs
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vp_Vn V_P" *)
  input  wire        Vp_Vn_p,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vp_Vn V_N" *)
  input  wire        Vp_Vn_n,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vaux0 V_P" *)
  input  wire        Vaux0_p,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vaux0 V_N" *)
  input  wire        Vaux0_n,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vaux1 V_P" *)
  input  wire        Vaux1_p,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vaux1 V_N" *)
  input  wire        Vaux1_n,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vaux8 V_P" *)
  input  wire        Vaux8_p,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vaux8 V_N" *)
  input  wire        Vaux8_n,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vaux9 V_P" *)
  input  wire        Vaux9_p,
  (* X_INTERFACE_INFO = "xilinx.com:interface:diff_analog_io:1.0 Vaux9 V_N" *)
  input  wire        Vaux9_n,

  // BRAM port
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram CLK" *)
  input  wire        b_bram_clk,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram RST" *)
  input  wire        b_bram_rst,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram EN" *)
  input  wire        b_bram_en,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram ADDR" *)
  input  wire [4:0]  b_bram_addr,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram DOUT" *)
  output wire [15:0] b_bram_rdata
);

  wire [15:0] int_data_wire;
  wire [4:0] int_addr_wire;
  wire int_eoc_wire, int_we_wire;

  XADC #(
    .INIT_40(16'h0000),
    .INIT_41(16'h8100),
    .INIT_42(16'h1900),
    .INIT_48(16'h0800),
    .INIT_49(16'h0303),
    .INIT_4A(16'h0000),
    .INIT_4B(16'h0000),
    .INIT_4C(16'h0000),
    .INIT_4D(16'h0000),
    .INIT_4E(16'h0000),
    .INIT_4F(16'h0000),
    .INIT_50(16'hb5ed),
    .INIT_51(16'h57e4),
    .INIT_52(16'ha147),
    .INIT_53(16'hca33),
    .INIT_54(16'ha93a),
    .INIT_55(16'h52c6),
    .INIT_56(16'h9555),
    .INIT_57(16'hae4e),
    .INIT_58(16'h5999),
    .INIT_5C(16'h5111),
    .INIT_59(16'h5555),
    .INIT_5D(16'h5111),
    .INIT_5A(16'h9999),
    .INIT_5E(16'h91eb),
    .INIT_5B(16'h6aaa),
    .INIT_5F(16'h6666)
  ) xadc_0 (
    .DCLK(b_bram_clk),
    .RESET(b_bram_rst),
    .DEN(int_eoc_wire),
    .DADDR({2'd0, int_addr_wire}),
    .CHANNEL(int_addr_wire),
    .DO(int_data_wire),
    .DRDY(int_we_wire),
    .EOC(int_eoc_wire),
    .VN(Vp_Vn_n),
    .VP(Vp_Vn_p),
    .VAUXN({6'd0,Vaux9_n,Vaux8_n,6'd0,Vaux1_n,Vaux0_n}),
    .VAUXP({6'd0,Vaux9_p,Vaux8_p,6'd0,Vaux1_p,Vaux0_p})
  );

  xpm_memory_dpdistram #(
    .ADDR_WIDTH_A(5),
    .ADDR_WIDTH_B(5),
    .MEMORY_SIZE(512),
    .BYTE_WRITE_WIDTH_A(16),
    .WRITE_DATA_WIDTH_A(16),
    .READ_DATA_WIDTH_A(16),
    .READ_DATA_WIDTH_B(16),
    .READ_LATENCY_A(1),
    .READ_LATENCY_B(1)
  ) ram_0 (
    .clka(b_bram_clk),
    .rsta(b_bram_rst),
    .rstb(b_bram_rst),
    .addra(int_addr_wire),
    .dina(int_data_wire),
    .ena(int_we_wire),
    .wea(int_we_wire),
    .regcea(1'b0),
    .addrb(b_bram_addr),
    .doutb(b_bram_rdata),
    .enb(b_bram_en),
    .regceb(b_bram_en)
  );

endmodule
