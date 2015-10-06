
`timescale 1 ns / 1 ps

module axis_timer #
(
  parameter integer CNTR_WIDTH = 64
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire                        run_flag,
  input  wire                        cfg_flag,
  input  wire [CNTR_WIDTH-1:0]       cfg_data,

  output wire                        trg_flag,
  output wire [CNTR_WIDTH-1:0]       sts_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire                        s_axis_tvalid
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  wire int_enbl_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
    end
  end

  assign int_enbl_wire = run_flag & (int_cntr_reg > {(CNTR_WIDTH){1'b0}});

  always @*
  begin
    int_cntr_next = int_cntr_reg;

    if(cfg_flag)
    begin
      int_cntr_next = cfg_data;
    end
    else if(int_enbl_wire & s_axis_tvalid)
    begin
      int_cntr_next = int_cntr_reg - 1'b1;
    end
  end

  assign trg_data = int_enbl_wire;
  assign sts_data = int_cntr_reg;

  assign s_axis_tready = 1'b1;

endmodule
