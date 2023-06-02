
`timescale 1 ns / 1 ps

module axis_histogram #
(
  parameter integer AXIS_TDATA_WIDTH = 16,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 14
)
(
  // System signals
  input  wire                         aclk,
  input  wire                         aresetn,

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
  output wire [BRAM_DATA_WIDTH-1:0]   b_bram_wdata,
  (* X_INTERFACE_INFO = "xilinx.com:interface:bram:1.0 b_bram DOUT" *)
  input  wire [BRAM_DATA_WIDTH-1:0]   b_bram_rdata
);

  reg [BRAM_ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [BRAM_DATA_WIDTH-1:0] int_data_reg, int_data_next;
  reg [1:0] int_case_reg, int_case_next;
  reg int_tready_reg, int_tready_next;
  reg int_we_reg, int_we_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_data_reg <= {(BRAM_DATA_WIDTH){1'b0}};
      int_case_reg <= 2'd0;
      int_tready_reg <= 1'b1;
      int_we_reg <= 1'b0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_data_reg <= int_data_next;
      int_case_reg <= int_case_next;
      int_tready_reg <= int_tready_next;
      int_we_reg <= int_we_next;
    end
  end

  always @*
  begin
    int_addr_next = int_addr_reg;
    int_data_next = int_data_reg;
    int_case_next = int_case_reg;
    int_tready_next = int_tready_reg;
    int_we_next = int_we_reg;

    case(int_case_reg)
      2'd0:
      begin
        if(s_axis_tvalid)
        begin
          int_addr_next = s_axis_tdata[BRAM_ADDR_WIDTH-1:0];
          int_tready_next = 1'b0;
          int_case_next = 2'd1;
        end
      end
      2'd1:
      begin
        int_data_next = b_bram_rdata + 1'b1;
        int_we_next = ~&b_bram_rdata;
        int_case_next = 2'd2;
      end
      2'd2:
      begin
        int_tready_next = 1'b1;
        int_we_next = 1'b0;
        int_case_next = 2'd0;
      end
    endcase
  end

  assign s_axis_tready = int_tready_reg;

  assign b_bram_clk = aclk;
  assign b_bram_rst = ~aresetn;
  assign b_bram_en = int_we_reg | s_axis_tvalid;
  assign b_bram_we = {(BRAM_DATA_WIDTH/8){int_we_reg}};
  assign b_bram_addr = int_we_reg ? int_addr_reg : s_axis_tdata[BRAM_ADDR_WIDTH-1:0];
  assign b_bram_wdata = int_data_reg;

endmodule
