
`timescale 1 ns / 1 ps

module axis_averager #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 16,
  parameter AXIS_TDATA_SIGNED = "FALSE"
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  input  wire [CNTR_WIDTH-1:0]       pre_data,
  input  wire [CNTR_WIDTH-1:0]       tot_data,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,

  // FIFO programmable flag
  input  wire                        fifo_prog_full,

  // FIFO_WRITE port
  input  wire                        fifo_write_full,
  output wire [AXIS_TDATA_WIDTH-1:0] fifo_write_data,
  output wire                        fifo_write_wren,

  // FIFO_READ port
  input  wire                        fifo_read_empty,
  input  wire [AXIS_TDATA_WIDTH-1:0] fifo_read_data,
  output wire                        fifo_read_rden
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg, int_cntr_next;
  reg int_rden_reg, int_rden_next;
  reg int_tvalid_reg, int_tvalid_next;

  wire [AXIS_TDATA_WIDTH-1:0] int_data_wire, sum_data_wire;

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      int_rden_reg <= 1'b0;
      int_tvalid_reg <= 1'b1;
    end
    else
    begin
      int_cntr_reg <= int_cntr_next;
      int_rden_reg <= int_rden_next;
      int_tvalid_reg <= int_tvalid_next;
    end
  end

  always @*
  begin
    int_cntr_next = int_cntr_reg;
    int_rden_next = int_rden_reg;
    int_tvalid_next = int_tvalid_reg;

    if(s_axis_tvalid)
    begin
      int_cntr_next = int_cntr_reg + 1'b1;

      if(int_cntr_reg == pre_data)
      begin
        int_rden_next = 1'b1;
        int_tvalid_next = 1'b0;
      end

      if(int_cntr_reg == tot_data)
      begin
        int_cntr_next = {(CNTR_WIDTH){1'b0}};
        int_tvalid_next = 1'b1;
      end
    end
  end

  assign int_data_wire = int_tvalid_reg ? {(AXIS_TDATA_WIDTH){1'b0}} : fifo_read_data;

  generate
    if(AXIS_TDATA_SIGNED == "TRUE")
    begin : SIGNED
      assign sum_data_wire = $signed(int_data_wire) + $signed(s_axis_tdata);
    end
    else
    begin : UNSIGNED
      assign sum_data_wire = int_data_wire + s_axis_tdata;
    end
  endgenerate

  assign s_axis_tready = 1'b1;

  assign m_axis_tdata = fifo_read_data;
  assign m_axis_tvalid = fifo_prog_full & int_tvalid_reg & s_axis_tvalid;

  assign fifo_read_rden = int_rden_reg & s_axis_tvalid;

  assign fifo_write_data = sum_data_wire;
  assign fifo_write_wren = s_axis_tvalid;

endmodule
