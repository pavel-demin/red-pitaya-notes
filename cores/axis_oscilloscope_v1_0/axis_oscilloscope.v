
`timescale 1 ns / 1 ps

module axis_oscilloscope #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 12
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire                        run_flag,
  input  wire                        trg_flag,

  input  wire [CNTR_WIDTH-1:0]       pre_data,
  input  wire [CNTR_WIDTH-1:0]       tot_data,

  output wire [CNTR_WIDTH:0]         sts_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // Master side
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  reg [CNTR_WIDTH-1:0] int_addr_reg, int_addr_next;
  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg [1:0] int_case_reg, int_case_next;
  reg int_enbl_reg, int_enbl_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(CNTR_WIDTH){1'b0}};
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_case_reg <= 2'd0;
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_cntr_reg <= int_cntr_next;
      int_case_reg <= int_case_next;
      int_enbl_reg <= int_enbl_next;
    end
  end

  always @*
  begin
    int_addr_next = int_addr_reg;
    int_cntr_next = int_cntr_reg;
    int_case_next = int_case_reg;
    int_enbl_next = int_enbl_reg;

    case(int_case_reg)
      // idle
      0:
      begin
        if(run_flag)
        begin
          int_addr_next = {(CNTR_WIDTH){1'b0}};
          int_cntr_next = {(CNTR_WIDTH){1'b0}};
          int_case_next = 2'd1;
          int_enbl_next = 1'b1;
        end
      end

      // pre-trigger recording
      1:
      begin
        if(s_axis_tvalid)
        begin
          int_cntr_next = int_cntr_reg + 1'b1;
          if(int_cntr_reg == pre_data)
          begin
            int_case_next = 2'd2;
          end
        end
      end

      // pre-trigger recording
      2:
      begin
        if(s_axis_tvalid)
        begin
          int_cntr_next = int_cntr_reg + 1'b1;
        end
        if(trg_flag)
        begin
          int_addr_next = int_cntr_reg;
          int_cntr_next = pre_data + int_cntr_reg[5:0];
          int_case_next = 2'd3;
        end
      end

      // post-trigger recording
      3:
      begin
        if(s_axis_tvalid)
        begin
          if(int_cntr_reg < tot_data)
          begin
            int_cntr_next = int_cntr_reg + 1'b1;
          end
          else
          begin
            int_case_next = 2'd0;
            int_enbl_next = 1'b0;
          end
        end
      end
    endcase
  end

  assign sts_data = {int_addr_reg, int_enbl_reg};

  assign s_axis_tready = 1'b1;
  assign m_axis_tdata = s_axis_tdata;
  assign m_axis_tvalid = int_enbl_reg & s_axis_tvalid;

endmodule
