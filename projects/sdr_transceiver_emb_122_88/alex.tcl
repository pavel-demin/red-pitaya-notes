# Create axi_axis_writer
cell pavel-demin:user:axi_axis_writer writer_0 {
  AXI_DATA_WIDTH 32
} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_data_fifo
cell xilinx.com:ip:axis_data_fifo fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  FIFO_DEPTH 1024
} {
  S_AXIS writer_0/M_AXIS
  s_axis_aclk /pll_0/clk_out1
  s_axis_aresetn /rst_0/peripheral_aresetn
}

# Create axis_alex
cell pavel-demin:user:axis_alex alex_0 {} {
  S_AXIS fifo_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}
