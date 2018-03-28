
`timescale 1 ns / 1 ps

module axis_adder #
(
  parameter integer AXIS_TDATA_WIDTH = 32,
  parameter         AXIS_TDATA_SIGNED = "FALSE"
)
(
  // System signals
  input  wire                        aclk,

  // Slave side
  output wire                        s_axis_a_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_a_tdata,
  input  wire                        s_axis_a_tvalid,

  output wire                        s_axis_b_tready,
  input  wire [AXIS_TDATA_WIDTH-1:0] s_axis_b_tdata,
  input  wire                        s_axis_b_tvalid,

  // Master side
  input  wire                        m_axis_tready,
  output wire [AXIS_TDATA_WIDTH-1:0] m_axis_tdata,
  output wire                        m_axis_tvalid
);

  wire [AXIS_TDATA_WIDTH-1:0] int_tdata_wire;
  wire int_tready_wire, int_tvalid_wire;

  generate
    if(AXIS_TDATA_SIGNED == "TRUE")
    begin : SIGNED
      assign int_tdata_wire = $signed(s_axis_a_tdata) + $signed(s_axis_b_tdata);
    end
    else
    begin : UNSIGNED
      assign int_tdata_wire = s_axis_a_tdata + s_axis_b_tdata;
    end
  endgenerate

  assign int_tvalid_wire = s_axis_a_tvalid & s_axis_b_tvalid;
  assign int_tready_wire = int_tvalid_wire & m_axis_tready;

  assign s_axis_a_tready = int_tready_wire;
  assign s_axis_b_tready = int_tready_wire;
  assign m_axis_tdata = int_tdata_wire;
  assign m_axis_tvalid = int_tvalid_wire;

endmodule
