
`timescale 1 ns / 1 ps

module axis_ram_reader #
(
  parameter integer ADDR_WIDTH = 16,
  parameter integer AXI_ID_WIDTH = 6,
  parameter integer AXI_ADDR_WIDTH = 32,
  parameter integer AXI_DATA_WIDTH = 64,
  parameter integer AXIS_TDATA_WIDTH = 64,
  parameter integer FIFO_WRITE_DEPTH = 512
)
(
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [AXI_ADDR_WIDTH-1:0]   min_addr,
  input  wire [ADDR_WIDTH-1:0]       cfg_data,
  output wire [ADDR_WIDTH-1:0]       sts_data,

  output wire [AXI_ID_WIDTH-1:0]     m_axi_arid,
  output wire [3:0]                  m_axi_arlen,
  output wire [2:0]                  m_axi_arsize,
  output wire [1:0]                  m_axi_arburst,
  output wire [3:0]                  m_axi_arcache,
  output wire [AXI_ADDR_WIDTH-1:0]   m_axi_araddr,
  output wire                        m_axi_arvalid,
  input  wire                        m_axi_arready,

  input  wire [AXI_ID_WIDTH-1:0]     m_axi_rid,
  input  wire                        m_axi_rlast,
  input  wire [AXI_DATA_WIDTH-1:0]   m_axi_rdata,
  input  wire                        m_axi_rvalid,
  output wire                        m_axi_rready,

  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,
  input  wire                        m_axis_tready
);

  localparam integer ADDR_SIZE = $clog2(AXI_DATA_WIDTH / 8);
  localparam integer COUNT_WIDTH = $clog2(FIFO_WRITE_DEPTH) + 1;

  reg int_arvalid_reg, int_rvalid_reg;
  reg [ADDR_WIDTH-1:0] int_addr_reg;

  wire int_empty_wire, int_valid_wire;
  wire int_arvalid_wire, int_arready_wire;
  wire [COUNT_WIDTH-1:0] int_count_wire;

  assign int_valid_wire = (int_count_wire < FIFO_WRITE_DEPTH - 15) & ~int_rvalid_reg;
  assign int_arvalid_wire = int_valid_wire | int_arvalid_reg;

  xpm_fifo_sync #(
    .WRITE_DATA_WIDTH(AXI_DATA_WIDTH),
    .FIFO_WRITE_DEPTH(FIFO_WRITE_DEPTH),
    .READ_DATA_WIDTH(AXIS_TDATA_WIDTH),
    .READ_MODE("fwft"),
    .FIFO_READ_LATENCY(0),
    .FIFO_MEMORY_TYPE("block"),
    .USE_ADV_FEATURES("0004"),
    .WR_DATA_COUNT_WIDTH(COUNT_WIDTH)
  ) fifo_0 (
    .empty(int_empty_wire),
    .wr_data_count(int_count_wire),
    .rst(~aresetn),
    .wr_clk(aclk),
    .wr_en(m_axi_rvalid),
    .din(m_axi_rdata),
    .rd_en(m_axis_tready),
    .dout(m_axis_tdata)
  );

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_arvalid_reg <= 1'b0;
      int_rvalid_reg <= 1'b0;
      int_addr_reg <= {(ADDR_WIDTH){1'b0}};
    end
    else
    begin
      if(int_valid_wire)
      begin
        int_arvalid_reg <= 1'b1;
        int_rvalid_reg <= 1'b1;
      end

      if(int_arvalid_wire & int_arready_wire)
      begin
        int_arvalid_reg <= 1'b0;
        int_addr_reg <= int_addr_reg < cfg_data ? int_addr_reg + 1'b1 : {(ADDR_WIDTH){1'b0}};
      end

      if(m_axi_rlast)
      begin
        int_rvalid_reg <= 1'b0;
      end
    end
  end

  output_buffer #(
    .DATA_WIDTH(AXI_ADDR_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(min_addr + {int_addr_reg, 4'd0, {(ADDR_SIZE){1'b0}}}),
    .in_valid(int_arvalid_wire), .in_ready(int_arready_wire),
    .out_data(m_axi_araddr),
    .out_valid(m_axi_arvalid), .out_ready(m_axi_arready)
  );

  assign sts_data = int_addr_reg;

  assign m_axi_arid = {(AXI_ID_WIDTH){1'b0}};
  assign m_axi_arlen = 4'd15;
  assign m_axi_arsize = ADDR_SIZE;
  assign m_axi_arburst = 2'b01;
  assign m_axi_arcache = 4'b1111;

  assign m_axi_rready = 1'b1;

  assign m_axis_tvalid = ~int_empty_wire;

endmodule
