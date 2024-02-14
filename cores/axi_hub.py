hub_size = 6
source = """
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
{% for i in range(hub_size) -%}
{% set index =  "%02d" % i %}
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b{{index}}_bram CLK" *)
  output wire                      b{{index}}_bram_clk,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b{{index}}_bram RST" *)
  output wire                      b{{index}}_bram_rst,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b{{index}}_bram EN" *)
  output wire                      b{{index}}_bram_en,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b{{index}}_bram WE" *)
  output wire [3:0]                b{{index}}_bram_we,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b{{index}}_bram ADDR" *)
  output wire [21:0]               b{{index}}_bram_addr,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b{{index}}_bram DIN" *)
  output wire [31:0]               b{{index}}_bram_wdata,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b{{index}}_bram DOUT" *)
  input  wire [31:0]               b{{index}}_bram_rdata,

  input  wire [31:0]               s{{index}}_axis_tdata,
  input  wire                      s{{index}}_axis_tvalid,
  output wire                      s{{index}}_axis_tready,

  output wire [31:0]               m{{index}}_axis_tdata,
  output wire                      m{{index}}_axis_tvalid,
  input  wire                      m{{index}}_axis_tready{% if not loop.last %},{% endif %}
{% endfor -%}
);

  localparam integer HUB_SIZE = {{hub_size}};
  localparam integer MUX_SIZE = HUB_SIZE + 2;
  localparam integer CFG_SIZE = CFG_DATA_WIDTH / 32;
  localparam integer CFG_WIDTH = CFG_SIZE > 1 ? $clog2(CFG_SIZE) : 1;
  localparam integer STS_SIZE = STS_DATA_WIDTH / 32;
  localparam integer STS_WIDTH = STS_SIZE > 1 ? $clog2(STS_SIZE) : 1;

  reg [3:0] int_awcntr_reg, int_awcntr_next;
  reg [3:0] int_arcntr_reg, int_arcntr_next;

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
  wire [31:0] int_rdata_wire [MUX_SIZE-1:0];

  wire [31:0] int_sdata_wire [HUB_SIZE-1:0];
  wire [31:0] int_mdata_wire [HUB_SIZE-1:0];
  wire [HUB_SIZE-1:0] int_svalid_wire, int_sready_wire;
  wire [HUB_SIZE-1:0] int_mvalid_wire, int_mready_wire;

  wire [31:0] int_bdata_wire [HUB_SIZE-1:0];

  wire [21:0] int_waddr_wire;
  wire [21:0] int_raddr_wire;

  wire [31:0] int_cfg_mux [CFG_SIZE-1:0];
  wire [31:0] int_sts_mux [STS_SIZE-1:0];

  wire [31:0] int_rdata_mux [MUX_SIZE-1:0];
  wire [MUX_SIZE-1:0] int_wsel_wire, int_rsel_wire;

  wire [HUB_SIZE-1:0] int_bsel_wire;

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

  assign int_waddr_wire = int_awaddr_wire[23:2] + int_awcntr_reg;
  assign int_raddr_wire = int_araddr_wire[23:2] + int_arcntr_reg;

  assign int_rdata_wire[0] = int_rdata_mux[int_araddr_wire[27:24]];

  assign int_rdata_mux[0] = int_cfg_mux[int_raddr_wire[CFG_WIDTH-1:0]];
  assign int_rdata_mux[1] = int_sts_mux[int_raddr_wire[STS_WIDTH-1:0]];

  generate
    for(j = 0; j < HUB_SIZE; j = j + 1)
    begin : MUXES
      assign int_rdata_mux[j+2] = int_svalid_wire[j] ? int_sdata_wire[j] : 32'd0;
      assign int_rdata_wire[j+2] = int_bsel_wire[j] ? int_bdata_wire[j] : 32'd0;
      assign int_mdata_wire[j] = int_wdata_wire;
      assign int_mvalid_wire[j] = int_wsel_wire[j+2];
      assign int_sready_wire[j] = int_rsel_wire[j+2];
    end
  endgenerate

  generate
    for(j = 0; j < MUX_SIZE; j = j + 1)
    begin : SELECTS
      assign int_wsel_wire[j] = int_we_wire & (int_awaddr_wire[27:24] == j);
      assign int_rsel_wire[j] = int_re_wire & (int_araddr_wire[27:24] == j);
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
    end
    else
    begin
      int_awcntr_reg <= int_awcntr_next;
      int_arcntr_reg <= int_arcntr_next;
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

  inout_buffer #(
    .DATA_WIDTH(44)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({s_axi_awid, s_axi_awaddr}),
    .in_valid(s_axi_awvalid), .in_ready(s_axi_awready),
    .out_data({int_awid_wire, int_awaddr_wire}),
    .out_valid(int_awvalid_wire), .out_ready(int_awready_wire)
  );

  inout_buffer #(
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

  inout_buffer #(
    .DATA_WIDTH(48)
  ) buf_3 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({s_axi_arid, s_axi_arlen, s_axi_araddr}),
    .in_valid(s_axi_arvalid), .in_ready(s_axi_arready),
    .out_data({int_arid_wire, int_arlen_wire, int_araddr_wire}),
    .out_valid(int_arvalid_wire), .out_ready(int_arready_wire)
  );

  output_buffer #(
    .DATA_WIDTH(HUB_SIZE + 45)
  ) buf_4 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({int_rsel_wire[MUX_SIZE-1:2] & ~int_svalid_wire, int_arid_wire, int_rlast_wire, int_rdata_wire[0]}),
    .in_valid(int_rvalid_wire), .in_ready(int_rready_wire),
    .out_data({int_bsel_wire, s_axi_rid, s_axi_rlast, int_rdata_wire[1]}),
    .out_valid(s_axi_rvalid), .out_ready(s_axi_rready)
  );

  assign s_axi_rdata = {{s_axi_rdata(hub_size)}};
{% for i in range(hub_size) -%}
{% set index =  "%02d" % i %}
  assign int_bdata_wire[{{i}}] = b{{index}}_bram_rdata;
  assign b{{index}}_bram_clk = aclk;
  assign b{{index}}_bram_rst = ~aresetn;
  assign b{{index}}_bram_en = int_rsel_wire[{{i+2}}] | int_wsel_wire[{{i+2}}];
  assign b{{index}}_bram_we = int_wsel_wire[{{i+2}}] ? int_wstrb_wire : 4'd0;
  assign b{{index}}_bram_addr = int_we_wire ? int_waddr_wire : int_raddr_wire;
  assign b{{index}}_bram_wdata = int_wdata_wire;

  assign int_sdata_wire[{{i}}] = s{{index}}_axis_tdata;
  assign int_svalid_wire[{{i}}] = s{{index}}_axis_tvalid;
  assign s{{index}}_axis_tready = int_sready_wire[{{i}}];

  inout_buffer #(
    .DATA_WIDTH(32)
  ) mbuf_{{i}} (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(int_mdata_wire[{{i}}]), .in_valid(int_mvalid_wire[{{i}}]), .in_ready(int_mready_wire[{{i}}]),
    .out_data(m{{index}}_axis_tdata), .out_valid(m{{index}}_axis_tvalid), .out_ready(m{{index}}_axis_tready)
  );
{% endfor %}
endmodule
"""

import jinja2


def s_axi_rdata(n):
    return " | ".join(map(lambda i: "int_rdata_wire[%d]" % i, range(1, n + 2)))


print(jinja2.Template(source).render(hub_size=hub_size, s_axi_rdata=s_axi_rdata))
