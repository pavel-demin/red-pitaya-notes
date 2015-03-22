
`timescale 1 ns / 1 ps

module axi_bram_writer #
(
  parameter integer AXI_DATA_WIDTH = 32,
  parameter integer AXI_ADDR_WIDTH = 32,
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

  // BRAM port
  output wire                         bram_porta_clk,
  output wire                         bram_porta_rst,
  output wire [BRAM_ADDR_WIDTH-1:0]   bram_porta_addr,
  output wire [BRAM_DATA_WIDTH-1:0]   bram_porta_wrdata,
  output wire [BRAM_DATA_WIDTH/8-1:0] bram_porta_we
);

  function integer clogb2 (input integer value);
    for(clogb2 = 0; value > 0; clogb2 = clogb2 + 1) value = value >> 1;
  endfunction

  localparam integer ADDR_LSB = clogb2(AXI_DATA_WIDTH/8 - 1);

  reg int_awready_reg, int_awready_next;
  reg int_wready_reg, int_wready_next;
  reg int_bvalid_reg, int_bvalid_next;
  wire int_wvalid_wire;

  assign int_wvalid_wire = s_axi_awvalid & s_axi_wvalid;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_awready_reg <= 1'b0;
      int_wready_reg <= 1'b0;
      int_bvalid_reg <= 1'b0;
    end
    else
    begin
      int_awready_reg <= int_awready_next;
      int_wready_reg <= int_wready_next;
      int_bvalid_reg <= int_bvalid_next;
    end
  end

  always @*
  begin
    int_awready_next = int_awready_reg;
    int_wready_next = int_wready_reg;
    int_bvalid_next = int_bvalid_reg;

    if(int_wvalid_wire & ~int_awready_reg)
    begin
      int_awready_next = 1'b1;
      int_wready_next = 1'b1;
    end

    if(int_awready_reg)
    begin
      int_awready_next = 1'b0;
      int_wready_next = 1'b0;
      int_bvalid_next = 1'b1;
    end

    if(s_axi_bready & int_bvalid_reg)
    begin
      int_bvalid_next = 1'b0;
    end
  end

  assign s_axi_bresp = 2'd0;

  assign s_axi_awready = int_awready_reg;
  assign s_axi_wready = int_wready_reg;
  assign s_axi_bvalid = int_bvalid_reg;

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_addr = s_axi_awaddr[ADDR_LSB+BRAM_ADDR_WIDTH-1:ADDR_LSB];
  assign bram_porta_wrdata = s_axi_wdata;
  assign bram_porta_we = int_wvalid_wire ? s_axi_wstrb : {(BRAM_DATA_WIDTH/8){1'b0}};

endmodule
