
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
  output wire [BRAM_ADDR_WIDTH-1:0] bram_porta_addr,
  input  wire [BRAM_DATA_WIDTH-1:0] bram_porta_rddata
);

  function integer clogb2 (input integer value);
    for(clogb2 = 0; value > 0; clogb2 = clogb2 + 1) value = value >> 1;
  endfunction

  localparam integer ADDR_LSB = clogb2(AXI_DATA_WIDTH/8 - 1);

  reg int_rvalid_reg, int_rvalid_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_rvalid_reg <= 1'b0;
    end
    else
    begin
      int_rvalid_reg <= int_rvalid_next;
    end
  end

  always @*
  begin
    int_rvalid_next = int_rvalid_reg;

    if(s_axi_arvalid)
    begin
      int_rvalid_next = 1'b1;
    end

    if(s_axi_rready & int_rvalid_reg)
    begin
      int_rvalid_next = 1'b0;
    end
  end

  assign s_axi_rresp = 2'd0;

  assign s_axi_arready = 1'b1;
  assign s_axi_rdata = bram_porta_rddata;
  assign s_axi_rvalid = int_rvalid_reg;

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_addr = s_axi_araddr[ADDR_LSB+BRAM_ADDR_WIDTH-1:ADDR_LSB];

endmodule
