# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register cfg_0 {
  CFG_DATA_WIDTH 160
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}


# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_0 {
  DIN_WIDTH 160 DIN_FROM 7 DIN_TO 0
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_1 {
  DIN_WIDTH 160 DIN_FROM 15 DIN_TO 8
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer out_slice_0 {
  DIN_WIDTH 160 DIN_FROM 16 DIN_TO 16
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_0 {
  DIN_WIDTH 160 DIN_FROM 95 DIN_TO 32
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_1 {
  DIN_WIDTH 160 DIN_FROM 159 DIN_TO 96
} {
  din cfg_0/cfg_data
}

module rx_0 {
  source projects/sdr_transceiver/rx.tcl
} {
  slice_0/din rst_slice_0/dout
  slice_1/din cfg_slice_0/dout
  slice_2/din cfg_slice_0/dout
}

module tx_0 {
  source projects/sdr_transceiver/tx.tcl
} {
  slice_0/din rst_slice_1/dout
  slice_1/din cfg_slice_1/dout
  slice_2/din cfg_slice_1/dout
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 rx_0/fifo_generator_0/rd_data_count
  In1 tx_0/fifo_generator_0/wr_data_count
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register sts_0 {
  STS_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data concat_0/dout
}
