
`timescale 1 ns / 1 ps

module axi_bram_writer #
(
  parameter integer AXI_DATA_WIDTH = 32,
  parameter integer AXI_ADDR_WIDTH = 16,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 10
)
(
  // System signals
  input  wire                         aclk,
  input  wire                         aresetn,

  // Slave side
  input  wire [AXI_ADDR_WIDTH-1:0]    s_axi_awaddr,  // AXI4-Lite slave: Write address
  input  wire                         s_axi_awvalid, // AXI4-Lite slave: Write address valid
  output wire                         s_axi_awready, // AXI4-Lite slave: Write address ready
  input  wire [AXI_DATA_WIDTH-1:0]    s_axi_wdata,   // AXI4-Lite slave: Write data
  input  wire [AXI_DATA_WIDTH/8-1:0]  s_axi_wstrb,   // AXI4-Lite slave: Write strobe
  input  wire                         s_axi_wvalid,  // AXI4-Lite slave: Write data valid
  output wire                         s_axi_wready,  // AXI4-Lite slave: Write data ready
  output wire [1:0]                   s_axi_bresp,   // AXI4-Lite slave: Write response
  output wire                         s_axi_bvalid,  // AXI4-Lite slave: Write response valid
  input  wire                         s_axi_bready,  // AXI4-Lite slave: Write response ready
  input  wire [AXI_ADDR_WIDTH-1:0]    s_axi_araddr,  // AXI4-Lite slave: Read address
  input  wire                         s_axi_arvalid, // AXI4-Lite slave: Read address valid
  output wire                         s_axi_arready, // AXI4-Lite slave: Read address ready
  output wire [AXI_DATA_WIDTH-1:0]    s_axi_rdata,   // AXI4-Lite slave: Read data
  output wire [1:0]                   s_axi_rresp,   // AXI4-Lite slave: Read data response
  output wire                         s_axi_rvalid,  // AXI4-Lite slave: Read data valid
  input  wire                         s_axi_rready,  // AXI4-Lite slave: Read data ready

  // BRAM port
  output wire                         b_bram_clk,
  output wire                         b_bram_rst,
  output wire                         b_bram_en,
  output wire [BRAM_DATA_WIDTH/8-1:0] b_bram_we,
  output wire [BRAM_ADDR_WIDTH-1:0]   b_bram_addr,
  output wire [BRAM_DATA_WIDTH-1:0]   b_bram_wdata
);

  function integer clogb2 (input integer value);
    for(clogb2 = 0; value > 0; clogb2 = clogb2 + 1) value = value >> 1;
  endfunction

  localparam integer ADDR_LSB = clogb2(AXI_DATA_WIDTH/8 - 1);

  wire int_awready_wire, int_awvalid_wire;
  wire int_wready_wire, int_wvalid_wire;
  wire int_bready_wire, int_bvalid_wire;
  wire [AXI_ADDR_WIDTH-1:0] int_awaddr_wire;
  wire [AXI_DATA_WIDTH-1:0] int_wdata_wire;
  wire [AXI_DATA_WIDTH/8-1:0] int_wstrb_wire;

  assign int_awready_wire = int_wvalid_wire & int_bready_wire;
  assign int_wready_wire = int_awvalid_wire & int_bready_wire;
  assign int_bvalid_wire = int_awvalid_wire & int_wvalid_wire;

  input_buffer #(
    .DATA_WIDTH(AXI_ADDR_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_ready(s_axi_awready), .in_data(s_axi_awaddr), .in_valid(s_axi_awvalid),
    .out_ready(int_awready_wire), .out_data(int_awaddr_wire), .out_valid(int_awvalid_wire)
  );

  input_buffer #(
    .DATA_WIDTH(AXI_DATA_WIDTH + AXI_DATA_WIDTH/8)
  ) buf_1 (
    .aclk(aclk), .aresetn(aresetn),
    .in_ready(s_axi_wready), .in_data({s_axi_wstrb, s_axi_wdata}), .in_valid(s_axi_wvalid),
    .out_ready(int_wready_wire), .out_data({int_wstrb_wire, int_wdata_wire}), .out_valid(int_wvalid_wire)
  );

  output_buffer #(
    .DATA_WIDTH(0)
  ) buf_2 (
    .aclk(aclk), .aresetn(aresetn),
    .in_ready(int_bready_wire), .in_valid(int_bvalid_wire),
    .out_ready(s_axi_bready), .out_valid(s_axi_bvalid)
  );

  assign s_axi_bresp = 2'd0;

  assign s_axi_arready = 1'b0;
  assign s_axi_rdata = {(AXI_DATA_WIDTH){1'b0}};
  assign s_axi_rresp = 2'd0;
  assign s_axi_rvalid = 1'b0;

  assign b_bram_clk = aclk;
  assign b_bram_rst = ~aresetn;
  assign b_bram_en = int_bvalid_wire;
  assign b_bram_we = int_bvalid_wire ? int_wstrb_wire : {(BRAM_DATA_WIDTH/8){1'b0}};
  assign b_bram_addr = int_awaddr_wire[ADDR_LSB+BRAM_ADDR_WIDTH-1:ADDR_LSB];
  assign b_bram_wdata = int_wdata_wire;

endmodule
