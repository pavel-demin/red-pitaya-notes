
`timescale 1 ns / 1 ps

module axis_keyer #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 10
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [BRAM_ADDR_WIDTH-1:0]  cfg_data,
  input  wire                        key_flag,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,
  input  wire                        m_axis_tready,

  // BRAM port
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram CLK" *)
  output wire                        b_bram_clk,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram RST" *)
  output wire                        b_bram_rst,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram EN" *)
  output wire                        b_bram_en,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram ADDR" *)
  output wire [BRAM_ADDR_WIDTH-1:0]  b_bram_addr,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram DOUT" *)
  input  wire [BRAM_DATA_WIDTH-1:0]  b_bram_rdata
);

  reg [BRAM_ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [BRAM_ADDR_WIDTH-1:0] int_data_reg;
  reg int_enbl_reg, int_enbl_next;

  wire [1:0] int_valid_wire, int_ready_wire;
  wire [1:0] int_comp_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_data_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_data_reg <= cfg_data;
      int_enbl_reg <= int_enbl_next;
    end
  end


  assign int_comp_wire = {|int_addr_reg, int_addr_reg < int_data_reg};

  assign int_valid_wire[0] = 1'b1;

  always @*
  begin
    int_addr_next = int_addr_reg;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg & ~int_comp_wire[1] & key_flag)
    begin
      int_enbl_next = 1'b1;
    end

    if(int_ready_wire[0] & int_enbl_reg & int_comp_wire[0])
    begin
      int_addr_next = int_addr_reg + 1'b1;
    end

    if(int_ready_wire[0] & int_enbl_reg & ~int_comp_wire[0] & ~key_flag)
    begin
      int_enbl_next = 1'b0;
    end

    if(int_ready_wire[0] & ~int_enbl_reg & int_comp_wire[1])
    begin
      int_addr_next = int_addr_reg - 1'b1;
    end
  end

  output_buffer #(
    .DATA_WIDTH(0)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_valid(int_valid_wire[0]), .in_ready(int_ready_wire[0]),
    .out_valid(int_valid_wire[1]), .out_ready(int_ready_wire[1])
  );

  inout_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH)
  ) buf_1 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(b_bram_rdata), .in_valid(int_valid_wire[1]), .in_ready(int_ready_wire[1]),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

  assign b_bram_clk = aclk;
  assign b_bram_rst = ~aresetn;
  assign b_bram_en = int_ready_wire[0];
  assign b_bram_addr = int_addr_reg;

endmodule
