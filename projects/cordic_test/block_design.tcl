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
  DIN_WIDTH 1024 DIN_FROM 2 DIN_TO 2
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_5 {
  DIN_WIDTH 1024 DIN_FROM 63 DIN_TO 32
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_6 {
  DIN_WIDTH 1024 DIN_FROM 95 DIN_TO 64
} {
  din cfg_0/cfg_data
}

# Create port_slicer
cell pavel-demin:user:port_slicer slice_7 {
  DIN_WIDTH 1024 DIN_FROM 127 DIN_TO 96
} {
  din cfg_0/cfg_data
}

# Create axis_constant
cell pavel-demin:user:axis_constant const_0 {
  AXIS_TDATA_WIDTH 32
} {
  cfg_data slice_5/dout
  aclk pll_0/clk_out1
}

# Create axis_phase_generator
cell pavel-demin:user:axis_phase_generator phase_0 {
  AXIS_TDATA_WIDTH 32
  PHASE_WIDTH 30
} {
  cfg_data slice_6/dout
  aclk pll_0/clk_out1
  aresetn slice_2/dout
}

# Create cordic
cell xilinx.com:ip:cordic cordic_0 {
  INPUT_WIDTH.VALUE_SRC USER
  PIPELINING_MODE Optimal
  PHASE_FORMAT Scaled_Radians
  INPUT_WIDTH 32
  OUTPUT_WIDTH 15
  ROUND_MODE Round_Pos_Neg_Inf
  COMPENSATION_SCALING Embedded_Multiplier
} {
  S_AXIS_CARTESIAN const_0/M_AXIS
  S_AXIS_PHASE phase_0/M_AXIS
  aclk pll_0/clk_out1
}

# Create axis_packetizer
cell pavel-demin:user:axis_packetizer pktzr_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 32
  CONTINUOUS FALSE
} {
  S_AXIS cordic_0/M_AXIS_DOUT
  cfg_data slice_7/dout
  aclk pll_0/clk_out1
  aresetn slice_3/dout
}

# Create axis_dwidth_converter
cell xilinx.com:ip:axis_dwidth_converter conv_0 {
  S_TDATA_NUM_BYTES.VALUE_SRC USER
  S_TDATA_NUM_BYTES 4
  M_TDATA_NUM_BYTES 8
} {
  S_AXIS pktzr_0/M_AXIS
  aclk pll_0/clk_out1
  aresetn slice_4/dout
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_1 {
  CONST_WIDTH 32
  CONST_VAL 503316480
}

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer writer_0 {} {
  S_AXIS conv_0/M_AXIS
  M_AXI ps_0/S_AXI_HP0
  cfg_data const_1/dout
  aclk pll_0/clk_out1
  aresetn slice_4/dout
}

assign_bd_address [get_bd_addr_segs ps_0/S_AXI_HP0/HP0_DDR_LOWOCM]
