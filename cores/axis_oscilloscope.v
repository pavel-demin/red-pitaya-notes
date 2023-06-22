
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
  reg int_run_reg, int_run_next;
  reg int_pre_reg, int_pre_next;
  reg int_trg_reg, int_trg_next;
  reg int_tot_reg, int_tot_next;

  wire int_valid_wire, int_last_wire;

  assign int_valid_wire = int_run_reg & s_axis_tvalid;
  assign int_last_wire = int_cntr_reg == tot_data;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_addr_reg <= {(CNTR_WIDTH){1'b0}};
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_run_reg <= 1'b0;
      int_pre_reg <= 1'b0;
      int_trg_reg <= 1'b0;
      int_tot_reg <= 1'b0;
    end
    else
    begin
      int_addr_reg <= int_addr_next;
      int_cntr_reg <= int_cntr_next;
      int_run_reg <= int_run_next;
      int_pre_reg <= int_pre_next;
      int_trg_reg <= int_trg_next;
      int_tot_reg <= int_tot_next;
    end
  end

  always @*
  begin
    int_addr_next = int_addr_reg;
    int_cntr_next = int_cntr_reg;
    int_run_next = int_run_reg;
    int_pre_next = int_pre_reg;
    int_trg_next = int_trg_reg;
    int_tot_next = int_tot_reg;

    if(~int_run_reg & run_flag)
    begin
      int_addr_next = {(CNTR_WIDTH){1'b0}};
      int_cntr_next = {(CNTR_WIDTH){1'b0}};
      int_run_next = 1'b1;
      int_pre_next = 1'b0;
      int_trg_next = 1'b0;
      int_tot_next = 1'b0;
    end

    if(int_run_reg & int_pre_reg & trg_flag)
    begin
      int_trg_next = 1'b1;
    end

    if(int_valid_wire)
    begin
      int_cntr_next = int_last_wire ? {(CNTR_WIDTH){1'b0}} : int_cntr_reg + 1'b1;

      if(int_cntr_reg == pre_data)
      begin
        int_pre_next = 1'b1;
      end

      if(~int_tot_reg & int_trg_reg)
      begin
        int_addr_next = int_cntr_reg;
        int_cntr_next = pre_data + int_cntr_reg[5:0];
        int_tot_next = 1'b1;
      end

      if(int_tot_reg & int_last_wire)
      begin
        int_run_next = 1'b0;
      end
    end
  end

  assign sts_data = {int_addr_reg, int_run_reg};

  assign s_axis_tready = 1'b1;
  assign m_axis_tdata = s_axis_tdata;
  assign m_axis_tvalid = int_valid_wire;

endmodule
