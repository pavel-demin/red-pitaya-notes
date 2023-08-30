# scope_0/aresetn

# Create port_slicer
cell pavel-demin:user:port_slicer slice_0 {
  DIN_WIDTH 8 DIN_FROM 0 DIN_TO 0
}

# writer_0/aresetn

# Create port_slicer
cell pavel-demin:user:port_slicer slice_1 {
  DIN_WIDTH 8 DIN_FROM 1 DIN_TO 1
}

# sel_0/cfg_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_2 {
  DIN_WIDTH 8 DIN_FROM 2 DIN_TO 2
}

# trig_0/pol_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_3 {
  DIN_WIDTH 8 DIN_FROM 3 DIN_TO 3
}

# or_0/Op1

# Create port_slicer
cell pavel-demin:user:port_slicer slice_4 {
  DIN_WIDTH 8 DIN_FROM 4 DIN_TO 4
}

# scope_0/run_flag

# Create port_slicer
cell pavel-demin:user:port_slicer slice_5 {
  DIN_WIDTH 8 DIN_FROM 5 DIN_TO 5
}

# writer_0/cfg_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_6 {
  DIN_WIDTH 128 DIN_FROM 31 DIN_TO 0
}

# scope_0/pre_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_7 {
  DIN_WIDTH 128 DIN_FROM 63 DIN_TO 32
}

# scope_0/tot_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_8 {
  DIN_WIDTH 128 DIN_FROM 95 DIN_TO 64
}

# trig_0/lvl_data

# Create port_slicer
cell pavel-demin:user:port_slicer slice_9 {
  DIN_WIDTH 128 DIN_FROM 111 DIN_TO 96
}

# Create axis_selector
cell pavel-demin:user:axis_selector sel_0 {
  AXIS_TDATA_WIDTH 16
} {
  cfg_data slice_2/dout
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_0 {
  CONST_WIDTH 16
  CONST_VAL 65535
}

# Create axis_trigger
cell pavel-demin:user:axis_trigger trig_0 {
  AXIS_TDATA_WIDTH 16
  AXIS_TDATA_SIGNED TRUE
} {
  S_AXIS sel_0/M_AXIS
  pol_data slice_3/dout
  msk_data const_0/dout
  lvl_data slice_9/dout
  aclk /pll_0/clk_out1
}

# Create util_vector_logic
cell xilinx.com:ip:util_vector_logic or_0 {
  C_SIZE 1
  C_OPERATION or
} {
  Op1 slice_4/dout
  Op2 trig_0/trg_flag
}

# Create axis_combiner
cell  xilinx.com:ip:axis_combiner comb_0 {
  TDATA_NUM_BYTES.VALUE_SRC USER
  TDATA_NUM_BYTES 2
} {
  aclk /pll_0/clk_out1
  aresetn /rst_0/peripheral_aresetn
}

# Create axis_oscilloscope
cell pavel-demin:user:axis_oscilloscope scope_0 {
  AXIS_TDATA_WIDTH 32
  CNTR_WIDTH 23
} {
  S_AXIS comb_0/M_AXIS
  run_flag slice_5/dout
  trg_flag or_0/Res
  pre_data slice_7/dout
  tot_data slice_8/dout
  aclk /pll_0/clk_out1
  aresetn slice_0/dout
}

# Create xlconstant
cell xilinx.com:ip:xlconstant const_1 {
  CONST_WIDTH 18
  CONST_VAL 262143
}

# Create axis_ram_writer
cell pavel-demin:user:axis_ram_writer writer_0 {
  ADDR_WIDTH 18
  AXI_ID_WIDTH 3
  AXIS_TDATA_WIDTH 32
} {
  S_AXIS scope_0/M_AXIS
  min_addr slice_6/dout
  cfg_data const_1/dout
  aclk /pll_0/clk_out1
  aresetn slice_1/dout
}
