# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0 DOUT_WIDTH 1
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_1 {
  DIN_WIDTH 64 DIN_FROM 31 DIN_TO 0 DOUT_WIDTH 32
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 slice_2 {
  DIN_WIDTH 64 DIN_FROM 47 DIN_TO 32 DOUT_WIDTH 16
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0

# Create axis_clock_converter
cell xilinx.com:ip:axis_clock_converter:1.1 fifo_1 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
} {
  S_AXIS mult_0/M_AXIS_DOUT
  s_axis_aclk /ps_0/FCLK_CLK0
  s_axis_aresetn /rst_0/peripheral_aresetn
  m_axis_aclk /pll_0/clk_out1
  m_axis_aresetn const_0/dout
}
