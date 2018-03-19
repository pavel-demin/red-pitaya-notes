
`timescale 1 ns / 1 ps

module axis_iir_filter
(
  // System signals
  input  wire        aclk,
  input  wire        aresetn,

  input  wire [31:0] cfg_data,

  // DSP signals
  output wire [15:0] dsp_a_a,
  input  wire [32:0] dsp_a_p,

  output wire [24:0] dsp_b_a,
  output wire [41:0] dsp_b_c,
  input  wire [41:0] dsp_b_p,

  output wire [24:0] dsp_c_a,
  output wire [41:0] dsp_c_c,
  input  wire [41:0] dsp_c_p,

  // Slave side
  output wire        s_axis_tready,
  input  wire [15:0] s_axis_tdata,
  input  wire        s_axis_tvalid,

  // Master side
  input  wire        m_axis_tready,
  output wire [15:0] m_axis_tdata,
  output wire        m_axis_tvalid
);

  reg [41:0] int_data_reg[1:0];

  always @(posedge aclk)
  begin
    if(~aresetn)
    begin
      int_data_reg[0] <= 42'd0;
      int_data_reg[1] <= 42'd0;
    end
    else
    begin
      int_data_reg[0] <= dsp_b_p;
      int_data_reg[1] <= dsp_c_p;
    end
  end

  assign dsp_a_a = s_axis_tdata;
  assign dsp_b_a = int_data_reg[0][39:15];
  assign dsp_b_c = {dsp_a_p, 9'd0};
  assign dsp_c_a = int_data_reg[1][39:15];
  assign dsp_c_c = int_data_reg[0];
  assign s_axis_tready = m_axis_tready;
  assign m_axis_tdata = $signed(int_data_reg[1][41:23]) < $signed(cfg_data[15:0]) ? cfg_data[15:0] : $signed(int_data_reg[1][41:23]) > $signed(cfg_data[31:16]) ? cfg_data[31:16] : int_data_reg[1][38:23];
  assign m_axis_tvalid = s_axis_tvalid;

endmodule
