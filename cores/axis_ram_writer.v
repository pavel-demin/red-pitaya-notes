
`timescale 1 ns / 1 ps

module axis_ram_writer #
(
  parameter integer ADDR_WIDTH = 16,
  parameter integer AXI_ID_WIDTH = 6,
  parameter integer AXI_ADDR_WIDTH = 32,
  parameter integer AXI_DATA_WIDTH = 64,
  parameter integer AXIS_TDATA_WIDTH = 64,
  parameter integer FIFO_WRITE_DEPTH = 512
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [AXI_ADDR_WIDTH-1:0]   min_addr,
  input  wire [ADDR_WIDTH-1:0]       cfg_data,
  output wire [ADDR_WIDTH-1:0]       sts_data,

  // Master side
  output wire [AXI_ID_WIDTH-1:0]     m_axi_awid,    // AXI master: Write address ID
  output wire [3:0]                  m_axi_awlen,   // AXI master: Write burst length
  output wire [2:0]                  m_axi_awsize,  // AXI master: Write burst size
  output wire [1:0]                  m_axi_awburst, // AXI master: Write burst type
  output wire [3:0]                  m_axi_awcache, // AXI master: Write memory type
  output wire [AXI_ADDR_WIDTH-1:0]   m_axi_awaddr,  // AXI master: Write address
  output wire                        m_axi_awvalid, // AXI master: Write address valid
  input  wire                        m_axi_awready, // AXI master: Write address ready

  output wire [AXI_ID_WIDTH-1:0]     m_axi_wid,     // AXI master: Write data ID
  output wire [AXI_DATA_WIDTH/8-1:0] m_axi_wstrb,   // AXI master: Write strobes
  output wire                        m_axi_wlast,   // AXI master: Write last
  output wire [AXI_DATA_WIDTH-1:0]   m_axi_wdata,   // AXI master: Write data
  output wire                        m_axi_wvalid,  // AXI master: Write valid
  input  wire                        m_axi_wready,  // AXI master: Write ready

  input  wire                        m_axi_bvalid,  // AXI master: Write response valid
  output wire                        m_axi_bready,  // AXI master: Write response ready

  // Slave side
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,
  output wire                        s_axis_tready
);

  localparam integer ADDR_SIZE = $clog2(AXI_DATA_WIDTH / 8);
  localparam integer COUNT_WIDTH = $clog2(FIFO_WRITE_DEPTH * AXIS_TDATA_WIDTH / AXI_DATA_WIDTH) + 1;

  reg int_awvalid_reg, int_wvalid_reg;
  reg [3:0] int_cntr_reg;
  reg [ADDR_WIDTH-1:0] int_addr_reg;

  wire int_full_wire, int_valid_wire;
  wire int_awvalid_wire, int_awready_wire;
  wire int_wlast_wire, int_wvalid_wire, int_wready_wire, int_rden_wire;
  wire [COUNT_WIDTH-1:0] int_count_wire;
  wire [AXI_DATA_WIDTH-1:0] int_wdata_wire;

  assign int_valid_wire = (int_count_wire > 15) & ~int_wvalid_reg;
  assign int_awvalid_wire = int_valid_wire | int_awvalid_reg;
  assign int_wvalid_wire = int_valid_wire | int_wvalid_reg;

  assign int_rden_wire = int_wvalid_wire & int_wready_wire;
  assign int_wlast_wire = &int_cntr_reg;

  xpm_fifo_sync #(
    .WRITE_DATA_WIDTH(AXIS_TDATA_WIDTH),
    .FIFO_WRITE_DEPTH(FIFO_WRITE_DEPTH),
    .READ_DATA_WIDTH(AXI_DATA_WIDTH),
    .READ_MODE("fwft"),
    .FIFO_READ_LATENCY(0),
    .FIFO_MEMORY_TYPE("block"),
    .USE_ADV_FEATURES("0400"),
    .RD_DATA_COUNT_WIDTH(COUNT_WIDTH)
  ) fifo_0 (
    .full(int_full_wire),
    .rd_data_count(int_count_wire),
    .rst(~aresetn),
    .wr_clk(aclk),
    .wr_en(s_axis_tvalid),
    .din(s_axis_tdata),
    .rd_en(int_rden_wire),
    .dout(int_wdata_wire)
  );

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_awvalid_reg <= 1'b0;
      int_wvalid_reg <= 1'b0;
      int_cntr_reg <= 4'd0;
      int_addr_reg <= {(ADDR_WIDTH){1'b0}};
    end
    else
    begin
      if(int_valid_wire)
      begin
        int_awvalid_reg <= 1'b1;
        int_wvalid_reg <= 1'b1;
      end

      if(int_awvalid_wire & int_awready_wire)
      begin
        int_awvalid_reg <= 1'b0;
        int_addr_reg <= int_addr_reg < cfg_data ? int_addr_reg + 1'b1 : {(ADDR_WIDTH){1'b0}};
      end

      if(int_rden_wire)
      begin
        int_cntr_reg <= int_cntr_reg + 1'b1;
      end

      if(int_wready_wire & int_wlast_wire)
      begin
        int_wvalid_reg <= 1'b0;
      end
    end
  end

  output_buffer #(
    .DATA_WIDTH(AXI_ADDR_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(min_addr + {int_addr_reg, 4'd0, {(ADDR_SIZE){1'b0}}}),
    .in_valid(int_awvalid_wire), .in_ready(int_awready_wire),
    .out_data(m_axi_awaddr),
    .out_valid(m_axi_awvalid), .out_ready(m_axi_awready)
  );

  output_buffer #(
    .DATA_WIDTH(AXI_DATA_WIDTH+1)
  ) buf_1 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({int_wlast_wire, int_wdata_wire}),
    .in_valid(int_wvalid_wire), .in_ready(int_wready_wire),
    .out_data({m_axi_wlast, m_axi_wdata}),
    .out_valid(m_axi_wvalid), .out_ready(m_axi_wready)
  );

  assign sts_data = int_addr_reg;

  assign m_axi_awid = {(AXI_ID_WIDTH){1'b0}};
  assign m_axi_awlen = 4'd15;
  assign m_axi_awsize = ADDR_SIZE;
  assign m_axi_awburst = 2'b01;
  assign m_axi_awcache = 4'b1111;

  assign m_axi_wid = {(AXI_ID_WIDTH){1'b0}};
  assign m_axi_wstrb = {(AXI_DATA_WIDTH/8){1'b1}};

  assign m_axi_bready = 1'b1;

  assign s_axis_tready = ~int_full_wire;

endmodule
