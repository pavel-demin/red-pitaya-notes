
`timescale 1 ns / 1 ps

module axis_ram_writer #
(
  parameter integer ADDR_WIDTH = 20,
  parameter integer ADDR_BASE = 32'h1E000000,
  parameter integer AXI_ID_WIDTH = 6,
  parameter integer AXI_ADDR_WIDTH = 32,
  parameter integer AXI_DATA_WIDTH = 64,
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input wire  aclk,
  input wire  aresetn,

  // Master side
  output wire [AXI_ID_WIDTH-1:0]     m_axi_awid,    // AXI master: Write address ID
  output wire [AXI_ADDR_WIDTH-1:0]   m_axi_awaddr,  // AXI master: Write address
  output wire [3:0]                  m_axi_awlen,   // AXI master: Write burst length
  output wire [2:0]                  m_axi_awsize,  // AXI master: Write burst size
  output wire [1:0]                  m_axi_awburst, // AXI master: Write burst type
  output wire [1:0]                  m_axi_awlock,  // AXI master: Write lock type
  output wire [3:0]                  m_axi_awcache, // AXI master: Write memory type
  output wire [2:0]                  m_axi_awprot,  // AXI master: Write protection type
  output wire [3:0]                  m_axi_awqos,   // AXI master: Write quality of service
  output wire                        m_axi_awvalid, // AXI master: Write address valid
  input  wire                        m_axi_awready, // AXI master: Write address ready
  output wire [AXI_ID_WIDTH-1:0]     m_axi_wid,     // AXI master: Write data ID
  output wire [AXI_DATA_WIDTH-1:0]   m_axi_wdata,   // AXI master: Write data
  output wire [AXI_DATA_WIDTH/8-1:0] m_axi_wstrb,   // AXI master: Write strobes
  output wire                        m_axi_wlast,   // AXI master: Write last
  output wire                        m_axi_wvalid,  // AXI master: Write valid
  input  wire                        m_axi_wready,  // AXI master: Write ready
  input  wire [AXI_ID_WIDTH-1:0]     m_axi_bid,     // AXI master: Write response ID
  input  wire [1:0]                  m_axi_bresp,   // AXI master: Write response
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

  reg int_awvalid_reg, int_awvalid_next;
  reg int_wvalid_reg, int_wvalid_next;
  reg int_wstate_reg, int_wstate_next;
  reg [ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [AXI_ID_WIDTH-1:0] int_wid_reg, int_wid_next;

  wire int_wlast_wire;

  assign int_wlast_wire = &int_addr_reg[3:0];

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_awvalid_reg <= 1'b0;
      int_wvalid_reg <= 1'b0;
      int_wstate_reg <= 1'b0;
      int_addr_reg <= {(ADDR_WIDTH){1'b0}};
      int_wid_reg <= {(AXI_ID_WIDTH){1'b0}};
    end
    else
    begin
      int_awvalid_reg <= int_awvalid_next;
      int_wvalid_reg <= int_wvalid_next;
      int_wstate_reg <= int_wstate_next;
      int_addr_reg <= int_addr_next;
      int_wid_reg <= int_wid_next;
    end
  end

  always @*
  begin
    int_awvalid_next = int_awvalid_reg;
    int_wvalid_next = int_wvalid_reg;
    int_wstate_next = int_wstate_reg;
    int_addr_next = int_addr_reg;
    int_wid_next = int_wid_reg;

    case(int_wstate_reg)
      0:
      begin
        int_awvalid_next = 1'b1;
        if(m_axi_awready & int_awvalid_reg)
        begin
          int_awvalid_next = 1'b0;
          int_wvalid_next = 1'b1;
          int_wstate_next = 1'b1;
        end
      end
      1:
      begin
        if(m_axi_wready)
        begin
          int_addr_next = int_addr_reg + 1'b1;
          if(int_wlast_wire)
          begin
            int_wid_next = int_wid_reg + 1'b1;
            int_awvalid_next = 1'b1;
            int_wvalid_next = 1'b0;
            int_wstate_next = 1'b0;
          end
        end
      end
    endcase
  end

  assign m_axi_awid = int_wid_reg;
  assign m_axi_awaddr = ADDR_BASE + {int_addr_reg, 3'd0};
  assign m_axi_awlen = 4'd15;
  assign m_axi_awsize = clogb2((AXI_DATA_WIDTH/8)-1);
  assign m_axi_awburst = 2'b01;
  assign m_axi_awlock = 2'd0;
  assign m_axi_awcache = 4'b0010;
  assign m_axi_awprot = 3'd0;
  assign m_axi_awqos = 4'd0;
  assign m_axi_awvalid = int_awvalid_reg;
  assign m_axi_wid = int_wid_reg;
  assign m_axi_wdata = {{(AXI_DATA_WIDTH-ADDR_WIDTH){1'b0}}, int_addr_reg};
  assign m_axi_wstrb = {(AXI_DATA_WIDTH/8){1'b1}};
  assign m_axi_wlast = int_wlast_wire;
  assign m_axi_wvalid = int_wvalid_reg;
  assign m_axi_bready = 1'b1;

endmodule
