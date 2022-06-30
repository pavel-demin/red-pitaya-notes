
`timescale 1 ns / 1 ps

module axis_ram_writer #
(
  parameter integer ADDR_WIDTH = 20,
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

  input  wire [AXI_ADDR_WIDTH-1:0]   cfg_data,
  output wire [ADDR_WIDTH-1:0]       sts_data,

  // Master side
  output wire [AXI_ID_WIDTH-1:0]     m_axi_awid,    // AXI master: Write address ID
  output wire [AXI_ADDR_WIDTH-1:0]   m_axi_awaddr,  // AXI master: Write address
  output wire [3:0]                  m_axi_awlen,   // AXI master: Write burst length
  output wire [2:0]                  m_axi_awsize,  // AXI master: Write burst size
  output wire [1:0]                  m_axi_awburst, // AXI master: Write burst type
  output wire [3:0]                  m_axi_awcache, // AXI master: Write memory type
  output wire                        m_axi_awvalid, // AXI master: Write address valid
  input  wire                        m_axi_awready, // AXI master: Write address ready
  output wire [AXI_ID_WIDTH-1:0]     m_axi_wid,     // AXI master: Write data ID
  output wire [AXI_DATA_WIDTH-1:0]   m_axi_wdata,   // AXI master: Write data
  output wire [AXI_DATA_WIDTH/8-1:0] m_axi_wstrb,   // AXI master: Write strobes
  output wire                        m_axi_wlast,   // AXI master: Write last
  output wire                        m_axi_wvalid,  // AXI master: Write valid
  input  wire                        m_axi_wready,  // AXI master: Write ready
  input  wire                        m_axi_bvalid,  // AXI master: Write response valid
  output wire                        m_axi_bready,  // AXI master: Write response ready

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid
);

  function integer clogb2 (input integer value);
    for(clogb2 = 0; value > 0; clogb2 = clogb2 + 1) value = value >> 1;
  endfunction

  localparam integer ADDR_SIZE = clogb2(AXI_DATA_WIDTH/8 - 1);
  localparam integer COUNT_SIZE = clogb2(FIFO_WRITE_DEPTH*AXIS_TDATA_WIDTH/AXI_DATA_WIDTH - 1) + 1;

  reg int_awvalid_reg, int_awvalid_next;
  reg int_wvalid_reg, int_wvalid_next;
  reg [ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [AXI_ID_WIDTH-1:0] int_awid_reg, int_awid_next;

  wire int_full_wire, int_empty_wire, int_rden_wire;
  wire int_wlast_wire, int_tready_wire;
  wire [COUNT_SIZE-1:0] int_count_wire;
  wire [AXI_DATA_WIDTH-1:0] int_wdata_wire;

  assign int_tready_wire = ~int_full_wire;
  assign int_wlast_wire = &int_addr_reg[3:0];
  assign int_rden_wire = m_axi_wready & int_wvalid_reg;

  xpm_fifo_sync #(
    .WRITE_DATA_WIDTH(AXIS_TDATA_WIDTH),
    .FIFO_WRITE_DEPTH(FIFO_WRITE_DEPTH),
    .READ_DATA_WIDTH(AXI_DATA_WIDTH),
    .READ_MODE("fwft"),
    .FIFO_READ_LATENCY(0),
    .USE_ADV_FEATURES("0400"),
    .RD_DATA_COUNT_WIDTH(COUNT_SIZE)
  ) fifo_0 (
    .full(int_full_wire),
    .rd_data_count(int_count_wire),
    .rst(~aresetn),
    .wr_clk(aclk),
    .wr_en(int_tready_wire & s_axis_tvalid),
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
      int_addr_reg <= {(ADDR_WIDTH){1'b0}};
      int_awid_reg <= {(AXI_ID_WIDTH){1'b0}};
    end
    else
    begin
      int_awvalid_reg <= int_awvalid_next;
      int_wvalid_reg <= int_wvalid_next;
      int_addr_reg <= int_addr_next;
      int_awid_reg <= int_awid_next;
    end
  end

  always @*
  begin
    int_awvalid_next = int_awvalid_reg;
    int_wvalid_next = int_wvalid_reg;
    int_addr_next = int_addr_reg;
    int_awid_next = int_awid_reg;

    if((int_count_wire > 4'd15) & ~int_awvalid_reg & ~int_wvalid_reg)
    begin
      int_awvalid_next = 1'b1;
      int_wvalid_next = 1'b1;
    end

    if(m_axi_awready & int_awvalid_reg)
    begin
      int_awvalid_next = 1'b0;
    end

    if(int_rden_wire)
    begin
      int_addr_next = int_addr_reg + 1'b1;
    end

    if(m_axi_wready & int_wlast_wire)
    begin
      int_awid_next = int_awid_reg + 1'b1;
      if(int_count_wire > 5'd16)
      begin
        int_awvalid_next = 1'b1;
      end
      else
      begin
        int_wvalid_next = 1'b0;
      end
    end
  end

  assign sts_data = int_addr_reg;

  assign m_axi_awid = int_awid_reg;
  assign m_axi_awaddr = cfg_data + {int_addr_reg, {(ADDR_SIZE){1'b0}}};
  assign m_axi_awlen = 4'd15;
  assign m_axi_awsize = ADDR_SIZE;
  assign m_axi_awburst = 2'b01;
  assign m_axi_awcache = 4'b0110;
  assign m_axi_awvalid = int_awvalid_reg;
  assign m_axi_wid = int_awid_reg;
  assign m_axi_wdata = int_wdata_wire;
  assign m_axi_wstrb = {(AXI_DATA_WIDTH/8){1'b1}};
  assign m_axi_wlast = int_wlast_wire;
  assign m_axi_wvalid = int_wvalid_reg;
  assign m_axi_bready = 1'b1;

  assign s_axis_tready = int_tready_wire;

endmodule
