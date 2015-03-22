
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
  output wire                         s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0]  s_axis_tdata,
  input  wire                         s_axis_tvalid,

  // BRAM port
  output wire                         bram_porta_clk,
  output wire                         bram_porta_rst,
  output wire [BRAM_ADDR_WIDTH-1:0]   bram_porta_addr,
  output wire [BRAM_DATA_WIDTH-1:0]   bram_porta_wrdata,
  output wire [BRAM_DATA_WIDTH/8-1:0] bram_porta_we
);

  reg [BRAM_ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg int_enbl_reg, int_enbl_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_enbl_reg <= int_enbl_next;
    end
  end

  always @*
  begin
    int_addr_next = int_addr_reg;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg)
    begin
      int_enbl_next = 1'b1;
    end

    if(s_axis_tvalid & int_enbl_reg)
    begin
      int_addr_next = int_addr_reg + 1'b1;
    end
  end

  assign sts_data = int_addr_reg;

  assign s_axis_tready = int_enbl_reg;

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_addr = int_addr_reg;
  assign bram_porta_wrdata = s_axis_tdata;
  assign bram_porta_we = {(BRAM_DATA_WIDTH/8){s_axis_tvalid & int_enbl_reg}};

endmodule
