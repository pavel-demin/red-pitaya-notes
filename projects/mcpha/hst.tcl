# hst_0/aresetn

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 16384
  WRITE_WIDTH_B 32
  ENABLE_A Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_histogram
cell pavel-demin:user:axis_histogram hst_0 {
  BRAM_ADDR_WIDTH 14
  BRAM_DATA_WIDTH 32
  AXIS_TDATA_WIDTH 16
} {
  BRAM_PORTA bram_0/BRAM_PORTA
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create axi_bram_reader
cell pavel-demin:user:axi_bram_reader reader_0 {
  AXI_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
} {
  BRAM_PORTA bram_0/BRAM_PORTB
}
