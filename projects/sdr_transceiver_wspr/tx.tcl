# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
}

# Create axi_axis_writer
cell pavel-demin:user:axi_axis_writer:1.0 writer_0 {
  AXI_DATA_WIDTH 32
} {
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_data_fifo
cell xilinx.com:ip:axis_data_fifo:1.1 fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  FIFO_DEPTH 256
} {
  S_AXIS writer_0/M_AXIS
  s_axis_aclk /ps_0/FCLK_CLK0
  s_axis_aresetn /rst_0/peripheral_aresetn
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0 {
  CONST_WIDTH 32
  CONST_VAL 85333332
}

# Create axis_interpolator
cell pavel-demin:user:axis_interpolator:1.0 inter_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 32
} {
  S_AXIS fifo_0/M_AXIS
  cfg_data const_0/dout
  aclk /ps_0/FCLK_CLK0
  aresetn /rst_0/peripheral_aresetn
}

# Create dds_compiler
cell xilinx.com:ip:dds_compiler:6.0 dds_0 {
  DDS_CLOCK_RATE 125
  SPURIOUS_FREE_DYNAMIC_RANGE 84
  FREQUENCY_RESOLUTION 0.2
  PHASE_INCREMENT Streaming
  HAS_TREADY true
  HAS_ARESETN true
  HAS_PHASE_OUT false
  PHASE_WIDTH 30
  OUTPUT_WIDTH 14
  DSP48_USE Minimal
} {
  S_AXIS_PHASE inter_0/M_AXIS
  aclk /ps_0/FCLK_CLK0
  aresetn slice_0/Dout
}

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_1 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
} {
  S_AXIS dds_0/M_AXIS_DATA
  s_axis_aclk /ps_0/FCLK_CLK0
  s_axis_aresetn /rst_0/peripheral_aresetn
}
