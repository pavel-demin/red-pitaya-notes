source projects/cfg_test/block_design.tcl

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 1024 DIN_FROM 0 DIN_TO 0
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 1024 DIN_FROM 1 DIN_TO 1
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 1024 DIN_FROM 63 DIN_TO 32
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_5 {
  DIN_WIDTH 1024 DIN_FROM 95 DIN_TO 64
} {
  din cfg_0/cfg_data
}

# Create axis_counter
cell pavel-demin:user:axis_counter cntr_1 {} {
  cfg_data slice_5/dout
  aclk pll_0/clk_out1
  aresetn slice_2/dout
}

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer writer_0 {
  ADDR_WIDTH 20
  AXI_ID_WIDTH 3
  AXIS_TDATA_WIDTH 32
  FIFO_WRITE_DEPTH 1024
} {
  S_AXIS cntr_1/M_AXIS
  M_AXI ps_0/S_AXI_ACP
  cfg_data slice_4/dout
  aclk pll_0/clk_out1
  aresetn slice_3/dout
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register sts_0 {
  STS_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data writer_0/sts_data
}

addr 0x40001000 4K sts_0/S_AXI /ps_0/M_AXI_GP0

assign_bd_address [get_bd_addr_segs ps_0/S_AXI_ACP/ACP_DDR_LOWOCM]
