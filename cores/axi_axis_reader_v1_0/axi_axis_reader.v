
`timescale 1 ns / 1 ps

module axi_axis_reader #
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

  // Slave side
  output wire                      s_axis_tready,
  input  wire [AXI_DATA_WIDTH-1:0] s_axis_tdata,
  input  wire                      s_axis_tvalid
);

  reg int_arready_reg, int_arready_next;
  reg [AXI_DATA_WIDTH-1:0] int_rdata_reg, int_rdata_next;
  reg int_rvalid_reg, int_rvalid_next;

  wire int_ardone_wire, int_rdone_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_arready_reg <= 1'b1;
      int_rdata_reg <= {(AXI_DATA_WIDTH){1'b0}};
      int_rvalid_reg <= 1'b0;
    end
    else
    begin
      int_arready_reg <= int_arready_next;
      int_rdata_reg <= int_rdata_next;
      int_rvalid_reg <= int_rvalid_next;
    end
  end

  assign int_ardone_wire = ~int_arready_reg | s_axi_arvalid;
  assign int_rdone_wire = ~int_rvalid_reg | s_axi_rready;

  always @*
  begin
    int_arready_next = ~int_ardone_wire | int_rdone_wire;
    int_rdata_next = int_rdata_reg;
    int_rvalid_next = ~int_rdone_wire | int_ardone_wire;

    if(int_ardone_wire & int_rdone_wire)
    begin
      int_rdata_next = s_axis_tvalid ? s_axis_tdata : {(AXI_DATA_WIDTH){1'b0}};
    end
  end

  assign s_axi_awready = 1'b0;
  assign s_axi_wready = 1'b0;
  assign s_axi_bresp = 2'd0;
  assign s_axi_bvalid = 1'b0;
  assign s_axi_arready = int_arready_reg;
  assign s_axi_rdata = int_rdata_reg;
  assign s_axi_rresp = 2'd0;
  assign s_axi_rvalid = int_rvalid_reg;

  assign s_axis_tready = int_ardone_wire & int_rdone_wire;

endmodule
