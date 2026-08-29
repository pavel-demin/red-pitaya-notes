
`timescale 1 ns / 1 ps

module axis_packetizer #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter integer CNTR_WIDTH = 32,
  parameter         CONTINUOUS = "FALSE",
  parameter         ALWAYS_READY = "FALSE"
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

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid,
  output wire                        m_axis_tlast
);

  reg [CNTR_WIDTH-1:0] int_cntr_reg;

  wire int_gate_wire, int_last_wire, int_valid_wire, int_ready_wire;

  assign int_valid_wire = s_axis_tvalid & int_gate_wire;
  assign int_last_wire = int_cntr_reg >= cfg_data;

  generate
    if(CONTINUOUS == "TRUE")
    begin : CONTINUE
      assign int_gate_wire = 1'b1;
    end
    else
    begin : STOP
      reg int_gate_reg;

      always @(posedge aclk)
      begin
        if(~aresetn)
        begin
          int_gate_reg <= 1'b1;
        end
        else if(int_valid_wire & int_ready_wire & int_last_wire)
        begin
          int_gate_reg <= 1'b0;
        end
      end

      assign int_gate_wire = int_gate_reg;
    end
  endgenerate

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
    end
    else if(int_valid_wire & int_ready_wire)
    begin
      if(int_last_wire)
      begin
        int_cntr_reg <= {(CNTR_WIDTH){1'b0}};
      end
      else
      begin
        int_cntr_reg <= int_cntr_reg + 1'b1;
      end
    end
  end

  generate
    if(ALWAYS_READY == "TRUE")
    begin : READY
      assign s_axis_tready = 1'b1;
    end
    else
    begin : BLOCKING
      assign s_axis_tready = int_ready_wire & int_gate_wire;
    end
  endgenerate

  output_buffer #(
    .DATA_WIDTH(AXIS_TDATA_WIDTH + 1)
  ) buf_0 (
    .aclk(aclk), .aresetn(aresetn),
    .in_data({int_last_wire, s_axis_tdata}), .in_valid(int_valid_wire), .in_ready(int_ready_wire),
    .out_data({m_axis_tlast, m_axis_tdata}), .out_valid(m_axis_tvalid), .out_ready(m_axis_tready)
  );

endmodule
