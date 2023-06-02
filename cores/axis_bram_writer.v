
`timescale 1 ns / 1 ps

module axis_bram_writer #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 10
)
(
  // System signals
  input  wire                         aclk,
  input  wire                         aresetn,

  output wire [BRAM_ADDR_WIDTH-1:0]   sts_data,

  // Slave side
  input  wire [AXIS_TDATA_WIDTH-1:0]  s_axis_tdata,
  input  wire                         s_axis_tvalid,
  output wire                         s_axis_tready,

  // BRAM port
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram CLK" *)
  output wire                         b_bram_clk,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram RST" *)
  output wire                         b_bram_rst,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram EN" *)
  output wire                         b_bram_en,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram WE" *)
  output wire [BRAM_DATA_WIDTH/8-1:0] b_bram_we,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram ADDR" *)
  output wire [BRAM_ADDR_WIDTH-1:0]   b_bram_addr,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram DIN" *)
  output wire [BRAM_DATA_WIDTH-1:0]   b_bram_wdata
);

  reg [BRAM_ADDR_WIDTH-1:0] int_addr_reg;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
    end
    else if(s_axis_tvalid)
    begin
      int_addr_reg <= int_addr_reg + 1'b1;
    end
  end

  assign sts_data = int_addr_reg;

  assign s_axis_tready = 1'b1;

  assign b_bram_clk = aclk;
  assign b_bram_rst = ~aresetn;
  assign b_bram_en = s_axis_tvalid;
  assign b_bram_we = {(BRAM_DATA_WIDTH/8){s_axis_tvalid}};
  assign b_bram_addr = int_addr_reg;
  assign b_bram_wdata = s_axis_tdata;

endmodule
