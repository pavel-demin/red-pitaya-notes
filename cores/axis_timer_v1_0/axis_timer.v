
`timescale 1 ns / 1 ps

module axis_timer #
(
  parameter integer CNTR_WIDTH = 64
)
(
  // System signals
  input  wire                  aclk,
  input  wire                  aresetn,

  input  wire                  run_flag,
  input  wire [CNTR_WIDTH-1:0] cfg_data,

  output wire                  trg_flag,
  output wire [CNTR_WIDTH-1:0] sts_data,

  // Slave side
  output wire                  s_axis_tready,
  input  wire                  s_axis_tvalid
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_enbl_reg, int_enbl_next;

  wire int_comp_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_enbl_reg <= 1'b0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_enbl_reg <= int_enbl_next;
    end
  end

  assign int_comp_wire = run_flag & (int_cntr_reg < cfg_data);

  always @*
  begin
    int_cntr_next = int_cntr_reg;
    int_enbl_next = int_enbl_reg;

    if(~int_enbl_reg & int_comp_wire & s_axis_tvalid)
    begin
      int_enbl_next = 1'b1;
    end

    if(int_enbl_reg & int_comp_wire & s_axis_tvalid)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end

    if(int_enbl_reg & ~int_comp_wire & s_axis_tvalid)
    begin
      int_enbl_next = 1'b0;
    end
  end

  assign trg_flag = int_enbl_reg;
  assign sts_data = int_cntr_reg;

  assign s_axis_tready = 1'b1;

endmodule
