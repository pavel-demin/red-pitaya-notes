
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

  reg int_awready_reg, int_awready_next;
  reg [AXI_DATA_WIDTH-1:0] int_wdata_reg, int_wdata_next;
  reg int_wready_reg, int_wready_next;
  reg int_bvalid_reg, int_bvalid_next;

  wire int_awdone_wire, int_wdone_wire, int_bdone_wire;
  wire [AXI_DATA_WIDTH-1:0] int_wdata_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_awready_reg <= 1'b1;
      int_wdata_reg <= {(AXI_DATA_WIDTH){1'b0}};
      int_wready_reg <= 1'b1;
      int_bvalid_reg <= 1'b0;
    end
    else
    begin
      int_awready_reg <= int_awready_next;
      int_wdata_reg <= int_wdata_next;
      int_wready_reg <= int_wready_next;
      int_bvalid_reg <= int_bvalid_next;
    end
  end

  assign int_awdone_wire = ~int_awready_reg | s_axi_awvalid;
  assign int_wdone_wire = ~int_wready_reg | s_axi_wvalid;
  assign int_bdone_wire = ~int_bvalid_reg | s_axi_bready;

  assign int_wdata_wire = int_wready_reg ? s_axi_wdata : int_wdata_reg;

  always @*
  begin
    int_awready_next = ~int_awdone_wire | (int_wdone_wire & int_bdone_wire);
    int_wdata_next = int_wdata_reg;
    int_wready_next = ~int_wdone_wire | (int_awdone_wire & int_bdone_wire);
    int_bvalid_next = ~int_bdone_wire | (int_awdone_wire & int_wdone_wire);

    if(int_wready_reg)
    begin
      int_wdata_next = s_axi_wdata;
    end
  end

  assign s_axi_awready = int_awready_reg;
  assign s_axi_wready = int_wready_reg;
  assign s_axi_bresp = 2'd0;
  assign s_axi_bvalid = int_bvalid_reg;
  assign s_axi_arready = 1'b0;
  assign s_axi_rdata = {(AXI_DATA_WIDTH){1'b0}};
  assign s_axi_rresp = 2'd0;
  assign s_axi_rvalid = 1'b0;

  assign m_axis_tdata = int_wdata_wire;
  assign m_axis_tvalid = int_awdone_wire & int_wdone_wire & int_bdone_wire;

endmodule
