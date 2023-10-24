# hst_0/aresetn

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  USE_BYTE_WRITE_ENABLE true
  BYTE_SIZE 8
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 4096
  REGISTER_PORTA_OUTPUT_OF_MEMORY_PRIMITIVES false
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_histogram
cell pavel-demin:user:axis_histogram hst_0 {
  AXIS_TDATA_WIDTH 16
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 12
} {
  B_BRAM bram_0/BRAM_PORTB
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}
