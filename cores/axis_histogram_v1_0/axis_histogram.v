
`timescale 1 ns / 1 ps

module axis_histogram #
(
  parameter integer AXIS_TDATA_WIDTH = 16,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 14
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // BRAM port
  output wire                        bram_porta_clk,
  output wire                        bram_porta_rst,
  output wire [BRAM_ADDR_WIDTH-1:0]  bram_porta_addr,
  output wire [BRAM_DATA_WIDTH-1:0]  bram_porta_wrdata,
  input  wire [BRAM_DATA_WIDTH-1:0]  bram_porta_rddata,
  output wire                        bram_porta_we
);

  reg [BRAM_ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [BRAM_DATA_WIDTH-1:0] int_data_reg, int_data_next;
  reg [2:0] int_case_reg, int_case_next;
  reg int_tready_reg, int_tready_next;
  reg int_wren_reg, int_wren_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_data_reg <= {(BRAM_DATA_WIDTH){1'b0}};
      int_case_reg <= 3'd0;
      int_tready_reg <= 1'b0;
      int_wren_reg <= 1'b0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_data_reg <= int_data_next;
      int_case_reg <= int_case_next;
      int_tready_reg <= int_tready_next;
      int_wren_reg <= int_wren_next;
    end
  end

  always @*
  begin
    int_addr_next = int_addr_reg;
    int_data_next = int_data_reg;
    int_case_next = int_case_reg;
    int_tready_next = int_tready_reg;
    int_wren_next = int_wren_reg;

    case(int_case_reg)
      0:
      begin
        int_addr_next = {(BRAM_ADDR_WIDTH){1'b0}};
        int_data_next = {(BRAM_DATA_WIDTH){1'b0}};
        int_wren_next = 1'b1;
        int_case_next = 3'd1;
      end
      1:
      begin
        // write zeros
        int_addr_next = int_addr_reg + 1'b1;
        if(&int_addr_reg)
        begin
          int_tready_next = 1'b1;
          int_wren_next = 1'b0;
          int_case_next = 3'd2;
        end
      end
      2:
      begin
        if(s_axis_tvalid)
        begin
          int_addr_next = s_axis_tdata[BRAM_ADDR_WIDTH-1:0];
          int_tready_next = 1'b0;
          int_case_next = 3'd3;
        end
      end
      3:
      begin
        int_case_next = 3'd4;
      end
      4:
      begin
        int_data_next = bram_porta_rddata + 1'b1;
        int_wren_next = ~&bram_porta_rddata;
        int_case_next = 3'd5;
      end
      5:
      begin
        int_tready_next = 1'b1;
        int_wren_next = 1'b0;
        int_case_next = 3'd2;
      end
    endcase
  end

  assign s_axis_tready = int_tready_reg;

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_addr = int_wren_reg ? int_addr_reg : s_axis_tdata[BRAM_ADDR_WIDTH-1:0];
  assign bram_porta_wrdata = int_data_reg;
  assign bram_porta_we = int_wren_reg;

endmodule
