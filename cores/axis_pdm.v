
`timescale 1 ns / 1 ps

module axis_pdm #
(
  parameter integer AXIS_TDATA_WIDTH = 16,
  parameter integer CNTR_WIDTH = 8
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [CNTR_WIDTH-1:0]       cfg_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  output wire                        dout
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg [AXIS_TDATA_WIDTH-1:0] int_data_reg, int_data_next;
  reg [AXIS_TDATA_WIDTH:0] int_acc_reg, int_acc_next;
  reg int_tready_reg, int_tready_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_data_reg <= {(AXIS_TDATA_WIDTH){1'b0}};
      int_acc_reg <= {(AXIS_TDATA_WIDTH+1){1'b0}};
      int_tready_reg <= 1'b0;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_data_reg <= int_data_next;
      int_acc_reg <= int_acc_next;
      int_tready_reg <= int_tready_next;
    end
  end

  always @*
  begin
    int_cntr_next = int_cntr_reg;
    int_data_next = int_data_reg;
    int_acc_next = int_acc_reg;
    int_tready_next = int_tready_reg;

    if(int_cntr_reg < cfg_data)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end
    else
    begin
      int_cntr_next = {(CNTR_WIDTH){1'b0}};
      int_tready_next = 1'b1;
    end

    if(int_tready_reg)
    begin
      int_tready_next = 1'b0;
    end

    if(int_tready_reg & s_axis_tvalid)
    begin
      int_data_next = {~s_axis_tdata[AXIS_TDATA_WIDTH-1], s_axis_tdata[AXIS_TDATA_WIDTH-2:0]};
      int_acc_next = int_acc_reg[AXIS_TDATA_WIDTH-1:0] + int_data_reg;
    end
  end

  assign s_axis_tready = int_tready_reg;

  assign dout = int_acc_reg[AXIS_TDATA_WIDTH];

endmodule
