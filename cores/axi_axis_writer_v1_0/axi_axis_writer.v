
`timescale 1 ns / 1 ps

module axi_axis_writer #
(
  parameter integer AXI_DATA_WIDTH = 32
)
(
  // System signals
  input  wire                         aclk,
  input  wire                         aresetn,

  // Slave side
  input  wire                         s_axi_awvalid, // AXI4-Lite slave: Write address valid
  output wire                         s_axi_awready, // AXI4-Lite slave: Write address ready
  input  wire [AXI_DATA_WIDTH-1:0]    s_axi_wdata,   // AXI4-Lite slave: Write data
  input  wire                         s_axi_wvalid,  // AXI4-Lite slave: Write data valid
  output wire                         s_axi_wready,  // AXI4-Lite slave: Write data ready
  output wire [1:0]                   s_axi_bresp,   // AXI4-Lite slave: Write response
  output wire                         s_axi_bvalid,  // AXI4-Lite slave: Write response valid
  input  wire                         s_axi_bready,  // AXI4-Lite slave: Write response ready


  // Master side
  output wire [AXI_DATA_WIDTH-1:0]    m_axis_tdata,
  output wire                         m_axis_tvalid
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

    if(s_axi_awvalid & s_axi_wvalid & ~int_ready_reg)
    begin
      int_ready_next = 1'b1;
      int_tdata_next = s_axi_wdata;
    end

    if(int_ready_reg)
    begin
      int_ready_next = 1'b0;
      int_valid_next = 1'b1;
    end

    if(s_axi_bready & int_valid_reg)
    begin
      int_valid_next = 1'b0;
    end
  end

  assign s_axi_bresp = 2'd0;

  assign s_axi_awready = int_ready_reg;
  assign s_axi_wready = int_ready_reg;
  assign s_axi_bvalid = int_valid_reg;

  assign m_axis_tdata = int_tdata_reg;
  assign m_axis_tvalid = int_ready_reg;

endmodule
