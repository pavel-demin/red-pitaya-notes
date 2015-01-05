
`timescale 1 ns / 1 ps

module axi_sts_register #
(
  parameter integer STS_DATA_WIDTH = 1024,
  parameter integer AXI_DATA_WIDTH = 32,
  parameter integer AXI_ADDR_WIDTH = 7
)
(
  // System signals
  input  wire                          aclk,
  input  wire                          aresetn,

  // Configuration bits
  input  wire [STS_DATA_WIDTH-1:0]     sts_data,

  // Slave side
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
  localparam integer STS_SIZE = STS_DATA_WIDTH/AXI_DATA_WIDTH;

  reg [AXI_ADDR_WIDTH-1:0] int_araddr_reg, int_araddr_next;
  reg int_arready_reg, int_arready_next;
  reg int_rvalid_reg, int_rvalid_next;

  reg int_rstate_reg, int_rstate_next;

  wire [AXI_DATA_WIDTH-1:0] int_data_mux [STS_SIZE-1:0];

  integer i;
  genvar j, k, l;

  generate
    for (j = 0; j < STS_SIZE; j = j + 1)
    begin : WORDS
      assign int_data_mux[j] = sts_data[j*STS_SIZE+31:j*STS_SIZE];
      for (k = 0; k < AXI_DATA_WIDTH/8; k = k + 1)
      begin : BYTES
        for (l = 0; l < 8; l = l + 1)
        begin : BITS
          FDRE #(
            .INIT(1'b0)
          ) FDRE_inst (
            .CE(1'b1),
            .C(aclk),
            .R(aresetn),
            .D(sts_data[j*AXI_DATA_WIDTH + k*8 + l]),
            .Q(s_axi_rdata[k*8 + l])
          );
        end
      end
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_arready_reg <= 1'b0;
      int_araddr_reg <= {(AXI_ADDR_WIDTH){1'b0}};
      int_rvalid_reg <= 1'b0;
      int_rdata_reg <= {(AXI_DATA_WIDTH){1'b0}};
      int_rstate_reg <= 1'b0;
    end
    else
    begin
      int_arready_reg <= int_arready_next;
      int_araddr_reg <= int_araddr_next;
      int_rvalid_reg <= int_rvalid_next;
      int_rdata_reg <= int_rdata_next;
      int_rstate_reg <= int_rstate_next;
    end
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

  assign s_axi_rresp = 2'b0;

  assign s_axi_arready = int_arready_reg;
  assign s_axi_rdata = int_wdata_reg;
  assign s_axi_rvalid = int_rvalid_reg;

endmodule
