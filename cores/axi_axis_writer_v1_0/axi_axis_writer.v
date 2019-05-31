
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

  reg int_bvalid_reg, int_bvalid_next;

  wire int_awready_wire;

  assign int_awready_wire = ~int_bvalid_reg & s_axi_awvalid & s_axi_wvalid;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_bvalid_reg <= 1'b0;
    end
    else
    begin
      int_bvalid_reg <= int_bvalid_next;
    end
  end

  always @*
  begin
    int_bvalid_next = int_bvalid_reg;

    if(int_awready_wire)
    begin
      int_bvalid_next = 1'b1;
    end

    if(int_bvalid_reg & s_axi_bready)
    begin
      int_bvalid_next = 1'b0;
    end
  end

  assign s_axi_awready = int_awready_wire;
  assign s_axi_wready = int_awready_wire;
  assign s_axi_bresp = 2'd0;
  assign s_axi_bvalid = int_bvalid_reg;
  assign s_axi_arready = 1'b0;
  assign s_axi_rdata = {(AXI_DATA_WIDTH){1'b0}};
  assign s_axi_rresp = 2'd0;
  assign s_axi_rvalid = 1'b0;

  assign m_axis_tdata = s_axi_wdata;
  assign m_axis_tvalid = s_axi_wvalid;

endmodule
