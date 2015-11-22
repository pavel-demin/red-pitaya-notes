
`timescale 1 ns / 1 ps

module axi_axis_reader #
(
  parameter integer AXI_DATA_WIDTH = 32
)
(
  // System signals
  input  wire                      aclk,
  input  wire                      aresetn,

  // Slave side
  input  wire                      s_axi_arvalid, // AXI4-Lite slave: Read address valid
  output wire                      s_axi_arready, // AXI4-Lite slave: Read address ready
  output wire [AXI_DATA_WIDTH-1:0] s_axi_rdata,   // AXI4-Lite slave: Read data
  output wire [1:0]                s_axi_rresp,   // AXI4-Lite slave: Read data response
  output wire                      s_axi_rvalid,  // AXI4-Lite slave: Read data valid
  input  wire                      s_axi_rready,   // AXI4-Lite slave: Read data ready

  // Slave side
  output wire                      s_axis_tready,
  input  wire [AXI_DATA_WIDTH-1:0] s_axis_tdata,
  input  wire                      s_axis_tvalid
);

  reg int_ready_reg, int_ready_next;
  reg int_valid_reg, int_valid_next;
  reg [AXI_DATA_WIDTH-1:0] int_tdata_reg, int_tdata_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_ready_reg <= 1'b0;
      int_valid_reg <= 1'b0;
      int_tdata_reg <= {(AXI_DATA_WIDTH){1'b0}};
    end
    else
    begin
      int_ready_reg <= int_ready_next;
      int_valid_reg <= int_valid_next;
      int_tdata_reg <= int_tdata_next;
    end
  end

  always @*
  begin
    int_ready_next = int_ready_reg;
    int_valid_next = int_valid_reg;
    int_tdata_next = int_tdata_reg;

    if(s_axi_arvalid & ~int_valid_reg)
    begin
      int_ready_next = 1'b1;
      int_valid_next = 1'b1;
      int_tdata_next = s_axis_tvalid ? s_axis_tdata : {(AXI_DATA_WIDTH){1'b0}};
    end

    if(int_ready_reg)
    begin
      int_ready_next = 1'b0;
    end

    if(s_axi_rready & int_valid_reg)
    begin
      int_valid_next = 1'b0;
    end
  end

  assign s_axi_rresp = 2'd0;

  assign s_axi_arready = int_ready_reg;
  assign s_axi_rdata = int_tdata_reg;
  assign s_axi_rvalid = int_valid_reg;

  assign s_axis_tready = int_ready_reg;

endmodule
