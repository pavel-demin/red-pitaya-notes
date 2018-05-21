source projects/cfg_test/block_design.tcl

# Create port_slicer
cell pavel-demin:user:port_slicer:1.0 slice_2 {
  DIN_WIDTH 1024 DIN_FROM 0 DIN_TO 0
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer:1.0 slice_3 {
  DIN_WIDTH 1024 DIN_FROM 1 DIN_TO 1
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer:1.0 slice_4 {
  DIN_WIDTH 1024 DIN_FROM 2 DIN_TO 2
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer:1.0 slice_5 {
  DIN_WIDTH 1024 DIN_FROM 3 DIN_TO 3
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer:1.0 slice_6 {
  DIN_WIDTH 1024 DIN_FROM 63 DIN_TO 32
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer:1.0 slice_7 {
  DIN_WIDTH 1024 DIN_FROM 95 DIN_TO 64
} {
  din cfg_0/cfg_data
}

# Create axis_counter
cell pavel-demin:user:axis_counter:1.0 cntr_1 {} {
  cfg_data slice_6/dout
  aclk pll_0/clk_out1
  aresetn slice_2/dout
}

# Create blk_mem_gen
cell xilinx.com:ip:blk_mem_gen:8.4 bram_0 {
  MEMORY_TYPE True_Dual_Port_RAM
  USE_BRAM_BLOCK Stand_Alone
  WRITE_WIDTH_A 32
  WRITE_DEPTH_A 16384
  ENABLE_A Always_Enabled
  ENABLE_B Always_Enabled
  REGISTER_PORTB_OUTPUT_OF_MEMORY_PRIMITIVES false
}

# Create axis_histogram
cell pavel-demin:user:axis_histogram:1.0 hist_0 {
  BRAM_ADDR_WIDTH 14
  BRAM_DATA_WIDTH 32
  AXIS_TDATA_WIDTH 16
} {
  S_AXIS cntr_1/M_AXIS
  BRAM_PORTA bram_0/BRAM_PORTA
  aclk pll_0/clk_out1
  aresetn slice_3/dout
}

# Create axis_bram_reader
cell pavel-demin:user:axis_bram_reader:1.0 reader_0 {
  AXIS_TDATA_WIDTH 32
  BRAM_DATA_WIDTH 32
  BRAM_ADDR_WIDTH 14
  CONTINUOUS FALSE
} {
  BRAM_PORTA bram_0/BRAM_PORTB
  cfg_data slice_7/dout
  aclk pll_0/clk_out1
  aresetn slice_4/dout
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter:1.1 conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 8
} {
  S_AXIS reader_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn slice_5/dout
}

# Create xlconstant
cell xilinx.com:ip:xlconstant:1.1 const_0 {
  CONST_WIDTH 32
  CONST_VAL 503316480
}

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer:1.0 writer_0 {} {
  S_AXIS conv_0/M_AXIS
  M_AXI ps_0/S_AXI_HP0
  cfg_data const_0/dout
  aclk pll_0/clk_out1
  aresetn slice_5/dout
}

assign_bd_address [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM]
