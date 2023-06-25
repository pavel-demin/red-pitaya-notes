# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 32 DIN_FROM 31 DIN_TO 0
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 128
  WRITE_DEPTH 16384
} {
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_gate_controller
cell pavel-demin:user:axis_gate_controller gate_0 {} {
  S_AXIS fifo_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_1/dout
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 32
  IN1_WIDTH 32
} {
  In0 slice_2/dout
  In1 gate_0/poff
}

# Create axis_constant
cell pavel-demin:user:axis_constant phase_0 {
  AXIS_TDATA_WIDTH 64
} {
  cfg_data concat_0/dout
  aclk /pll_0/clk_out1
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler dds_0 {
  DDS_CLOCK_RATE 122.88
  SPURIOUS_FREE_DYNAMIC_RANGE 138
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  PHASE_OFFSET Streaming
  HAS_ARESETN true
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 24
  DSP48_USE Minimal
  OUTPUT_SELECTION Sine
} {
  S_AXIS_PHASE phase_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_1/dout
}

# Create c_shift_ram
cell xilinx.com:ip:c_shift_ram delay_0 {
  WIDTH.VALUE_SRC USER
  WIDTH 16
  DEPTH 11
} {
  D gate_0/level
  CLK /pll_0/clk_out1
}

# Create dsp48
cell pavel-demin:user:dsp48 mult_0 {
  A_WIDTH 24
  B_WIDTH 16
  P_WIDTH 14
} {
  A dds_0/m_axis_data_tdata
  B delay_0/Q
  CLK /pll_0/clk_out1
}

# Create c_shift_ram
cell xilinx.com:ip:c_shift_ram delay_1 {
  WIDTH.VALUE_SRC USER
  WIDTH 1
  DEPTH 14
} {
  D gate_0/dout
  CLK /pll_0/clk_out1
}

# Create axis_zeroer
cell pavel-demin:user:axis_zeroer zeroer_0 {
  AXIS_TDATA_WIDTH 16
} {
  s_axis_tdata mult_0/P
  s_axis_tvalid delay_1/Q
  aclk /pll_0/clk_out1
}
