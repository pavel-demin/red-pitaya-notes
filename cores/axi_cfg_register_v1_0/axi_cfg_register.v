
`timescale 1 ns / 1 ps

module axi_cfg_register #
(
  parameter integer CFG_DATA_WIDTH = 1024,
  parameter integer AXI_DATA_WIDTH = 32,
  parameter integer AXI_ADDR_WIDTH = 7
)
(
  // System signals
  input  wire                          aclk,
  input  wire                          aresetn,

  // Configuration bits
  output wire [CFG_DATA_WIDTH-1:0]     cfg_data,

  // Slave side
  input  wire [AXI_ADDR_WIDTH-1:0]     s_axi_awaddr,  // AXI4-Lite slave: Write address
  input  wire                          s_axi_awvalid, // AXI4-Lite slave: Write address valid
  output wire                          s_axi_awready, // AXI4-Lite slave: Write address ready
  input  wire [AXI_DATA_WIDTH-1:0]     s_axi_wdata,   // AXI4-Lite slave: Write data
  input  wire [(AXI_DATA_WIDTH/8)-1:0] s_axi_wstrb,   // AXI4-Lite slave: Write strobe
  input  wire                          s_axi_wvalid,  // AXI4-Lite slave: Write data valid
  output wire                          s_axi_wready,  // AXI4-Lite slave: Write data ready
  output wire [1:0]                    s_axi_bresp,   // AXI4-Lite slave: Write response
  output wire                          s_axi_bvalid,  // AXI4-Lite slave: Write response valid
  input  wire                          s_axi_bready,  // AXI4-Lite slave: Write response ready
  input  wire [AXI_ADDR_WIDTH-1:0]     s_axi_araddr,  // AXI4-Lite slave: Read address
  input  wire                          s_axi_arvalid, // AXI4-Lite slave: Read address valid
  output wire                          s_axi_arready, // AXI4-Lite slave: Read address ready
  output wire [AXI_DATA_WIDTH-1:0]     s_axi_rdata,   // AXI4-Lite slave: Read data
  output wire [1:0]                    s_axi_rresp,   // AXI4-Lite slave: Read data response
  output wire                          s_axi_rvalid,  // AXI4-Lite slave: Read data valid
  input  wire                          s_axi_rready   // AXI4-Lite slave: Read data ready
);

  localparam integer ADDR_LSB = (AXI_DATA_WIDTH/32) + 1;
  localparam integer ADDR_BITS = AXI_ADDR_WIDTH - ADDR_LSB;
  localparam integer CFG_SIZE = CFG_DATA_WIDTH/AXI_DATA_WIDTH;

  reg [AXI_ADDR_WIDTH-1:0] int_awaddr_reg, int_awaddr_next;
  reg [AXI_ADDR_WIDTH-1:0] int_araddr_reg, int_araddr_next;
  reg [AXI_DATA_WIDTH-1:0] int_rdata_reg, int_rdata_next;
  reg int_awready_reg, int_awready_next;
  reg int_wready_reg, int_wready_next;
  reg int_arready_reg, int_arready_next;
  reg int_bvalid_reg, int_bvalid_next;
  reg int_rvalid_reg, int_rvalid_next;

  reg int_wren_reg, int_wren_next;
  reg int_wstate_reg, int_wstate_next;
  reg int_rstate_reg, int_rstate_next;

  wire [AXI_DATA_WIDTH-1:0] int_data_mux [CFG_SIZE-1:0];
  wire [CFG_DATA_WIDTH-1:0] int_data_wire;
  wire word_wren_wire, byte_wren_wire;

  integer i;
  genvar j, k, l;

  generate
    for (j = 0; j < CFG_SIZE; j = j + 1)
    begin : WORDS
      assign int_data_mux[j] = int_data_wire[j*CFG_SIZE+31:j*CFG_SIZE];
      assign word_wren_wire = int_wren_reg & (int_awaddr_reg[ADDR_LSB+ADDR_BITS-1:ADDR_LSB] == j);
      for (k = 0; k < AXI_DATA_WIDTH/8; k = k + 1)
      begin : BYTES
        assign byte_wren_wire = word_wren_wire & s_axi_wstrb[k];
        for (l = 0; l < 8; l = l + 1)
        begin : BITS
          FDRE #(
            .INIT(1'b0)
          ) FDRE_inst (
            .CE(byte_wren_wire),
            .C(aclk),
            .R(aresetn),
            .D(s_axi_wdata[k*8 + l]),
            .Q(int_data_wire[j*AXI_DATA_WIDTH + k*8 + l])
          );
        end
      end
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_awready_reg <= 1'b0;
      int_awaddr_reg <= {(AXI_ADDR_WIDTH){1'b0}};
      int_wready_reg <= 1'b0;
      int_bvalid_reg <= 1'b0;
      int_arready_reg <= 1'b0;
      int_araddr_reg <= {(AXI_ADDR_WIDTH){1'b0}};
      int_rvalid_reg <= 1'b0;
      int_rdata_reg <= {(AXI_DATA_WIDTH){1'b0}};
      int_wren_reg <= 1'b0;
      int_wstate_reg <= 1'b0;
      int_rstate_reg <= 1'b0;
    end
    else
    begin
      int_awready_reg <= int_awready_next;
      int_awaddr_reg <= int_awaddr_next;
      int_wready_reg <= int_wready_next;
      int_bvalid_reg <= int_bvalid_next;
      int_arready_reg <= int_arready_next;
      int_araddr_reg <= int_araddr_next;
      int_rvalid_reg <= int_rvalid_next;
      int_rdata_reg <= int_rdata_next;
      int_wren_reg <= int_wren_next;
      int_wstate_reg <= int_wstate_next;
      int_rstate_reg <= int_rstate_next;
    end
  end

  always @*
  begin
    int_awready_next = int_awready_reg;
    int_awaddr_next = int_awaddr_reg;
    int_wready_next = int_wready_reg;
    int_bvalid_next = int_bvalid_reg;
    int_wren_next = int_wren_reg;
    int_wstate_next = int_wstate_reg;

    case(int_wstate_reg)
      0:
      begin
        if(s_axi_awvalid & s_axi_wvalid)
        begin
          int_awready_next = 1'b1;
          int_awaddr_next = s_axi_awaddr;
          int_wready_next = 1'b1;
          int_wren_next = 1'b1;
          int_wstate_next = 1'b1;
        end
      end
      1:
      begin
        int_awready_next = 1'b0;
        int_wready_next = 1'b0;
        int_wren_next = 1'b0;
        int_bvalid_next = 1'b1;
        if(s_axi_bready & int_bvalid_reg)
        begin
          int_bvalid_next = 1'b0;
          int_wstate_next = 1'b0;
        end
      end
    endcase
  end

  always @*
  begin
    int_arready_next = int_arready_reg;
    int_araddr_next = int_araddr_reg;
    int_rvalid_next = int_rvalid_reg;
    int_rdata_next = int_rdata_reg;
    int_rstate_next = int_rstate_reg;

    case(int_rstate_reg)
      0:
      begin
        if(s_axi_arvalid)
        begin
          int_arready_next = 1'b1;
          int_araddr_next = s_axi_araddr;
          int_rstate_next = 1'b1;
        end
      end
      1:
      begin
        int_arready_next = 1'b0;
        int_rvalid_next = 1'b1;
        int_rdata_next = int_data_mux[int_araddr_reg[ADDR_LSB+ADDR_BITS-1:ADDR_LSB]];
        if(s_axi_rready & int_rvalid_reg)
        begin
          int_rvalid_next = 1'b0;
          int_rstate_next = 1'b0;
        end
      end
    endcase
  end

  assign cfg_data = int_data_wire;

  assign s_axi_bresp = 2'b0;
  assign s_axi_rresp = 2'b0;

  assign s_axi_awready = int_awready_reg;
  assign s_axi_wready = int_wready_reg;
  assign s_axi_bvalid = int_bvalid_reg;
  assign s_axi_arready = int_arready_reg;
  assign s_axi_rdata = int_rdata_reg;
  assign s_axi_rvalid = int_rvalid_reg;

endmodule
