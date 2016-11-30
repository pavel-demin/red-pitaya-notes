
`timescale 1 ns / 1 ps

module axis_keyer #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer BRAM_DATA_WIDTH = 32,
  parameter integer BRAM_ADDR_WIDTH = 10
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [BRAM_ADDR_WIDTH-1:0]  cfg_data,
  input  wire                        key_flag,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,

  // BRAM port
  output wire                        bram_porta_clk,
  output wire                        bram_porta_rst,
  output wire [BRAM_ADDR_WIDTH-1:0]  bram_porta_addr,
  input  wire [BRAM_DATA_WIDTH-1:0]  bram_porta_rddata
);

  reg [BRAM_ADDR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [BRAM_ADDR_WIDTH-1:0] int_data_reg;
  reg [1:0] int_case_reg, int_case_next;

  wire [1:0] int_comp_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_data_reg <= {(BRAM_ADDR_WIDTH){1'b0}};
      int_case_reg <= 2'd0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_data_reg <= cfg_data;
      int_case_reg <= int_case_next;
    end
  end

  assign int_comp_wire = {|int_addr_reg, int_addr_reg < int_data_reg};

  always @*
  begin
    int_addr_next = int_addr_reg;
    int_case_next = int_case_reg;

    case(int_case_reg)
      2'd0:
      begin
        if(key_flag & int_comp_wire[0])
        begin
          int_case_next = 2'd1;
        end
      end
      2'd1:
      begin
        if(m_axis_tready)
        begin
          if(int_comp_wire[0])
          begin
            int_addr_next = int_addr_reg + 1'b1;
          end
          else
          begin
            int_case_next = 2'd2;
          end
        end
      end
      2'd2:
      begin
        if(~key_flag)
        begin
          int_case_next = 2'd3;
        end
      end
      2'd3:
      begin
        if(m_axis_tready)
        begin
          if(int_comp_wire[1])
          begin
            int_addr_next = int_addr_reg - 1'b1;
          end
          else
          begin
            int_case_next = 2'd0;
          end
        end
      end
    endcase
  end

  assign m_axis_tdata = bram_porta_rddata;
  assign m_axis_tvalid = 1'b1;

  assign bram_porta_clk = aclk;
  assign bram_porta_rst = ~aresetn;
  assign bram_porta_addr = m_axis_tready ? int_addr_next : int_addr_reg;

endmodule
