
`timescale 1 ns / 1 ps

module axis_accumulator #
(
  parameter integer S_AXIS_TDATA_WIDTH = 16,
  parameter integer M_AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 16,
  parameter         AXIS_TDATA_SIGNED = "FALSE",
  parameter         CONTINUOUS = "FALSE"
)
(
  // System signals
  input  wire                          aclk,
  input  wire                          aresetn,

  input  wire [CNTR_WIDTH-1:0]         cfg_data,

  // Slave side
  output wire                          s_axis_tready,
  input  wire [S_AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                          s_axis_tvalid,

  // Master side
  input  wire                          m_axis_tready,
  output wire [M_AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                          m_axis_tvalid
);

  reg [M_AXIS_TDATA_WIDTH-1:0] int_accu_reg;
  reg [CNTR_WIDTH-1:0] int_cntr_reg;

  wire [M_AXIS_TDATA_WIDTH-1:0] sum_accu_wire;
  wire int_last_wire, int_ready_wire;

  assign int_last_wire = int_cntr_reg >= cfg_data;

  generate
    if(AXIS_TDATA_SIGNED == "TRUE")
    begin : SIGNED
      assign sum_accu_wire = $signed(int_accu_reg) + $signed(s_axis_tdata);
    end
    else
    begin : UNSIGNED
      assign sum_accu_wire = int_accu_reg + s_axis_tdata;
    end
  endgenerate

  generate
    if(CONTINUOUS == "TRUE")
    begin : CONTINUE
      assign s_axis_tready = int_ready_wire;
    end
    else
    begin : STOP
      reg int_ready_reg;

      always @(posedge aclk)
      begin
        if(~aresetn)
        begin
          int_ready_reg <= 1'b1;
        end
        else if(s_axis_tvalid & s_axis_tready & int_last_wire)
        begin
          int_ready_reg <= 1'b0;
        end
      end

      assign s_axis_tready = int_ready_wire & int_ready_reg;
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_accu_reg <= {(M_AXIS_TDATA_WIDTH){1'b0}};
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
    end
    else if(s_axis_tvalid & s_axis_tready)
    begin
      if(int_last_wire)
      begin
        int_accu_reg <= {(M_AXIS_TDATA_WIDTH){1'b0}};
        int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      end
      else
      begin
        int_accu_reg <= sum_accu_wire;
        int_cntr_reg <= int_cntr_reg + 1'b1;
      end
    end
  end

  output_buffer #(
    .DATA_WIDTH(M_AXIS_TDATA_WIDTH)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data(sum_accu_wire), .in_valid(s_axis_tvalid & int_last_wire), .in_ready(int_ready_wire),
    .out_data(m_axis_tdata), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

endmodule
