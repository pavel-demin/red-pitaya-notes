
`timescale 1 ns / 1 ps

module axi_hub #
(
  parameter integer CFG_DATA_WIDTH = 1024,
  parameter integer STS_DATA_WIDTH = 1024
)
(
  input  wire                      aclk,
  input  wire                      aresetn,

  input  wire [11:0]               s_axi_awid,
  input  wire [31:0]               s_axi_awaddr,
  input  wire                      s_axi_awvalid,
  output wire                      s_axi_awready,

  input  wire [3:0]                s_axi_wstrb,
  input  wire                      s_axi_wlast,
  input  wire [31:0]               s_axi_wdata,
  input  wire                      s_axi_wvalid,
  output wire                      s_axi_wready,

  output wire [11:0]               s_axi_bid,
  output wire                      s_axi_bvalid,
  input  wire                      s_axi_bready,

  input  wire [11:0]               s_axi_arid,
  input  wire [3:0]                s_axi_arlen,
  input  wire [31:0]               s_axi_araddr,
  input  wire                      s_axi_arvalid,
  output wire                      s_axi_arready,

  output wire [11:0]               s_axi_rid,
  output wire                      s_axi_rlast,
  output wire [31:0]               s_axi_rdata,
  output wire                      s_axi_rvalid,
  input  wire                      s_axi_rready,

  output wire [CFG_DATA_WIDTH-1:0] cfg_data,

  input  wire [STS_DATA_WIDTH-1:0] sts_data,

  output wire                      bram_porta_clk,
  output wire                      bram_porta_rst,
  output wire                      bram_porta_en,
  output wire [3:0]                bram_porta_we,
  output wire [15:0]               bram_porta_addr,
  output wire [31:0]               bram_porta_wrdata,
  input  wire [31:0]               bram_porta_rddata,

  output wire                      bram_portb_clk,
  output wire                      bram_portb_rst,
  output wire                      bram_portb_en,
  output wire [3:0]                bram_portb_we,
  output wire [15:0]               bram_portb_addr,
  output wire [31:0]               bram_portb_wrdata,
  input  wire [31:0]               bram_portb_rddata,

  input  wire [31:0]               s00_axis_tdata,
  input  wire                      s00_axis_tvalid,
  output wire                      s00_axis_tready,

  output wire [31:0]               m00_axis_tdata,
  output wire                      m00_axis_tvalid,
  input  wire                      m00_axis_tready,

  input  wire [31:0]               s01_axis_tdata,
  input  wire                      s01_axis_tvalid,
  output wire                      s01_axis_tready,

  output wire [31:0]               m01_axis_tdata,
  output wire                      m01_axis_tvalid,
  input  wire                      m01_axis_tready,

  input  wire [31:0]               s02_axis_tdata,
  input  wire                      s02_axis_tvalid,
  output wire                      s02_axis_tready,

  output wire [31:0]               m02_axis_tdata,
  output wire                      m02_axis_tvalid,
  input  wire                      m02_axis_tready,

  input  wire [31:0]               s03_axis_tdata,
  input  wire                      s03_axis_tvalid,
  output wire                      s03_axis_tready,

  output wire [31:0]               m03_axis_tdata,
  output wire                      m03_axis_tvalid,
  input  wire                      m03_axis_tready,

  input  wire [31:0]               s04_axis_tdata,
  input  wire                      s04_axis_tvalid,
  output wire                      s04_axis_tready,

  output wire [31:0]               m04_axis_tdata,
  output wire                      m04_axis_tvalid,
  input  wire                      m04_axis_tready,

  input  wire [31:0]               s05_axis_tdata,
  input  wire                      s05_axis_tvalid,
  output wire                      s05_axis_tready,

  output wire [31:0]               m05_axis_tdata,
  output wire                      m05_axis_tvalid,
  input  wire                      m05_axis_tready,

  input  wire [31:0]               s06_axis_tdata,
  input  wire                      s06_axis_tvalid,
  output wire                      s06_axis_tready,

  output wire [31:0]               m06_axis_tdata,
  output wire                      m06_axis_tvalid,
  input  wire                      m06_axis_tready,

  input  wire [31:0]               s07_axis_tdata,
  input  wire                      s07_axis_tvalid,
  output wire                      s07_axis_tready,

  output wire [31:0]               m07_axis_tdata,
  output wire                      m07_axis_tvalid,
  input  wire                      m07_axis_tready,

  input  wire [31:0]               s08_axis_tdata,
  input  wire                      s08_axis_tvalid,
  output wire                      s08_axis_tready,

  output wire [31:0]               m08_axis_tdata,
  output wire                      m08_axis_tvalid,
  input  wire                      m08_axis_tready,

  input  wire [31:0]               s09_axis_tdata,
  input  wire                      s09_axis_tvalid,
  output wire                      s09_axis_tready,

  output wire [31:0]               m09_axis_tdata,
  output wire                      m09_axis_tvalid,
  input  wire                      m09_axis_tready,

  input  wire [31:0]               s10_axis_tdata,
  input  wire                      s10_axis_tvalid,
  output wire                      s10_axis_tready,

  output wire [31:0]               m10_axis_tdata,
  output wire                      m10_axis_tvalid,
  input  wire                      m10_axis_tready,

  input  wire [31:0]               s11_axis_tdata,
  input  wire                      s11_axis_tvalid,
  output wire                      s11_axis_tready,

  output wire [31:0]               m11_axis_tdata,
  output wire                      m11_axis_tvalid,
  input  wire                      m11_axis_tready,

  input  wire [31:0]               s12_axis_tdata,
  input  wire                      s12_axis_tvalid,
  output wire                      s12_axis_tready,

  output wire [31:0]               m12_axis_tdata,
  output wire                      m12_axis_tvalid,
  input  wire                      m12_axis_tready,

  input  wire [31:0]               s13_axis_tdata,
  input  wire                      s13_axis_tvalid,
  output wire                      s13_axis_tready,

  output wire [31:0]               m13_axis_tdata,
  output wire                      m13_axis_tvalid,
  input  wire                      m13_axis_tready,

  input  wire [31:0]               s14_axis_tdata,
  input  wire                      s14_axis_tvalid,
  output wire                      s14_axis_tready,

  output wire [31:0]               m14_axis_tdata,
  output wire                      m14_axis_tvalid,
  input  wire                      m14_axis_tready,

  input  wire [31:0]               s15_axis_tdata,
  input  wire                      s15_axis_tvalid,
  output wire                      s15_axis_tready,

  output wire [31:0]               m15_axis_tdata,
  output wire                      m15_axis_tvalid,
  input  wire                      m15_axis_tready
);

  function integer clogb2 (input integer value);
    for(clogb2 = 0; value > 0; clogb2 = clogb2 + 1) value = value >> 1;
  endfunction

  localparam integer NUM_AXIS = 16;
  localparam integer MUX_SIZE = NUM_AXIS + 4;
  localparam integer CFG_SIZE = CFG_DATA_WIDTH / 32;
  localparam integer CFG_WIDTH = CFG_SIZE > 1 ? clogb2(CFG_SIZE - 1) : 1;
  localparam integer STS_SIZE = STS_DATA_WIDTH / 32;
  localparam integer STS_WIDTH = STS_SIZE > 1 ? clogb2(STS_SIZE - 1) : 1;

  reg [3:0] int_awcntr_reg, int_awcntr_next;
  reg [3:0] int_arcntr_reg, int_arcntr_next;
  reg [1:0] int_rsel_reg;

  wire int_awvalid_wire, int_awready_wire;
  wire int_wvalid_wire, int_wready_wire;
  wire int_bvalid_wire, int_bready_wire;
  wire int_arvalid_wire, int_arready_wire;
  wire int_rvalid_wire, int_rready_wire;

  wire [11:0] int_awid_wire;
  wire [31:0] int_awaddr_wire;

  wire [3:0] int_wstrb_wire;
  wire int_wlast_wire;
  wire [31:0] int_wdata_wire;

  wire [11:0] int_arid_wire;
  wire [3:0]  int_arlen_wire;
  wire [31:0] int_araddr_wire;

  wire [11:0] int_rid_wire;
  wire int_rlast_wire;
  wire [31:0] int_rdata_wire [3:0];

  wire [31:0] int_sdata_wire [NUM_AXIS-1:0];
  wire [31:0] int_mdata_wire [NUM_AXIS-1:0];
  wire [NUM_AXIS-1:0] int_svalid_wire, int_sready_wire;
  wire [NUM_AXIS-1:0] int_mvalid_wire, int_mready_wire;

  wire [15:0] int_waddr_wire;
  wire [15:0] int_raddr_wire;

  wire [31:0] int_cfg_mux [CFG_SIZE-1:0];
  wire [31:0] int_sts_mux [STS_SIZE-1:0];

  wire [31:0] int_rdata_mux [MUX_SIZE-1:0];
  wire [MUX_SIZE-1:0] int_wsel_wire, int_rsel_wire;

  wire [CFG_SIZE-1:0] int_ce_wire;
  wire int_we_wire, int_re_wire;

  genvar j, k;

  assign int_awready_wire = int_bready_wire & int_wvalid_wire & int_wlast_wire;
  assign int_wready_wire = int_bready_wire & int_awvalid_wire;
  assign int_bvalid_wire = int_awvalid_wire & int_wvalid_wire & int_wlast_wire;

  assign int_arready_wire = int_rready_wire & int_rlast_wire;
  assign int_rvalid_wire = int_arvalid_wire;
  assign int_rlast_wire = int_arcntr_reg == int_arlen_wire;

  assign int_we_wire = int_bready_wire & int_awvalid_wire & int_wvalid_wire;
  assign int_re_wire = int_rready_wire & int_arvalid_wire;

  assign int_waddr_wire = int_awaddr_wire[17:2] + int_awcntr_reg;
  assign int_raddr_wire = int_araddr_wire[17:2] + int_arcntr_reg;

  assign int_rdata_wire[0] = int_rdata_mux[int_araddr_wire[27:20]];
  assign int_rdata_wire[2] = int_rsel_reg[0] ? bram_porta_rddata : 32'd0;
  assign int_rdata_wire[3] = int_rsel_reg[1] ? bram_portb_rddata : 32'd0;

  assign int_rdata_mux[0] = int_cfg_mux[int_raddr_wire[CFG_WIDTH-1:0]];
  assign int_rdata_mux[1] = int_sts_mux[int_raddr_wire[STS_WIDTH-1:0]];
  assign int_rdata_mux[2] = 32'd0;
  assign int_rdata_mux[3] = 32'd0;

  generate
    for(j = 0; j < NUM_AXIS; j = j + 1)
    begin : MUXES
      assign int_rdata_mux[j+4] = int_svalid_wire[j] ? int_sdata_wire[j] : 32'd0;
      assign int_mdata_wire[j] = int_wdata_wire;
      assign int_mvalid_wire[j] = int_wsel_wire[j+4];
      assign int_sready_wire[j] = int_rsel_wire[j+4];
    end
  endgenerate

  generate
    for(j = 0; j < MUX_SIZE; j = j + 1)
    begin : SELECTS
      assign int_wsel_wire[j] = int_we_wire & (int_awaddr_wire[27:20] == j);
      assign int_rsel_wire[j] = int_re_wire & (int_araddr_wire[27:20] == j);
    end
  endgenerate

  generate
    for(j = 0; j < CFG_SIZE; j = j + 1)
    begin : CFG_WORDS
      assign int_cfg_mux[j] = cfg_data[j*32+31:j*32];
      assign int_ce_wire[j] = int_wsel_wire[0] & (int_waddr_wire[CFG_WIDTH-1:0] == j);
      for(k = 0; k < 32; k = k + 1)
      begin : CFG_BITS
        FDRE #(
          .INIT(1'b0)
        ) FDRE_inst (
          .CE(int_ce_wire[j] & int_wstrb_wire[k/8]),
          .C(aclk),
          .R(~aresetn),
          .D(int_wdata_wire[k]),
          .Q(cfg_data[j*32 + k])
        );
      end
    end
  endgenerate

  generate
    for(j = 0; j < STS_SIZE; j = j + 1)
    begin : STS_WORDS
      assign int_sts_mux[j] = sts_data[j*32+31:j*32];
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_awcntr_reg <= 4'd0;
      int_arcntr_reg <= 4'd0;
      int_rsel_reg <= 2'd0;
    end
    else
    begin
      int_awcntr_reg <= int_awcntr_next;
      int_arcntr_reg <= int_arcntr_next;
      int_rsel_reg <= {int_rsel_wire[3], int_rsel_wire[2]};
    end
  end

  always @*
  begin
    int_awcntr_next = int_awcntr_reg;
    int_arcntr_next = int_arcntr_reg;

    if(int_awvalid_wire & int_awready_wire)
    begin
      int_awcntr_next = 4'd0;
    end

    if(int_arvalid_wire & int_arready_wire)
    begin
      int_arcntr_next = 4'd0;
    end

    if(~int_wlast_wire & int_we_wire)
    begin
      int_awcntr_next = int_awcntr_reg + 1'b1;
    end

    if(~int_rlast_wire & int_re_wire)
    begin
      int_arcntr_next = int_arcntr_reg + 1'b1;
    end
  end

  assign int_sdata_wire[0] = s00_axis_tdata;
  assign int_svalid_wire[0] = s00_axis_tvalid;
  assign s00_axis_tready = int_sready_wire[0];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[0]), .in_valid(int_mvalid_wire[0]), .in_ready(int_mready_wire[0]),
    .out_data(m00_axis_tdata), .out_valid(m00_axis_tvalid), .out_ready(m00_axis_tready)
  );

  assign int_sdata_wire[1] = s01_axis_tdata;
  assign int_svalid_wire[1] = s01_axis_tvalid;
  assign s01_axis_tready = int_sready_wire[1];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_1 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[1]), .in_valid(int_mvalid_wire[1]), .in_ready(int_mready_wire[1]),
    .out_data(m01_axis_tdata), .out_valid(m01_axis_tvalid), .out_ready(m01_axis_tready)
  );

  assign int_sdata_wire[2] = s02_axis_tdata;
  assign int_svalid_wire[2] = s02_axis_tvalid;
  assign s02_axis_tready = int_sready_wire[2];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_2 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[2]), .in_valid(int_mvalid_wire[2]), .in_ready(int_mready_wire[2]),
    .out_data(m02_axis_tdata), .out_valid(m02_axis_tvalid), .out_ready(m02_axis_tready)
  );

  assign int_sdata_wire[3] = s03_axis_tdata;
  assign int_svalid_wire[3] = s03_axis_tvalid;
  assign s03_axis_tready = int_sready_wire[3];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_3 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[3]), .in_valid(int_mvalid_wire[3]), .in_ready(int_mready_wire[3]),
    .out_data(m03_axis_tdata), .out_valid(m03_axis_tvalid), .out_ready(m03_axis_tready)
  );

  assign int_sdata_wire[4] = s04_axis_tdata;
  assign int_svalid_wire[4] = s04_axis_tvalid;
  assign s04_axis_tready = int_sready_wire[4];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_4 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[4]), .in_valid(int_mvalid_wire[4]), .in_ready(int_mready_wire[4]),
    .out_data(m04_axis_tdata), .out_valid(m04_axis_tvalid), .out_ready(m04_axis_tready)
  );

  assign int_sdata_wire[5] = s05_axis_tdata;
  assign int_svalid_wire[5] = s05_axis_tvalid;
  assign s05_axis_tready = int_sready_wire[5];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_5 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[5]), .in_valid(int_mvalid_wire[5]), .in_ready(int_mready_wire[5]),
    .out_data(m05_axis_tdata), .out_valid(m05_axis_tvalid), .out_ready(m05_axis_tready)
  );

  assign int_sdata_wire[6] = s06_axis_tdata;
  assign int_svalid_wire[6] = s06_axis_tvalid;
  assign s06_axis_tready = int_sready_wire[6];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_6 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[6]), .in_valid(int_mvalid_wire[6]), .in_ready(int_mready_wire[6]),
    .out_data(m06_axis_tdata), .out_valid(m06_axis_tvalid), .out_ready(m06_axis_tready)
  );

  assign int_sdata_wire[7] = s07_axis_tdata;
  assign int_svalid_wire[7] = s07_axis_tvalid;
  assign s07_axis_tready = int_sready_wire[7];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_7 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[7]), .in_valid(int_mvalid_wire[7]), .in_ready(int_mready_wire[7]),
    .out_data(m07_axis_tdata), .out_valid(m07_axis_tvalid), .out_ready(m07_axis_tready)
  );

  assign int_sdata_wire[8] = s08_axis_tdata;
  assign int_svalid_wire[8] = s08_axis_tvalid;
  assign s08_axis_tready = int_sready_wire[8];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_8 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[8]), .in_valid(int_mvalid_wire[8]), .in_ready(int_mready_wire[8]),
    .out_data(m08_axis_tdata), .out_valid(m08_axis_tvalid), .out_ready(m08_axis_tready)
  );

  assign int_sdata_wire[9] = s09_axis_tdata;
  assign int_svalid_wire[9] = s09_axis_tvalid;
  assign s09_axis_tready = int_sready_wire[9];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_9 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[9]), .in_valid(int_mvalid_wire[9]), .in_ready(int_mready_wire[9]),
    .out_data(m09_axis_tdata), .out_valid(m09_axis_tvalid), .out_ready(m09_axis_tready)
  );

  assign int_sdata_wire[10] = s10_axis_tdata;
  assign int_svalid_wire[10] = s10_axis_tvalid;
  assign s10_axis_tready = int_sready_wire[10];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_10 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[10]), .in_valid(int_mvalid_wire[10]), .in_ready(int_mready_wire[10]),
    .out_data(m10_axis_tdata), .out_valid(m10_axis_tvalid), .out_ready(m10_axis_tready)
  );

  assign int_sdata_wire[11] = s11_axis_tdata;
  assign int_svalid_wire[11] = s11_axis_tvalid;
  assign s11_axis_tready = int_sready_wire[11];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_11 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[11]), .in_valid(int_mvalid_wire[11]), .in_ready(int_mready_wire[11]),
    .out_data(m11_axis_tdata), .out_valid(m11_axis_tvalid), .out_ready(m11_axis_tready)
  );

  assign int_sdata_wire[12] = s12_axis_tdata;
  assign int_svalid_wire[12] = s12_axis_tvalid;
  assign s12_axis_tready = int_sready_wire[12];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_12 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[12]), .in_valid(int_mvalid_wire[12]), .in_ready(int_mready_wire[12]),
    .out_data(m12_axis_tdata), .out_valid(m12_axis_tvalid), .out_ready(m12_axis_tready)
  );

  assign int_sdata_wire[13] = s13_axis_tdata;
  assign int_svalid_wire[13] = s13_axis_tvalid;
  assign s13_axis_tready = int_sready_wire[13];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_13 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[13]), .in_valid(int_mvalid_wire[13]), .in_ready(int_mready_wire[13]),
    .out_data(m13_axis_tdata), .out_valid(m13_axis_tvalid), .out_ready(m13_axis_tready)
  );

  assign int_sdata_wire[14] = s14_axis_tdata;
  assign int_svalid_wire[14] = s14_axis_tvalid;
  assign s14_axis_tready = int_sready_wire[14];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_14 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[14]), .in_valid(int_mvalid_wire[14]), .in_ready(int_mready_wire[14]),
    .out_data(m14_axis_tdata), .out_valid(m14_axis_tvalid), .out_ready(m14_axis_tready)
  );

  assign int_sdata_wire[15] = s15_axis_tdata;
  assign int_svalid_wire[15] = s15_axis_tvalid;
  assign s15_axis_tready = int_sready_wire[15];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_15 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[15]), .in_valid(int_mvalid_wire[15]), .in_ready(int_mready_wire[15]),
    .out_data(m15_axis_tdata), .out_valid(m15_axis_tvalid), .out_ready(m15_axis_tready)
  );

  input_buffer #(
    .DATA_WIDTH(44)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({s_axi_awid, s_axi_awaddr}),
    .in_valid(s_axi_awvalid), .in_ready(s_axi_awready),
    .out_data({int_awid_wire, int_awaddr_wire}),
    .out_valid(int_awvalid_wire), .out_ready(int_awready_wire)
  );

  input_buffer #(
    .DATA_WIDTH(37)
  ) buf_1 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({s_axi_wstrb, s_axi_wlast, s_axi_wdata}),
    .in_valid(s_axi_wvalid), .in_ready(s_axi_wready),
    .out_data({int_wstrb_wire, int_wlast_wire, int_wdata_wire}),
    .out_valid(int_wvalid_wire), .out_ready(int_wready_wire)
  );

  output_buffer #(
    .DATA_WIDTH(12)
  ) buf_2 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_awid_wire), .in_valid(int_bvalid_wire), .in_ready(int_bready_wire),
    .out_data(s_axi_bid), .out_valid(s_axi_bvalid), .out_ready(s_axi_bready)
  );

  input_buffer #(
    .DATA_WIDTH(48)
  ) buf_3 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({s_axi_arid, s_axi_arlen, s_axi_araddr}),
    .in_valid(s_axi_arvalid), .in_ready(s_axi_arready),
    .out_data({int_arid_wire, int_arlen_wire, int_araddr_wire}),
    .out_valid(int_arvalid_wire), .out_ready(int_arready_wire)
  );

  output_buffer #(
    .DATA_WIDTH(45)
  ) buf_4 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({int_arid_wire, int_rlast_wire, int_rdata_wire[0]}),
    .in_valid(int_rvalid_wire), .in_ready(int_rready_wire),
    .out_data({s_axi_rid, s_axi_rlast, int_rdata_wire[1]}),
    .out_valid(s_axi_rvalid), .out_ready(s_axi_rready)
  );

  assign s_axi_rdata = int_rdata_wire[1] | int_rdata_wire[2] | int_rdata_wire[3];

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_en = int_rsel_wire[2] | int_wsel_wire[2];
  assign bram_porta_we = int_wsel_wire[2] ? int_wstrb_wire : 4'd0;
  assign bram_porta_addr = int_we_wire ? int_waddr_wire : int_raddr_wire;
  assign bram_porta_wrdata = int_wdata_wire;

  assign bram_portb_clk = aclk;
  assign bram_portb_rst = ~aresetn;
  assign bram_portb_en = int_rsel_wire[3] | int_wsel_wire[3];
  assign bram_portb_we = int_wsel_wire[3] ? int_wstrb_wire : 4'd0;
  assign bram_portb_addr = int_we_wire ? int_waddr_wire : int_raddr_wire;
  assign bram_portb_wrdata = int_wdata_wire;

endmodule
