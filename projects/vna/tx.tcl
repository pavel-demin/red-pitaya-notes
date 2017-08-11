# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 96 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 96 DIN_FROM 31 DIN_TO 16 DOUT_WIDTH 16
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler:6.0 dds_0 {
  DDS_CLOCK_RATE 125
  SPURIOUS_FREE_DYNAMIC_RANGE 96
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  HAS_TREADY true
  HAS_ARESETN true
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 16
  DSP48_USE Minimal
  OUTPUT_SELECTION Sine
} {
  aclk /pll_0/clk_out1
  aresetn slice_0/Dout
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_zeroer
cell pavel-demin:user:axis_zeroer:1.0 zeroer_0 {
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS dds_0/M_AXIS_DATA
  m_axis_tready const_0/dout
  aclk /pll_0/clk_out1
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_0 {} {
  aclk /pll_0/clk_out1
  aresetn slice_0/Dout
}

cell xilinx.com:ip:xbip_dsp48_macro:3.0 mult_0 {
  INSTRUCTION1 RNDSIMPLE(A*B+CARRYIN)
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 16
  B_WIDTH 16
  P_WIDTH 15
} {
  A zeroer_0/m_axis_tdata
  B slice_1/Dout
  CARRYIN lfsr_0/m_axis_tdata
  CLK /pll_0/clk_out1
}
