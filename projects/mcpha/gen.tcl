# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 96 DIN_FROM 15 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 96 DIN_FROM 31 DIN_TO 16
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 96 DIN_FROM 47 DIN_TO 32
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 96 DIN_FROM 95 DIN_TO 64
}

# Create axi_axis_writer
cell pavel-demin:user:axi_axis_writer writer_0 {
  AXI_DATA_WIDTH 32
} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic not_0 {
  C_SIZE 1
  C_OPERATION not
} {
  Op1 slice_0/dout
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator fifo_generator_0 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 32
  INPUT_DEPTH 16384
  OUTPUT_DATA_WIDTH 64
  OUTPUT_DEPTH 8192
  WRITE_DATA_COUNT true
  WRITE_DATA_COUNT_WIDTH 15
} {
  clk /pll_0/clk_out1
  srst not_0/Res
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 64
} {
  S_AXIS writer_0/M_AXIS
  FIFO_READ fifo_generator_0/FIFO_READ
  FIFO_WRITE fifo_generator_0/FIFO_WRITE
  aclk /pll_0/clk_out1
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 8
  M_TDATA_NUM_BYTES 8
  TDATA_REMAP {tdata[31:0],tdata[63:32]}
} {
  S_AXIS fifo_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_pulse_generator
cell pavel-demin:user:axis_pulse_generator gen_0 {} {
  S_AXIS subset_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axis_zeroer
cell pavel-demin:user:axis_zeroer zeroer_0 {
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS gen_0/M_AXIS
  aclk /pll_0/clk_out1
}

# Create dsp_macro
cell xilinx.com:ip:dsp_macro dsp_0 {
  INSTRUCTION1 A*B
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 16
  B_WIDTH 17
  P_WIDTH 33
} {
  B slice_1/dout
  CLK /pll_0/clk_out1
}

# Create dsp_macro
cell xilinx.com:ip:dsp_macro dsp_1 {
  INSTRUCTION1 A*B+C
  PIPELINE_OPTIONS Expert
  AREG_3 false
  AREG_4 false
  CREG_3 false
  CREG_4 false
  MREG_5 false
  PREG_6 false
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  C_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 25
  B_WIDTH 17
  C_WIDTH 42
  P_WIDTH 42
} {
  B slice_2/dout
  CLK /pll_0/clk_out1
}

cell xilinx.com:ip:dsp_macro dsp_2 {
  INSTRUCTION1 A*B+C
  PIPELINE_OPTIONS Expert
  AREG_3 false
  AREG_4 false
  CREG_3 false
  CREG_4 false
  MREG_5 false
  PREG_6 false
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  C_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 25
  B_WIDTH 17
  C_WIDTH 42
  P_WIDTH 42
} {
  B slice_3/dout
  CLK /pll_0/clk_out1
}

# Create axis_iir_filter
cell pavel-demin:user:axis_iir_filter iir_0 {
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS zeroer_0/M_AXIS
  cfg_data slice_4/dout
  dsp_a_a dsp_0/A
  dsp_a_p dsp_0/P
  dsp_b_a dsp_1/A
  dsp_b_c dsp_1/C
  dsp_b_p dsp_1/P
  dsp_c_a dsp_2/A
  dsp_c_c dsp_2/C
  dsp_c_p dsp_2/P
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}
