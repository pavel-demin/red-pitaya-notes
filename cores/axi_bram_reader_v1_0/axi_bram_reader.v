
`timescale 1 ns / 1 ps

module axi_bram_reader #
(
  parameter integer AXI_DATA_WIDTH = 32,
  parameter integer AXI_ADDR_WIDTH = 16,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 10
)
(
  // System signals
  input  wire                       aclk,
  input  wire                       aresetn,

  // Slave side
  input  wire [AXI_ADDR_WIDTH-1:0]  s_axi_awaddr,  // AXI4-Lite slave: Write address
  input  wire                       s_axi_awvalid, // AXI4-Lite slave: Write address valid
  output wire                       s_axi_awready, // AXI4-Lite slave: Write address ready
  input  wire [AXI_DATA_WIDTH-1:0]  s_axi_wdata,   // AXI4-Lite slave: Write data
  input  wire                       s_axi_wvalid,  // AXI4-Lite slave: Write data valid
  output wire                       s_axi_wready,  // AXI4-Lite slave: Write data ready
  output wire [1:0]                 s_axi_bresp,   // AXI4-Lite slave: Write response
  output wire                       s_axi_bvalid,  // AXI4-Lite slave: Write response valid
  input  wire                       s_axi_bready,  // AXI4-Lite slave: Write response ready
  input  wire [AXI_ADDR_WIDTH-1:0]  s_axi_araddr,  // AXI4-Lite slave: Read address
  input  wire                       s_axi_arvalid, // AXI4-Lite slave: Read address valid
  output wire                       s_axi_arready, // AXI4-Lite slave: Read address ready
  output wire [AXI_DATA_WIDTH-1:0]  s_axi_rdata,   // AXI4-Lite slave: Read data
  output wire [1:0]                 s_axi_rresp,   // AXI4-Lite slave: Read data response
  output wire                       s_axi_rvalid,  // AXI4-Lite slave: Read data valid
  input  wire                       s_axi_rready,  // AXI4-Lite slave: Read data ready

  // BRAM port
  output wire                       bram_porta_clk,
  output wire                       bram_porta_rst,
  output wire                       bram_porta_en,
  output wire [BRAM_ADDR_WIDTH-1:0] bram_porta_addr,
  input  wire [BRAM_DATA_WIDTH-1:0] bram_porta_rddata
);

  function integer clogb2 (input integer value);
    for(clogb2 = 0; value > 0; clogb2 = clogb2 + 1) value = value >> 1;
  endfunction

  localparam integer ADDR_LSB = clogb2(AXI_DATA_WIDTH/8 - 1);

  wire int_arvalid_wire, int_rready_wire;
  wire [AXI_ADDR_WIDTH-1:0] int_araddr_wire;
  wire [AXI_ADDR_WIDTH-1:0] int_addr_wire;

  input_buffer #(
    .DATA_WIDTH(AXI_ADDR_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_ready(s_axi_arready), .in_data(s_axi_araddr), .in_valid(s_axi_arvalid),
    .out_ready(int_rready_wire), .out_data(int_araddr_wire), .out_valid(int_arvalid_wire)
  );

  output_buffer #(
    .DATA_WIDTH(0)
  ) buf_1 (
    .aclk(aclk), .aresetn(aresetn),
    .in_ready(int_rready_wire), .in_valid(int_arvalid_wire),
    .out_ready(s_axi_rready), .out_valid(s_axi_rvalid)
  );

  assign s_axi_awready = 1'b0;
  assign s_axi_wready = 1'b0;
  assign s_axi_bresp = 2'd0;
  assign s_axi_bvalid = 1'b0;

  assign s_axi_rdata = bram_porta_rddata;
  assign s_axi_rresp = 2'd0;

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_en = int_rready_wire;
  assign bram_porta_addr = int_araddr_wire[ADDR_LSB+BRAM_ADDR_WIDTH-1:ADDR_LSB];

endmodule
