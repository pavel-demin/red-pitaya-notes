# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register:1.0 cfg_0 {
  CFG_DATA_WIDTH 160
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_0 {
  DIN_WIDTH 160 DIN_FROM 7 DIN_TO 0 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 rst_slice_1 {
  DIN_WIDTH 160 DIN_FROM 15 DIN_TO 8 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 out_slice_0 {
  DIN_WIDTH 160 DIN_FROM 23 DIN_TO 16 DOUT_WIDTH 8
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_0 {
  DIN_WIDTH 160 DIN_FROM 95 DIN_TO 32 DOUT_WIDTH 64
} {
  Din cfg_0/cfg_data
}

# Create xlslice
cell xilinx.com:ip:xlslice:1.0 cfg_slice_1 {
  DIN_WIDTH 160 DIN_FROM 159 DIN_TO 96 DOUT_WIDTH 64
} {
  Din cfg_0/cfg_data
}

module rx_0 {
  source projects/sdr_transceiver_emb/rx.tcl
} {
  slice_0/Din rst_slice_0/Dout
  slice_1/Din cfg_slice_0/Dout
  slice_2/Din cfg_slice_0/Dout
}

module tx_0 {
  source projects/sdr_transceiver_emb/tx.tcl
} {
  slice_0/Din rst_slice_1/Dout
  slice_1/Din cfg_slice_1/Dout
  slice_2/Din cfg_slice_1/Dout
}

# Create xlconcat
cell xilinx.com:ip:xlconcat:2.1 concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 16
  IN1_WIDTH 16
} {
  In0 rx_0/fifo_generator_0/rd_data_count
  In1 tx_0/fifo_generator_0/wr_data_count
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register:1.0 sts_0 {
  STS_DATA_WIDTH 32
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data concat_0/dout
}
