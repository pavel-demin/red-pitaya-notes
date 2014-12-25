
`timescale 1 ns / 1 ps

module axis_packetizer_v1_0 #
(
  parameter integer PACKET_SIZE = 65536,
  parameter integer AXIS_TDATA_WIDTH = 32
)
(
  // System signals
  input  wire                        aclk,
  input  wire                        aresetn,

  // Slave side
  output wire                        s_axis_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,

  // Master side
  output wire                        m_axis_tvalid,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tlast,
  input  wire                        m_axis_tready
);
  localparam integer CNTR_WIDTH = f_clogb2(PACKET_SIZE);

  reg  [CNTR_WIDTH-1:0]   int_cntr_reg, int_cntr_next;
  wire [CNTR_WIDTH-1:0]   int_cntr_wire;
  wire                    int_comp_wire;

  assign int_comp_wire = (int_cntr_next == (PACKET_SIZE - 1));

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

  always @*
  begin
    int_cntr_next = int_cntr_reg;
    if(s_axis_tvalid & m_axis_tready)
    begin
      int_cntr_next = int_comp_wire ? {(CNTR_WIDTH){1'b0}} : int_cntr_reg + 1'b1;
    end
  end

  assign s_axis_tready = m_axis_tready;
  assign m_axis_tvalid = s_axis_tvalid;
  assign m_axis_tdata = s_axis_tdata;

  assign m_axis_tlast = int_comp_wire;

endmodule
