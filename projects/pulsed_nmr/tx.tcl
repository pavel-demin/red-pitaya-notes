# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_1 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1
}

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_2 {
  DIN_WIDTH 64 DIN_FROM 31 DIN_TO 0
}

# Create xlslice
cell pavel-demin:user:port_slicer:1.0 slice_3 {
  DIN_WIDTH 64 DIN_FROM 47 DIN_TO 32
}

# Create axi_axis_writer
cell pavel-demin:user:axi_axis_writer:1.0 writer_0 {
  AXI_DATA_WIDTH 32
} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create fifo_generator
cell xilinx.com:ip:fifo_generator:13.2 fifo_generator_0 {
  PERFORMANCE_OPTIONS First_Word_Fall_Through
  INPUT_DATA_WIDTH 32
  INPUT_DEPTH 2048
  OUTPUT_DATA_WIDTH 128
  OUTPUT_DEPTH 512
  WRITE_DATA_COUNT true
  WRITE_DATA_COUNT_WIDTH 12
} {
  clk /pll_0/clk_out1
  srst slice_0/dout
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo:1.0 fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 128
} {
  S_AXIS writer_0/M_AXIS
  FIFO_READ fifo_generator_0/FIFO_READ
  FIFO_WRITE fifo_generator_0/FIFO_WRITE
  aclk /pll_0/clk_out1
}

# Create axis_gate_controller
cell pavel-demin:user:axis_gate_controller:1.0 gate_0 {} {
  S_AXIS fifo_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_1/dout
}

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 3
  IN0_WIDTH 32
  IN1_WIDTH 32
  IN2_WIDTH 1
} {
  In0 slice_2/dout
  In1 gate_0/poff
  In2 gate_0/sync
}

cell xilinx.com:ip:c_shift_ram:12.0 delay_0 {
  WIDTH.VALUE_SRC USER
  WIDTH 1
  DEPTH 11
} {
  D gate_0/dout
  CLK /pll_0/clk_out1
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic:2.0 or_0 {
  C_SIZE 1
  C_OPERATION or
} {
  Op1 gate_0/dout
  Op2 delay_0/Q
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler:6.0 dds_0 {
  DDS_CLOCK_RATE 125
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
  RESYNC true
} {
  s_axis_phase_tdata concat_0/dout
  s_axis_phase_tvalid or_0/Res
  aclk /pll_0/clk_out1
  aresetn slice_1/dout
}

# Create axis_lfsr
cell pavel-demin:user:axis_lfsr:1.0 lfsr_0 {} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create xbip_dsp48_macro
cell xilinx.com:ip:xbip_dsp48_macro:3.0 mult_0 {
  INSTRUCTION1 RNDSIMPLE(A*B+CARRYIN)
  A_WIDTH.VALUE_SRC USER
  B_WIDTH.VALUE_SRC USER
  OUTPUT_PROPERTIES User_Defined
  A_WIDTH 24
  B_WIDTH 16
  P_WIDTH 15
} {
  A dds_0/m_axis_data_tdata
  B slice_3/dout
  CARRYIN lfsr_0/m_axis_tdata
  CLK /pll_0/clk_out1
}

# Create c_shift_ram
cell xilinx.com:ip:c_shift_ram:12.0 delay_1 {
  WIDTH.VALUE_SRC USER
  WIDTH 1
  DEPTH 15
} {
  D gate_0/dout
  CLK /pll_0/clk_out1
}

# Create axis_zeroer
cell pavel-demin:user:axis_zeroer:1.0 zeroer_0 {
  AXIS_TDATA_WIDTH 32
} {
  s_axis_tdata mult_0/P
  s_axis_tvalid delay_1/Q
  aclk /pll_0/clk_out1
}
