
`timescale 1 ns / 1 ps

module axis_histogram #
(
  parameter integer BRAM_ADDR_WIDTH = 32,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                           aclk,
  input  wire                           aresetn,

  // Slave side
  output wire                           s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0]    s_axis_tdata,
  input  wire                           s_axis_tvalid,

  // Master side
  input  wire                           m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0]    m_axis_tdata,
  output wire                           m_axis_tvalid,

  // BRAM port A
  output wire                           bram_porta_clk,
  output wire                           bram_porta_rst,
  output wire [BRAM_ADDR_WIDTH-1:0]     bram_porta_addr,
  output wire [BRAM_DATA_WIDTH-1:0]     bram_porta_wrdata,
  input  wire [BRAM_DATA_WIDTH-1:0]     bram_porta_rddata,
  output wire                           bram_porta_en,
  output wire [(BRAM_DATA_WIDTH/8)-1:0] bram_porta_we,

  // BRAM port B
  output wire                           bram_portb_clk,
  output wire                           bram_portb_rst,
  output wire [BRAM_ADDR_WIDTH-1:0]     bram_portb_addr,
  output wire [BRAM_DATA_WIDTH-1:0]     bram_portb_wrdata,
  input  wire [BRAM_DATA_WIDTH-1:0]     bram_portb_rddata,
  output wire                           bram_portb_en,
  output wire [(BRAM_DATA_WIDTH/8)-1:0] bram_portb_we
);

  reg [BRAM_ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [BRAM_ADDR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg [BRAM_DATA_WIDTH-1:0] int_data_reg, int_data_next;
  reg [(BRAM_DATA_WIDTH/8)-1:0] int_we_reg, int_we_next;
  reg [1:0] int_case_reg, int_case_next;
  reg int_en_reg, int_en_next;
  reg int_tready_reg, int_tready_next;

  always @(posedge clock)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_cntr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_data_reg <= {(BRAM_DATA_WIDTH){1'b0}};
      int_en_reg <= 1'b1;
      int_we_reg <= {(BRAM_DATA_WIDTH/8){1'b1}};
      int_case_reg <= 2'd0;
      int_tready_reg <= 1'b0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_cntr_reg <= int_cntr_next;
      int_data_reg <= int_data_next;
      int_en_reg <= int_en_next;
      int_we_reg <= int_we_next;
      int_case_reg <= int_case_next;
      int_tready_reg <= int_tready_next;
    end
  end

  always @*
  begin
    int_addr_next = int_addr_reg;
    int_cntr_next = int_cntr_reg;
    int_data_next = int_data_reg;
    int_en_next = int_en_reg;
    int_we_next = int_we_reg;
    int_case_next = int_case_reg;
    int_tready_next = int_tready_reg;

    if(m_axis_tready)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end

    case(int_case_reg)
      0:
      begin
        // write zeros
        int_addr_next = int_addr_reg + 1'b1;
        if(&int_addr_reg)
        begin
          int_en_next = 1'b0;
          int_we_next = {(BRAM_DATA_WIDTH/8){1'b0}};
          int_case_next = 2'd1;
          int_tready_next = 1'b1;
        end
      end

      1:
      begin
        int_en_next = 1'b0;
        int_we_next = {(BRAM_DATA_WIDTH/8){1'b0}};
        int_tready_next = 1'b1;
        if(s_axis_tvalid)
        begin
          int_addr_next = s_axis_tdata;
          int_en_next = 1'b1;
          int_case_next = 2'd2;
          int_tready_next = 1'b0;
        end
      end

      2:
      begin
        int_case_next = 2'd3;
      end

			3:
			begin
				int_case_next = 2'd1;
				if(~&bram_porta_rddata)
				begin
					int_we_next = {(BRAM_DATA_WIDTH/8){1'b1}};
					int_data_next = bram_porta_rddata + 1'b1;
				end
			end
    endcase
  end

  assign s_axis_tready = int_tready_reg;

  assign m_axis_tdata = bram_portb_rddata;
  assign m_axis_tvalid = 1'b1;

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_addr = int_addr_reg;
  assign bram_porta_wrdata = int_data_reg;
  assign bram_porta_en = int_en_reg;
  assign bram_porta_we = int_we_reg;

  assign bram_portb_clk = aclk;
  assign bram_portb_rst = ~aresetn;
  assign bram_portb_addr = int_cntr_reg;
  assign bram_portb_wrdata = {(BRAM_DATA_WIDTH){1'b0}};
  assign bram_portb_en = 1'b1;
  assign bram_portb_we = {(BRAM_DATA_WIDTH/8){1'b0}};

endmodule
