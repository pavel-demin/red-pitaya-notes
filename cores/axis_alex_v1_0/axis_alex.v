
`timescale 1 ns / 1 ps

module axis_alex
(
  // System signals
  input  wire        aclk,
  input  wire        aresetn,

  output wire [3:0]  alex_data,

  // Slave side
  output wire        s_axis_tready,
  input  wire [31:0] s_axis_tdata,
  input  wire        s_axis_tvalid
);

  reg [15:0] int_data_reg, int_data_next;
  reg [11:0] int_cntr_reg, int_cntr_next;
  reg [1:0] int_load_reg, int_load_next;
  reg int_enbl_reg, int_enbl_next;
  reg int_tready_reg, int_tready_next;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_data_reg <= 16'd0;
      int_cntr_reg <= 12'd0;
      int_load_reg <= 2'd0;
      int_enbl_reg <= 1'b0;
      int_tready_reg <= 1'b0;
    end
    else
    begin
      int_data_reg <= int_data_next;
      int_cntr_reg <= int_cntr_next;
      int_load_reg <= int_load_next;
      int_enbl_reg <= int_enbl_next;
      int_tready_reg <= int_tready_next;
    end
  end

  always @*
  begin
    int_data_next = int_data_reg;
    int_cntr_next = int_cntr_reg;
    int_load_next = int_load_reg;
    int_enbl_next = int_enbl_reg;
    int_tready_next = int_tready_reg;

    if(s_axis_tvalid & ~int_enbl_reg)
    begin
      int_data_next = s_axis_tdata[15:0];
      int_load_next = s_axis_tdata[17:16];
      int_enbl_next = 1'b1;
      int_tready_next = 1'b1;
    end

    if(int_tready_reg)
    begin
      int_tready_next = 1'b0;
    end

    if(int_enbl_reg)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;
    end

    if(&int_cntr_reg[6:0])
    begin
      int_data_next = {int_data_reg[14:0], 1'b0};
    end

    if(int_cntr_reg[7] & int_cntr_reg[11])
    begin
      int_cntr_next = 12'd0;
      int_load_next = 2'd0;
      int_enbl_next = 1'b0;
    end

  end

  assign s_axis_tready = int_tready_reg;

  assign alex_data[0] = int_data_reg[15];
  assign alex_data[1] = int_cntr_reg[6] & ~int_cntr_reg[11];
  assign alex_data[2] = int_load_reg[0] & int_cntr_reg[6] & int_cntr_reg[11];
  assign alex_data[3] = int_load_reg[1] & int_cntr_reg[6] & int_cntr_reg[11];

endmodule
