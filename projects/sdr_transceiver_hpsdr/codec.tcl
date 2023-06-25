# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 64 DIN_FROM 31 DIN_TO 0
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 64 DIN_FROM 47 DIN_TO 32
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 64 DIN_FROM 63 DIN_TO 48
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_0 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 2048
  ALWAYS_VALID TRUE
} {
  aclk /pll_0/clk_out1
}

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  M00_TDATA_REMAP {tdata[23:16],tdata[31:24]}
  M01_TDATA_REMAP {tdata[7:0],tdata[15:8]}
} {
  S_AXIS fifo_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 512
  WRITE_WIDTH_B 16
  REGISTER_PORTA_OUTPUT_OF_MEMORY_PRIMITIVES false
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_keyer
cell pavel-demin:user:axis_keyer keyer_0 {
  AXIS_TDATA_WIDTH 16
  BRAM_DATA_WIDTH 16
  BRAM_ADDR_WIDTH 10
} {
  B_BRAM bram_0/BRAM_PORTB
  cfg_data slice_3/dout
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_constant
cell pavel-demin:user:axis_constant phase_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_2/dout
  aclk /pll_0/clk_out1
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler dds_0 {
  DDS_CLOCK_RATE 125
  SPURIOUS_FREE_DYNAMIC_RANGE 96
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  HAS_TREADY true
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 16
  DSP48_USE Minimal
  OUTPUT_SELECTION Sine
} {
  S_AXIS_PHASE phase_0/M_AXIS
  aclk /pll_0/clk_out1
}

# Create dsp48
cell pavel-demin:user:dsp48 mult_0 {
  A_WIDTH 16
  B_WIDTH 16
  P_WIDTH 24
} {
  A dds_0/m_axis_data_tdata
  B keyer_0/m_axis_tdata
  CLK /pll_0/clk_out1
}

# Create dsp48
cell pavel-demin:user:dsp48 mult_1 {
  A_WIDTH 24
  B_WIDTH 16
  P_WIDTH 16
} {
  A mult_0/P
  B slice_4/dout
  CLK /pll_0/clk_out1
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_1 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 2
  M_TDATA_NUM_BYTES 2
} {
  s_axis_tready keyer_0/m_axis_tready
  s_axis_tready dds_0/m_axis_data_tready
  s_axis_tdata mult_1/P
  s_axis_tvalid const_0/dout
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_adder
cell pavel-demin:user:axis_adder adder_0 {
  AXIS_TDATA_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
} {
  S_AXIS_A bcast_0/M00_AXIS
  S_AXIS_B bcast_1/M00_AXIS
  aclk /pll_0/clk_out1
}

# Create axis_adder
cell pavel-demin:user:axis_adder adder_1 {
  AXIS_TDATA_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
} {
  S_AXIS_A bcast_0/M01_AXIS
  S_AXIS_B bcast_1/M01_AXIS
  aclk /pll_0/clk_out1
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
  NUM_SI 2
} {
  S00_AXIS adder_0/M_AXIS
  S01_AXIS adder_1/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_i2s
cell pavel-demin:user:axis_i2s i2s_0 {
  AXIS_TDATA_WIDTH 32
} {
  alex_flag slice_1/dout
  S_AXIS comb_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_subset_converter
cell xilinx.com:ip:axis_subset_converter subset_2 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 4
  TDATA_REMAP {tdata[7:0],tdata[15:8],tdata[23:16],tdata[31:24]}
} {
  S_AXIS i2s_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_fifo
cell pavel-demin:user:axis_fifo fifo_1 {
  S_AXIS_TDATA_WIDTH 32
  M_AXIS_TDATA_WIDTH 32
  WRITE_DEPTH 1024
  ALWAYS_READY TRUE
} {
  S_AXIS subset_2/M_AXIS
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}
