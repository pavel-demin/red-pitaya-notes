
`timescale 1 ns / 1 ps

module axi_axis_writer #
(
  parameter integer AXI_DATA_WIDTH = 32,
  parameter integer AXI_ADDR_WIDTH = 16
)
(
  // System signals
  input  wire                      aclk,
  input  wire                      aresetn,

  // Slave side
  input  wire [AXI_ADDR_WIDTH-1:0] s_axi_awaddr,  // AXI4-Lite slave: Write address
  input  wire                      s_axi_awvalid, // AXI4-Lite slave: Write address valid
  output wire                      s_axi_awready, // AXI4-Lite slave: Write address ready
  input  wire [AXI_DATA_WIDTH-1:0] s_axi_wdata,   // AXI4-Lite slave: Write data
  input  wire                      s_axi_wvalid,  // AXI4-Lite slave: Write data valid
  output wire                      s_axi_wready,  // AXI4-Lite slave: Write data ready
  output wire [1:0]                s_axi_bresp,   // AXI4-Lite slave: Write response
  output wire                      s_axi_bvalid,  // AXI4-Lite slave: Write response valid
  input  wire                      s_axi_bready,  // AXI4-Lite slave: Write response ready
  input  wire [AXI_ADDR_WIDTH-1:0] s_axi_araddr,  // AXI4-Lite slave: Read address
  input  wire                      s_axi_arvalid, // AXI4-Lite slave: Read address valid
  output wire                      s_axi_arready, // AXI4-Lite slave: Read address ready
  output wire [AXI_DATA_WIDTH-1:0] s_axi_rdata,   // AXI4-Lite slave: Read data
  output wire [1:0]                s_axi_rresp,   // AXI4-Lite slave: Read data response
  output wire                      s_axi_rvalid,  // AXI4-Lite slave: Read data valid
  input  wire                      s_axi_rready,  // AXI4-Lite slave: Read data ready

  // Master side
  output wire [AXI_DATA_WIDTH-1:0] m_axis_tdata,
  output wire                      m_axis_tvalid
);

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

  assign m_axis_tdata = int_wdata_wire;
  assign m_axis_tvalid = int_awvalid_wire & int_wvalid_wire & int_bready_wire;

endmodule
