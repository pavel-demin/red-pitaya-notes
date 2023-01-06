
`timescale 1 ns / 1 ps

module axi_cfg_register #
(
  parameter integer CFG_DATA_WIDTH = 1024,
  parameter integer AXI_DATA_WIDTH = 32,
  parameter integer AXI_ADDR_WIDTH = 16
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  // Configuration bits
  output wire [CFG_DATA_WIDTH-1:0]   cfg_data,

  // Slave side
  input  wire [AXI_ADDR_WIDTH-1:0]   s_axi_awaddr,  // AXI4-Lite slave: Write address
  input  wire                        s_axi_awvalid, // AXI4-Lite slave: Write address valid
  output wire                        s_axi_awready, // AXI4-Lite slave: Write address ready
  input  wire [AXI_DATA_WIDTH-1:0]   s_axi_wdata,   // AXI4-Lite slave: Write data
  input  wire [AXI_DATA_WIDTH/8-1:0] s_axi_wstrb,   // AXI4-Lite slave: Write strobe
  input  wire                        s_axi_wvalid,  // AXI4-Lite slave: Write data valid
  output wire                        s_axi_wready,  // AXI4-Lite slave: Write data ready
  output wire [1:0]                  s_axi_bresp,   // AXI4-Lite slave: Write response
  output wire                        s_axi_bvalid,  // AXI4-Lite slave: Write response valid
  input  wire                        s_axi_bready,  // AXI4-Lite slave: Write response ready
  input  wire [AXI_ADDR_WIDTH-1:0]   s_axi_araddr,  // AXI4-Lite slave: Read address
  input  wire                        s_axi_arvalid, // AXI4-Lite slave: Read address valid
  output wire                        s_axi_arready, // AXI4-Lite slave: Read address ready
  output wire [AXI_DATA_WIDTH-1:0]   s_axi_rdata,   // AXI4-Lite slave: Read data
  output wire [1:0]                  s_axi_rresp,   // AXI4-Lite slave: Read data response
  output wire                        s_axi_rvalid,  // AXI4-Lite slave: Read data valid
  input  wire                        s_axi_rready   // AXI4-Lite slave: Read data ready
);

  function integer clogb2 (input integer value);
    for(clogb2 = 0; value > 0; clogb2 = clogb2 + 1) value = value >> 1;
  endfunction

  localparam integer ADDR_LSB = clogb2(AXI_DATA_WIDTH/8 - 1);
  localparam integer CFG_SIZE = CFG_DATA_WIDTH/AXI_DATA_WIDTH;
  localparam integer CFG_WIDTH = CFG_SIZE > 1 ? clogb2(CFG_SIZE-1) : 1;

  wire int_awready_wire, int_awvalid_wire;
  wire int_wready_wire, int_wvalid_wire;
  wire int_bready_wire, int_bvalid_wire;
  wire [AXI_ADDR_WIDTH-1:0] int_awaddr_wire;
  wire [AXI_DATA_WIDTH-1:0] int_wdata_wire;
  wire [AXI_DATA_WIDTH/8-1:0] int_wstrb_wire;

  wire int_arvalid_wire, int_rready_wire;
  wire [AXI_ADDR_WIDTH-1:0] int_araddr_wire;
  wire [AXI_DATA_WIDTH-1:0] int_rdata_wire;

  wire [AXI_DATA_WIDTH-1:0] int_data_mux [CFG_SIZE-1:0];
  wire [CFG_DATA_WIDTH-1:0] int_data_wire;
  wire [CFG_SIZE-1:0] int_ce_wire;

  genvar j, k;

  generate
    for(j = 0; j < CFG_SIZE; j = j + 1)
    begin : WORDS
      assign int_data_mux[j] = int_data_wire[j*AXI_DATA_WIDTH+AXI_DATA_WIDTH-1:j*AXI_DATA_WIDTH];
      assign int_ce_wire[j] = int_bvalid_wire & (int_awaddr_wire[ADDR_LSB+CFG_WIDTH-1:ADDR_LSB] == j);
      for(k = 0; k < AXI_DATA_WIDTH; k = k + 1)
      begin : BITS
        FDRE #(
          .INIT(1'b0)
        ) FDRE_inst (
          .CE(int_ce_wire[j] & int_wstrb_wire[k/8]),
          .C(aclk),
          .R(~aresetn),
          .D(int_wdata_wire[k]),
          .Q(int_data_wire[j*AXI_DATA_WIDTH + k])
        );
      end
    end
  endgenerate

  assign int_rdata_wire = int_data_mux[int_araddr_wire[ADDR_LSB+CFG_WIDTH-1:ADDR_LSB]];

  assign cfg_data = int_data_wire;

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

  input_buffer #(
    .DATA_WIDTH(AXI_ADDR_WIDTH)
  ) buf_3 (
    .aclk(aclk), .aresetn(aresetn),
    .in_ready(s_axi_arready), .in_data(s_axi_araddr), .in_valid(s_axi_arvalid),
    .out_ready(int_rready_wire), .out_data(int_araddr_wire), .out_valid(int_arvalid_wire)
  );

  output_buffer #(
    .DATA_WIDTH(AXI_DATA_WIDTH)
  ) buf_4 (
    .aclk(aclk), .aresetn(aresetn),
    .in_ready(int_rready_wire), .in_data(int_rdata_wire), .in_valid(int_arvalid_wire),
    .out_ready(s_axi_rready), .out_data(s_axi_rdata), .out_valid(s_axi_rvalid)
  );

  assign s_axi_bresp = 2'd0;

  assign s_axi_rresp = 2'd0;

endmodule
