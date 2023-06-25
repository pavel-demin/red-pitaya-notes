# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 32 DIN_FROM 15 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 32 DIN_FROM 31 DIN_TO 16
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 256
} {
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0 {
  CONST_WIDTH 32
  CONST_VAL 85333332
}

# Create axis_interpolator
cell pavel-demin:user:axis_interpolator inter_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 32
} {
  S_AXIS fifo_0/M_AXIS
  cfg_data const_0/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_1

# Create dds_compiler
cell xilinx.com:ip:dds_compiler dds_0 {
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
  S_AXIS_PHASE inter_0/M_AXIS
  m_axis_data_tready const_1/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create dsp48
cell pavel-demin:user:dsp48 mult_0 {
  A_WIDTH 16
  B_WIDTH 16
  P_WIDTH 14
} {
  A dds_0/m_axis_data_tdata
  B slice_1/dout
  CLK /pll_0/clk_out1
}

# Create dsp48
cell pavel-demin:user:dsp48 mult_1 {
  A_WIDTH 16
  B_WIDTH 16
  P_WIDTH 14
} {
  A dds_0/m_axis_data_tdata
  B slice_2/dout
  CLK /pll_0/clk_out1
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 mult_0/P
  In1 mult_1/P
}

# Create c_shift_ram
cell xilinx.com:ip:c_shift_ram delay_0 {
  WIDTH.VALUE_SRC USER
  WIDTH 1
  DEPTH 3
} {
  D dds_0/m_axis_data_tvalid
  CLK /pll_0/clk_out1
}

# Create axis_zeroer
cell pavel-demin:user:axis_zeroer zeroer_0 {
  AXIS_TDATA_WIDTH 32
} {
  s_axis_tdata concat_0/dout
  s_axis_tvalid delay_0/Q
  aclk /pll_0/clk_out1
}
