# CFG

# Create axi_cfg_register
cell pavel-demin:user:axi_cfg_register cfg_0 {
  CFG_DATA_WIDTH 64
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
}

cell pavel-demin:user:port_slicer rst_slice_0 {
  DIN_WIDTH 64 DIN_FROM 0 DIN_TO 0
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer rst_slice_1 {
  DIN_WIDTH 64 DIN_FROM 8 DIN_TO 8
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer cfg_slice_0 {
  DIN_WIDTH 64 DIN_FROM 63 DIN_TO 32
} {
  din cfg_0/cfg_data
}

# LED

# Create port_slicer
cell pavel-demin:user:port_slicer led_slice_0 {
  DIN_WIDTH 64 DIN_FROM 23 DIN_TO 16
} {
  din cfg_0/cfg_data
  dout /led_o
}

# GPIO

# Delete input/output port
delete_bd_objs [get_bd_ports /exp_p_tri_io]

# Create output port
create_bd_port -dir O -from 7 -to 0 exp_p_tri_io

# Create port_slicer
cell pavel-demin:user:port_slicer out_slice_0 {
  DIN_WIDTH 64 DIN_FROM 31 DIN_TO 24
} {
  din cfg_0/cfg_data
  dout /exp_p_tri_io
}

# Delete input/output port
delete_bd_objs [get_bd_ports /exp_n_tri_io]

# Create input port
create_bd_port -dir I -from 3 -to 0 exp_n_tri_io

# Create port_slicer
cell pavel-demin:user:port_slicer pps_slice_0 {
  DIN_WIDTH 4 DIN_FROM 3 DIN_TO 3
} {
  din /exp_n_tri_io
  dout /ps_0/GPIO_I
}

# PPS

# Create axis_pps_counter
cell pavel-demin:user:axis_pps_counter cntr_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 32
} {
  pps_data pps_slice_0/dout
  aclk /pll_0/clk_out1
  aresetn rst_slice_0/dout
}

# Create axis_data_fifo
cell xilinx.com:ip:axis_data_fifo fifo_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  FIFO_DEPTH 1024
  HAS_RD_DATA_COUNT true
} {
  S_AXIS cntr_0/M_AXIS
  s_axis_aclk /pll_0/clk_out1
  s_axis_aresetn rst_slice_0/dout
}

# Create axi_axis_reader
cell pavel-demin:user:axi_axis_reader reader_0 {
  AXI_DATA_WIDTH 32
} {
  S_AXIS fifo_0/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Level measurement

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0

# Create axis_broadcaster
cell xilinx.com:ip:axis_broadcaster bcast_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  M_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 2
  NUM_MI 2
  M00_TDATA_REMAP {tdata[15:0]}
  M01_TDATA_REMAP {tdata[31:16]}
} {
  s_axis_tdata /adc_0/m_axis_tdata
  s_axis_tvalid const_0/dout
  aclk /pll_0/clk_out1
  aresetn rst_slice_1/dout
}

# Create axis_decimator
cell pavel-demin:user:axis_maxabs_finder maxabs_0 {
  AXIS_TDATA_WIDTH 16
  CNTR_WIDTH 32
} {
  S_AXIS bcast_0/M00_AXIS
  cfg_data cfg_slice_0/dout
  aclk /pll_0/clk_out1
  aresetn rst_slice_1/dout
}

# Create axis_decimator
cell pavel-demin:user:axis_maxabs_finder maxabs_1 {
  AXIS_TDATA_WIDTH 16
  CNTR_WIDTH 32
} {
  S_AXIS bcast_0/M01_AXIS
  cfg_data cfg_slice_0/dout
  aclk /pll_0/clk_out1
  aresetn rst_slice_1/dout
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
} {
  S00_AXIS maxabs_0/M_AXIS
  S01_AXIS maxabs_1/M_AXIS
  aclk /pll_0/clk_out1
  aresetn rst_slice_1/dout
}

# Create axis_data_fifo
cell xilinx.com:ip:axis_data_fifo fifo_1 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 4
  FIFO_DEPTH 1024
  HAS_RD_DATA_COUNT true
} {
  S_AXIS comb_0/M_AXIS
  s_axis_aclk /pll_0/clk_out1
  s_axis_aresetn rst_slice_1/dout
}

# Create axi_axis_reader
cell pavel-demin:user:axi_axis_reader reader_1 {
  AXI_DATA_WIDTH 32
} {
  S_AXIS fifo_1/M_AXIS
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create xlconcat
cell xilinx.com:ip:xlconcat concat_0 {
  NUM_PORTS 2
  IN0_WIDTH 32
  IN1_WIDTH 32
} {
  In0 fifo_0/axis_rd_data_count
  In1 fifo_1/axis_rd_data_count
}

# Create axi_sts_register
cell pavel-demin:user:axi_sts_register sts_0 {
  STS_DATA_WIDTH 64
  AXI_ADDR_WIDTH 32
  AXI_DATA_WIDTH 32
} {
  sts_data concat_0/dout
}

addr 0x40040000 4K sts_0/S_AXI /ps_0/M_AXI_GP0

addr 0x40041000 4K cfg_0/S_AXI /ps_0/M_AXI_GP0

addr 0x40042000 4K reader_0/S_AXI /ps_0/M_AXI_GP0

addr 0x40043000 4K reader_1/S_AXI /ps_0/M_AXI_GP0
